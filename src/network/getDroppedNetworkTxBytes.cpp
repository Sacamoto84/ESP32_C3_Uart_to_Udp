#include "network_internal.h"

// Возвращает суммарное число UART-байтов, потерянных из-за полной очереди.
uint32_t getDroppedNetworkTxBytes()
{
    return droppedNetworkTxBytes;
}
