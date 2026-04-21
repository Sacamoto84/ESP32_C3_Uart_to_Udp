#include "network_internal.h"

namespace
{
// Пытаемся создать максимально большую очередь для burst-трафика UART.
// Если памяти не хватает, автоматически откатываемся на более короткую очередь,
// чтобы TCP server всё равно поднялся и устройство не осталось без порта 8888.
uint32_t createBestEffortNetworkQueue()
{
    uint32_t queueLength = NETWORK_TX_QUEUE_LENGTH;

    while (queueLength >= 4)
    {
        networkTxQueue = xQueueCreate(queueLength, sizeof(NetworkTxChunk));
        if (networkTxQueue != nullptr)
        {
            return queueLength;
        }

        Serial.printf("initTcpServer: queue length %u failed, trying smaller queue\n", (unsigned)queueLength);
        queueLength /= 2;
    }

    networkTxQueue = nullptr;
    return 0;
}
} // namespace

void initTcpServer()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    droppedNetworkTxBytes = 0;
    tcpClientConnected = false;
    actualNetworkTxQueueLength = createBestEffortNetworkQueue();

    // TCP server поднимаем даже если очередь не удалось создать.
    // В таком случае подключение к 8888 всё равно будет видно,
    // а в логах станет понятно, что проблема именно в буферах.
    tcpServer.begin(kNetworkDataPort);
    tcpServer.setNoDelay(true);

    xTaskCreate(networkTxTask, "networkTxTask", 6144, nullptr, 1, nullptr);
    initialized = true;

    if (actualNetworkTxQueueLength == 0)
    {
        Serial.printf("TCP server started on port %u without TX queue\n", kNetworkDataPort);
        return;
    }

    Serial.printf("TCP server started on port %u, queue %u x %u bytes\n",
                  kNetworkDataPort,
                  (unsigned)actualNetworkTxQueueLength,
                  (unsigned)NETWORK_TX_CHUNK_SIZE);
}
