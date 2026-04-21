#include "network_internal.h"

uint32_t getQueuedNetworkTxChunks()
{
    if (networkTxQueue == nullptr)
    {
        return 0;
    }

    return (uint32_t)uxQueueMessagesWaiting(networkTxQueue);
}
