#include "network_internal.h"

#include "esp32-hal-psram.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace
{
StaticQueue_t *gNetworkTxQueueStruct = nullptr;
uint8_t *gNetworkTxQueueStorage = nullptr;
bool gNetworkTxQueueUsesPsram = false;

void releaseStaticQueueBuffers()
{
    if (gNetworkTxQueueStorage != nullptr)
    {
        free(gNetworkTxQueueStorage);
        gNetworkTxQueueStorage = nullptr;
    }

    if (gNetworkTxQueueStruct != nullptr)
    {
        free(gNetworkTxQueueStruct);
        gNetworkTxQueueStruct = nullptr;
    }
}

bool tryCreateNetworkQueueInPsram(uint32_t queueLength)
{
    if (!psramFound())
    {
        return false;
    }

    releaseStaticQueueBuffers();

    const size_t itemSize = sizeof(NetworkTxChunk);
    const size_t storageSize = queueLength * itemSize;

    gNetworkTxQueueStruct = static_cast<StaticQueue_t *>(
        heap_caps_calloc(1, sizeof(StaticQueue_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    gNetworkTxQueueStorage = static_cast<uint8_t *>(
        heap_caps_malloc(storageSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (gNetworkTxQueueStruct == nullptr || gNetworkTxQueueStorage == nullptr)
    {
        releaseStaticQueueBuffers();
        return false;
    }

    networkTxQueue = xQueueCreateStatic(
        queueLength,
        itemSize,
        gNetworkTxQueueStorage,
        gNetworkTxQueueStruct);

    if (networkTxQueue == nullptr)
    {
        releaseStaticQueueBuffers();
        return false;
    }

    gNetworkTxQueueUsesPsram = true;
    return true;
}

bool tryCreateNetworkQueueInInternalRam(uint32_t queueLength)
{
    releaseStaticQueueBuffers();
    networkTxQueue = xQueueCreate(queueLength, sizeof(NetworkTxChunk));
    gNetworkTxQueueUsesPsram = false;
    return networkTxQueue != nullptr;
}

// Пытаемся создать максимально большую очередь для burst-трафика UART.
// Если памяти не хватает, автоматически откатываемся на более короткую очередь,
    // чтобы TCP server всё равно поднялся и устройство не осталось без порта 8888.
uint32_t createBestEffortNetworkQueue()
{
    uint32_t queueLength = NETWORK_TX_QUEUE_LENGTH;

    while (queueLength >= 4)
    {
        if (tryCreateNetworkQueueInPsram(queueLength))
        {
            return queueLength;
        }

        if (tryCreateNetworkQueueInInternalRam(queueLength))
        {
            return queueLength;
        }

        Serial.printf("initTcpServer: queue length %u failed in PSRAM and internal RAM, trying smaller queue\n",
                      (unsigned)queueLength);
        queueLength /= 2;
    }

    releaseStaticQueueBuffers();
    networkTxQueue = nullptr;
    gNetworkTxQueueUsesPsram = false;
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

    Serial.printf("PSRAM: found=%s total=%u free=%u max=%u\n",
                  psramFound() ? "yes" : "no",
                  (unsigned)ESP.getPsramSize(),
                  (unsigned)ESP.getFreePsram(),
                  (unsigned)ESP.getMaxAllocPsram());

    // TCP server поднимаем даже если очередь не удалось создать.
    // В таком случае подключение к 8888 всё равно будет видно,
    // а в логах станет понятно, что проблема именно в буферах.
    tcpServer.begin(kTcpServerPort);
    tcpServer.setNoDelay(true);

    xTaskCreate(networkTxTask, "networkTxTask", 6144, nullptr, 1, nullptr);
    initialized = true;

    if (actualNetworkTxQueueLength == 0)
    {
        Serial.printf("TCP server started on port %u without TX queue\n", kTcpServerPort);
        return;
    }

    Serial.printf("TCP server started on port %u, queue %u x %u bytes, storage=%s\n",
                  kTcpServerPort,
                  (unsigned)actualNetworkTxQueueLength,
                  (unsigned)NETWORK_TX_CHUNK_SIZE,
                  gNetworkTxQueueUsesPsram ? "PSRAM" : "internal RAM");
}
