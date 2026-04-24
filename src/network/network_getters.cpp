#include "network_internal.h"

// Небольшие read-only геттеры для состояния сетевого моста.
// Собраны в одном TU, чтобы не плодить отдельные единицы трансляции ради одной строки каждой.

// Отдаёт кэшированное состояние потокового TCP-клиента для интерфейса и диагностики.
bool isTcpClientConnected()
{
    return tcpClientConnected;
}

// Возвращает суммарное число UART-байтов, потерянных из-за полной очереди.
uint32_t getDroppedNetworkTxBytes()
{
    return droppedNetworkTxBytes;
}

// Возвращает ёмкость очереди, выбранную во время старта прошивки.
uint32_t getNetworkTxQueueCapacity()
{
    return actualNetworkTxQueueLength;
}

// Возвращает текущую заполненность очереди UART->TCP.
uint32_t getQueuedNetworkTxChunks()
{
    if (networkTxQueue == nullptr)
    {
        return 0;
    }

    return (uint32_t)uxQueueMessagesWaiting(networkTxQueue);
}
