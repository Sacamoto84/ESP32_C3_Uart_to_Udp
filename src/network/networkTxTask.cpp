#include "network_internal.h"

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
