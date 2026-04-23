#include "uart_bridge.h"

#include "app_globals.h"
#include "network_bridge.h"

namespace
{
constexpr int kBytesPerKb = 1024;
constexpr int kUartTxBufferSize = 256;
constexpr int kUartEventQueueLength = 100;

// Снимок памяти для логирования нагрузки на RAM при выборе размера UART RX-буфера.
struct MemorySnapshot
{
    uint32_t heapSize;
    uint32_t freeHeap;
    uint32_t maxHeapBlock;
    uint32_t psramSize;
    uint32_t freePsram;
    uint32_t maxPsramBlock;
};

// Локальный буфер чтения UART перед тем, как данные разрежутся на сетевые чанки.
char uartDataBuffer[NETWORK_TX_CHUNK_SIZE * 4];

// Считывает текущее состояние кучи и PSRAM для диагностики.
MemorySnapshot captureMemorySnapshot()
{
    return {
        ESP.getHeapSize(),
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap(),
        ESP.getPsramSize(),
        ESP.getFreePsram(),
        ESP.getMaxAllocPsram()};
}

// Возвращает знаковую разницу между двумя счётчиками памяти для удобного лога.
long getMemoryDelta(uint32_t before, uint32_t after)
{
    return (long)after - (long)before;
}

// Пишет в лог состояние памяти до попытки аллокации UART-драйвера.
void logUartMemorySnapshot(const char *stage, int bufferKb, const MemorySnapshot &snapshot)
{
    Serial.printf("UART RX alloc %s %d KB: heap=%u/%u maxHeapBlock=%u psram=%u/%u maxPsramBlock=%u\n",
                  stage,
                  bufferKb,
                  (unsigned)snapshot.freeHeap,
                  (unsigned)snapshot.heapSize,
                  (unsigned)snapshot.maxHeapBlock,
                  (unsigned)snapshot.freePsram,
                  (unsigned)snapshot.psramSize,
                  (unsigned)snapshot.maxPsramBlock);
}

// Логирует, как изменилась память после очередной попытки аллокации UART-драйвера.
void logUartMemoryDelta(int bufferKb,
                        esp_err_t result,
                        const MemorySnapshot &before,
                        const MemorySnapshot &after)
{
    Serial.printf("UART RX alloc after %d KB result=%d: freeHeap=%u (%ld) maxHeapBlock=%u (%ld) freePsram=%u (%ld) maxPsramBlock=%u (%ld)\n",
                  bufferKb,
                  (int)result,
                  (unsigned)after.freeHeap,
                  getMemoryDelta(before.freeHeap, after.freeHeap),
                  (unsigned)after.maxHeapBlock,
                  getMemoryDelta(before.maxHeapBlock, after.maxHeapBlock),
                  (unsigned)after.freePsram,
                  getMemoryDelta(before.freePsram, after.freePsram),
                  (unsigned)after.maxPsramBlock,
                  getMemoryDelta(before.maxPsramBlock, after.maxPsramBlock));
}

// Читает и ограничивает настроенный размер UART RX-буфера в килобайтах.
int getConfiguredSerialRxBufferKb()
{
    const int configuredKb = db.get(kk::serialRxBufferKb);

    if (configuredKb < kSerialRxBufferMinKb)
    {
        return kSerialRxBufferMinKb;
    }

    if (configuredKb > kSerialRxBufferMaxKb)
    {
        return kSerialRxBufferMaxKb;
    }

    return configuredKb;
}

// Пытается запустить UART-драйвер с заданным RX-буфером, уменьшая его при нехватке памяти.
int installUartDriverBestEffort()
{
    int bufferKb = getConfiguredSerialRxBufferKb();

    Serial.printf("UART RX buffer configured %d KB\n", bufferKb);

    while (bufferKb >= kSerialRxBufferMinKb)
    {
        const int rxBufferSize = bufferKb * kBytesPerKb;
        const MemorySnapshot before = captureMemorySnapshot();
        logUartMemorySnapshot("before", bufferKb, before);

        const esp_err_t result = uart_driver_install(
            UART_NUM_0,
            rxBufferSize,
            kUartTxBufferSize,
            kUartEventQueueLength,
            &uartQueue,
            0);

        const MemorySnapshot after = captureMemorySnapshot();
        logUartMemoryDelta(bufferKb, result, before, after);

        if (result == ESP_OK)
        {
            Serial.printf("UART driver started, RX buffer %d KB\n", bufferKb);
            return bufferKb;
        }

        Serial.printf("uart_driver_install failed for RX buffer %d KB: %d\n",
                      bufferKb,
                      (int)result);

        if (bufferKb == kSerialRxBufferMinKb)
        {
            break;
        }

        bufferKb /= 2;
        if (bufferKb < kSerialRxBufferMinKb)
        {
            bufferKb = kSerialRxBufferMinKb;
        }
    }

    return 0;
}
}

// Настраивает UART0 и запускает задачу, которая мостит UART RX в TCP 8888.
void initUART()
{
    uart_config_t config = {
        .baud_rate = db.get(kk::Serial2Bitrate),
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    const esp_err_t configResult = uart_param_config(UART_NUM_0, &config);
    if (configResult != ESP_OK)
    {
        Serial.printf("uart_param_config failed: %d\n", (int)configResult);
        return;
    }

    if (installUartDriverBestEffort() == 0)
    {
        Serial.println("UART driver start failed: RX buffer was not allocated");
        return;
    }

    const esp_err_t pinResult = uart_set_pin(UART_NUM_0, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (pinResult != ESP_OK)
    {
        Serial.printf("uart_set_pin failed: %d\n", (int)pinResult);
        return;
    }
#if defined(HW_VARIANT_ESP32_S2_MINI)
    gpio_set_pull_mode((gpio_num_t)UART_RX_PIN, GPIO_PULLDOWN_ONLY);
#endif
    uart_flush_input(UART_NUM_0);
    if (xTaskCreate(uartTask, "uartTask", 10000, nullptr, 1, nullptr) != pdPASS)
    {
        Serial.println("uartTask create failed");
        return;
    }

    const char *banner = "UART to TCP server " BOARD_LABEL " V" FW_VERSION "\n";
    enqueueNetworkTxData((const uint8_t *)banner, strlen(banner));
}

// Обрабатывает события UART, читает входящие байты и ставит их в сетевую очередь.
void uartTask(void *arg)
{
    (void)arg;

    EEPROM &eeprom = EEPROM::getInstance();
    uart_event_t event;

    while (true)
    {
        if (!xQueueReceive(uartQueue, (void *)&event, portMAX_DELAY))
        {
            continue;
        }

        if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL)
        {
            Serial.println("UART overflow event, flushing driver input buffer");
            uart_flush_input(UART_NUM_0);
            xQueueReset(uartQueue);
            continue;
        }

        if (event.type != UART_DATA)
        {
            continue;
        }

        // Ограничиваем чтение размером локального буфера и оставляем один байт
        // под завершающий ноль, если включён echo-лог.
        const size_t maxRead = (event.size < sizeof(uartDataBuffer) - 1) ? event.size : sizeof(uartDataBuffer) - 1;
        int available = uart_read_bytes(UART_NUM_0, uartDataBuffer, maxRead, 0);

        if (available <= 0)
        {
            Serial.printf("UART read skipped, available=%d\n", available);
            continue;
        }

        if (available >= (int)sizeof(uartDataBuffer))
        {
            Serial.printf("UART local buffer clamp: %d -> %u\n", available, (unsigned)(sizeof(uartDataBuffer) - 1));
            available = sizeof(uartDataBuffer) - 1;
        }

        const bool echoEnabled = db.get(kk::echo);
        if (echoEnabled)
        {
            uartDataBuffer[available] = '\0';

            // Оставляем читаемую копию в Serial-логе, чтобы можно было глазами
            // увидеть, что именно уходит из UART в транспортный слой.
            Serial.println("NET_tx echo:");
            Serial.println(uartDataBuffer);
        }

        const size_t queued = enqueueNetworkTxData((const uint8_t *)uartDataBuffer, (size_t)available);
        eeprom.all_TX_to_network += (int)queued;

        if (queued < (size_t)available)
        {
            Serial.printf("UART -> network queue overflow: queued %u of %u bytes, dropped total %u\n",
                          (unsigned)queued,
                          (unsigned)available,
                          (unsigned)getDroppedNetworkTxBytes());
        }
        else if (PROJECT_UART_VERBOSE_LOG_ENABLED)
        {
            Serial.printf("TX to network queue: %u bytes, total TX: %d\n",
                          (unsigned)queued,
                          eeprom.all_TX_to_network);
        }
    }
}
