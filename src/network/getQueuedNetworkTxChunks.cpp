#include "network_internal.h"

// Возвращает текущую заполненность очереди UART->TCP.
uint32_t getQueuedNetworkTxChunks()
{
    if (networkTxQueue == nullptr)
    {
        return 0;
    }

    return (uint32_t)uxQueueMessagesWaiting(networkTxQueue);
}
