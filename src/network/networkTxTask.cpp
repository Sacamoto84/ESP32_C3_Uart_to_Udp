#include "network_internal.h"

namespace
{
// Логи идут отдельным буфером, чтобы отсутствие TCP-клиента не считалось
// потерей UART-данных и не влияло на их очередь.
uint8_t gTcpLogChunk[NETWORK_TX_CHUNK_SIZE];

// Выкидывает все оставшиеся чанки и учитывает их в счётчике потерь.
// Вызывается, когда TCP-клиент не подключён: держать старые UART-данные в PSRAM нет смысла,
// свежий клиент получит актуальный поток.
void discardPendingTxData(NetworkTxChunk &pendingChunk, bool &hasPendingChunk)
{
    if (hasPendingChunk)
    {
        droppedNetworkTxBytes += pendingChunk.len;
        hasPendingChunk = false;
    }

    if (networkTxQueue == nullptr)
    {
        return;
    }

    NetworkTxChunk flushed;
    while (xQueueReceive(networkTxQueue, &flushed, 0) == pdTRUE)
    {
        droppedNetworkTxBytes += flushed.len;
    }
}
}

// Постоянно принимает потоковых клиентов и отправляет им чанки из UART-очереди.
void networkTxTask(void *arg)
{
    (void)arg;

    NetworkTxChunk pendingChunk = {};
    bool hasPendingChunk = false;
    bool tcpLogCursorInitialized = false;
    uint32_t tcpLogCursor = 0;

    for (;;)
    {
        pollTcpServer();

        if (!tcpClientConnected)
        {
            tcpLogCursorInitialized = false;
            // Клиента нет: дренируем накопленные UART-данные, чтобы память не держалась
            // занятой и новый клиент получил актуальный поток без исторического хвоста.
            if (networkTxQueue != nullptr)
            {
                discardPendingTxData(pendingChunk, hasPendingChunk);
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (!tcpLogCursorInitialized)
        {
            // Новый клиент начинает с начала доступной истории, затем получает
            // новые строки по мере их появления.
            tcpLogCursor = getEsp32TcpLogOldestSequence();
            tcpLogCursorInitialized = true;
        }

        const size_t logLength = readEsp32TcpLog(tcpLogCursor,
                                                 gTcpLogChunk,
                                                 sizeof(gTcpLogChunk));
        if (logLength > 0)
        {
            if (!sendTcpChunk(gTcpLogChunk, logLength))
            {
                tcpLogCursorInitialized = false;
            }
            continue;
        }

        // Диагностика доступна даже при ошибке создания UART->TCP очереди.
        if (networkTxQueue == nullptr)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (!hasPendingChunk)
        {
            if (xQueueReceive(networkTxQueue, &pendingChunk, pdMS_TO_TICKS(20)) != pdTRUE)
            {
                continue;
            }

            hasPendingChunk = true;
        }

        if (sendTcpChunk(pendingChunk.data, pendingChunk.len))
        {
            hasPendingChunk = false;
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}
