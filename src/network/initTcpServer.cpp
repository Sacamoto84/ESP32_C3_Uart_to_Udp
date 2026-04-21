#include "network_internal.h"

void initTcpServer()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    networkTxQueue = xQueueCreate(NETWORK_TX_QUEUE_LENGTH, sizeof(NetworkTxChunk));
    if (networkTxQueue == nullptr)
    {
        Serial.println("initTcpServer: failed to create networkTxQueue");
        return;
    }

    droppedNetworkTxBytes = 0;
    tcpClientConnected = false;

    tcpServer.begin();
    tcpServer.setNoDelay(true);

    xTaskCreate(networkTxTask, "networkTxTask", 6144, nullptr, 1, nullptr);
    initialized = true;

    Serial.printf("TCP server started on port %u, queue %u x %u bytes\n",
                  kNetworkDataPort,
                  (unsigned)NETWORK_TX_QUEUE_LENGTH,
                  (unsigned)NETWORK_TX_CHUNK_SIZE);
}
