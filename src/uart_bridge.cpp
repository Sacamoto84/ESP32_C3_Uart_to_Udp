#include "uart_bridge.h"

#include "app_globals.h"
#include "network_bridge.h"

namespace
{
// Размер буфера драйвера UART.
constexpr int kUartRxBufferSize = 1024 * 64;

// Таймер нужен, чтобы паковать мелкие UART-чанки в более крупные UDP-пакеты.
unsigned long lastUartBatchAt = 0;

// Рабочий буфер под принятые данные из UART.
char uartDataBuffer[32768];
} // namespace

// Функция инициализации UART.
void initUART()
{
    // Теперь можно повысить мощность, если нужно:
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

    String ipClient = db.get(kk::ipClient);
    sendUdpMessage("UART to UDP " BOARD_LABEL " V" FW_VERSION "\n", ipClient.c_str());
}

// Фоновая задача чтения UART и пересылки данных по UDP.
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

        if (event.type != UART_DATA)
        {
            continue;
        }

        // Ограничиваем чтение размером локального буфера
        // и оставляем место под завершающий ноль для отладочного вывода.
        size_t maxRead = (event.size < sizeof(uartDataBuffer) - 1) ? event.size : sizeof(uartDataBuffer) - 1;
        int available = uart_read_bytes(UART_NUM_0, uartDataBuffer, maxRead, 0);

        if (available == -1)
        {
            Serial.println("UART ошибка чтения: available == -1");
            continue;
        }

        if (available >= (int)sizeof(uartDataBuffer))
        {
            Serial.printf("UART предотвращение переполнения буфера: available %d >= %d\n", available, sizeof(uartDataBuffer));
            available = sizeof(uartDataBuffer) - 1;
        }

        uartDataBuffer[available] = '\0';
        Serial.printf("UART received %d bytes\n", available);

        // Если пришёл совсем маленький кусок, даём ему шанс "донакопиться"
        // ещё до 50 мс, чтобы не плодить короткие UDP-пакеты.
        if ((available <= 1023) && (millis() - lastUartBatchAt <= 50))
        {
            continue;
        }

        Serial.printf("UART processing: available %d, time diff %lu ms\n", available, millis() - lastUartBatchAt);
        if (uartDataBuffer[0] == 0)
        {
            Serial.println("UART data[0] == 0, skipping");
            continue;
        }

        lastUartBatchAt = millis();
        eeprom.all_TX_to_UDP += available;
        Serial.printf("TX to UDP: %d байт, общее TX: %d\n", available, eeprom.all_TX_to_UDP);

        if (db.get(kk::echo))
        {
            // Отладочный вывод полезен, когда надо понять,
            // что именно уходит из UART в сеть.
            Serial.println("UDP_tx echo:");
            Serial.println(uartDataBuffer);
        }

        if (db.get(kk::broadcast))
        {
            // В режиме broadcast шлём пакет во всю сеть.
            sendUdpBroadcast(uartDataBuffer, available);
            continue;
        }

        // Обычный режим - отправка на конкретный IP клиента.
        String ipClient = db.get(kk::ipClient);
        sendUdpMessageLen(uartDataBuffer, available, ipClient.c_str());
    }
}
