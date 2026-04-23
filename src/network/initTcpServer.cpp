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

// Освобождает буферы очереди, выделенные предыдущими попытками создания.
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

// Пытается разместить очередь UART->TCP в PSRAM, чтобы лучше переживать всплески трафика.
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

// Запасной вариант: обычная очередь FreeRTOS во внутренней RAM.
bool tryCreateNetworkQueueInInternalRam(uint32_t queueLength)
{
    releaseStaticQueueBuffers();
    networkTxQueue = xQueueCreate(queueLength, sizeof(NetworkTxChunk));
    gNetworkTxQueueUsesPsram = false;
    return networkTxQueue != nullptr;
}

// Читает и ограничивает длину очереди, заданную в настройках.
uint32_t getConfiguredNetworkTxQueueLength()
{
    const int configuredLength = db.get(kk::networkTxQueueLength);

    if (configuredLength < kNetworkTxQueueMinLength)
    {
        return (uint32_t)kNetworkTxQueueMinLength;
    }

    if (configuredLength > kNetworkTxQueueMaxLength)
    {
        return (uint32_t)kNetworkTxQueueMaxLength;
    }

    return (uint32_t)configuredLength;
}

// Создаёт максимально большую очередь UART->TCP, которую тянет доступная память.
uint32_t createBestEffortNetworkQueue()
{
    uint32_t queueLength = getConfiguredNetworkTxQueueLength();

    Serial.printf("initTcpServer: configured TX queue length %u chunks\n",
                  (unsigned)queueLength);

    while (queueLength >= (uint32_t)kNetworkTxQueueMinLength)
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

        if (queueLength == (uint32_t)kNetworkTxQueueMinLength)
        {
            break;
        }

        queueLength /= 2;
        if (queueLength < (uint32_t)kNetworkTxQueueMinLength)
        {
            queueLength = (uint32_t)kNetworkTxQueueMinLength;
        }
    }

    releaseStaticQueueBuffers();
    networkTxQueue = nullptr;
    gNetworkTxQueueUsesPsram = false;
    return 0;
}
}

// Запускает оба TCP-сервера и связанные с ними фоновые задачи.
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

    // Поднимаем серверы даже если очередь не создалась, чтобы устройство всё равно
    // было видно по сети, а проблема с буферами читалась в логах.
    tcpServer.begin(kTcpServerPort);
    tcpServer.setNoDelay(true);
    tcpCommandServer.begin(kTcpCommandPort);
    tcpCommandServer.setNoDelay(true);

    xTaskCreate(networkTxTask, "networkTxTask", 6144, nullptr, 1, nullptr);
    xTaskCreate(networkRxTask, "networkRxTask", 6144, nullptr, 1, nullptr);
    initialized = true;

    if (actualNetworkTxQueueLength == 0)
    {
        Serial.printf("TCP servers started: stream=%u cmd=%u without TX queue\n",
                      kTcpServerPort,
                      kTcpCommandPort);
        return;
    }

    Serial.printf("TCP servers started: stream=%u cmd=%u, queue %u x %u bytes, storage=%s\n",
                  kTcpServerPort,
                  kTcpCommandPort,
                  (unsigned)actualNetworkTxQueueLength,
                  (unsigned)NETWORK_TX_CHUNK_SIZE,
                  gNetworkTxQueueUsesPsram ? "PSRAM" : "internal RAM");
}
