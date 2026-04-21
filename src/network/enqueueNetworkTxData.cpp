#include "network_internal.h"

size_t enqueueNetworkTxData(const uint8_t *data, size_t len)
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

    const bool useBroadcast = db.get(kk::broadcast);
    size_t queuedTotal = 0;

    while (queuedTotal < len)
    {
        const size_t remaining = len - queuedTotal;
        const size_t partLen = (remaining < NETWORK_TX_CHUNK_SIZE) ? remaining : NETWORK_TX_CHUNK_SIZE;

        NetworkTxChunk chunk = {};
        chunk.len = (uint16_t)partLen;
        chunk.useBroadcast = useBroadcast;
        memcpy(chunk.data, data + queuedTotal, partLen);

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
