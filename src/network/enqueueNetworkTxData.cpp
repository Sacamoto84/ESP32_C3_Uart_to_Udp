#include "network_internal.h"

namespace
{
// Копирует диапазон байтов в один элемент очереди FreeRTOS.
static void IRAM_ATTR fillNetworkTxChunk(NetworkTxChunk &chunk, const uint8_t *src, size_t len)
{
    chunk.len = (uint16_t)len;
    memcpy(chunk.data, src, len);
}
}

// Разбивает произвольный буфер UART на чанки размера очереди и ставит в неё сколько получится.
size_t IRAM_ATTR enqueueNetworkTxData(const uint8_t *data, size_t len)
{
    static uint32_t lastOverflowLogAt = 0;

    if (data == nullptr || len == 0)
    {
        return 0;
    }

    if (networkTxQueue == nullptr)
    {
        Serial.println("enqueueNetworkTxData: networkTxQueue is not initialized");
        return 0;
    }

    size_t queuedTotal = 0;

    while (queuedTotal < len)
    {
        const size_t remaining = len - queuedTotal;
        const size_t partLen = (remaining < NETWORK_TX_CHUNK_SIZE) ? remaining : NETWORK_TX_CHUNK_SIZE;

        NetworkTxChunk chunk;
        fillNetworkTxChunk(chunk, data + queuedTotal, partLen);

        if (xQueueSend(networkTxQueue, &chunk, 0) != pdTRUE)
        {
            const uint32_t droppedNow = (uint32_t)(len - queuedTotal);
            droppedNetworkTxBytes += droppedNow;

            if (millis() - lastOverflowLogAt > 1000)
            {
                Serial.printf("enqueueNetworkTxData: queue full, dropped %u bytes, queued chunks %u\n",
                              droppedNow,
                              (unsigned)uxQueueMessagesWaiting(networkTxQueue));
                lastOverflowLogAt = millis();
            }
            break;
        }

        queuedTotal += partLen;
    }

    return queuedTotal;
}
