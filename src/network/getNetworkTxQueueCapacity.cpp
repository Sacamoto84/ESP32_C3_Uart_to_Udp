#include "network_internal.h"

// Возвращает ёмкость очереди, выбранную во время старта прошивки.
uint32_t getNetworkTxQueueCapacity()
{
    return actualNetworkTxQueueLength;
}
