#include "network_internal.h"

namespace
{
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

    for (;;)
    {
        pollTcpServer();

        if (networkTxQueue == nullptr)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (!tcpClientConnected)
        {
            // Клиента нет: дренируем накопленные UART-данные, чтобы память не держалась
            // занятой и новый клиент получил актуальный поток без исторического хвоста.
            discardPendingTxData(pendingChunk, hasPendingChunk);
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
