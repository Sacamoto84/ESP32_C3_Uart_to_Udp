#include "uart_bridge.h"

#include "app_globals.h"
#include "network_bridge.h"

namespace
{
// Буфер драйвера UART.
// Он нужен, чтобы короткие всплески входящего потока не терялись сразу на IRQ-уровне.
constexpr int kUartRxBufferSize = 1024 * 64;

// Локальный буфер чтения из драйвера UART.
// Дальше данные уже дробятся на сетевые чанки фиксированного размера.
char uartDataBuffer[NETWORK_TX_CHUNK_SIZE * 4];
} // namespace

// Инициализация UART.
void initUART()
{
    // При необходимости мощность Wi-Fi можно поднять отдельно:
    // WiFi.setTxPower(WIFI_POWER_19_5dBm);
    uart_config_t config = {
        .baud_rate = db.get(kk::Serial2Bitrate),
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    uart_param_config(UART_NUM_0, &config);
    uart_driver_install(UART_NUM_0, kUartRxBufferSize, 256, 100, &uartQueue, 0);
    uart_set_pin(UART_NUM_0, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
#if defined(HW_VARIANT_ESP32_S2_MINI)
    gpio_set_pull_mode((gpio_num_t)UART_RX_PIN, GPIO_PULLDOWN_ONLY);
#endif
    uart_flush_input(UART_NUM_0);
    xTaskCreate(uartTask, "uartTask", 10000, nullptr, 1, nullptr);

    const char *banner = "UART to TCP server " BOARD_LABEL " V" FW_VERSION "\n";
    enqueueNetworkTxData((const uint8_t *)banner, strlen(banner));
}

// Фоновая задача чтения UART и постановки данных в ограниченную сетевую очередь.
// Такой разрыв между UART и TCP нужен, чтобы медленная сеть не блокировала чтение UART
// и не приводила к крашу или разрастанию буферов.
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

        // Читаем не больше локального буфера и оставляем байт под завершающий ноль,
        // но реально ставим его только когда включен echo-лог.
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

            // Echo оставлен как и раньше, чтобы можно было глазами посмотреть,
            // что уходит из UART в транспортный слой.
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
