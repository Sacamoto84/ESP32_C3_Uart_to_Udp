#include "define.h"
#include <GTimer.h>
#include <SPI.h>
#include <lwip/inet.h>

WiFiUDP udp; // Объект для работы с UDP

byte buffer[1024]; // Буфер для входящих данных
QueueHandle_t uartQueue;
SettingsGyver sett("My Settings", &db);
GyverDBFile db(&LittleFS, "/data.db", 500);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

void setup()
{

    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OPEN_DRAIN);
    pinMode(10, OUTPUT);

    Serial.begin(460800);
    LittleFS.begin(true);

    EEPROM &eeprom = EEPROM::getInstance();

    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    display.display();

    int count = 0;
    bool needAP = false; // Если сети нет создаем точку доступа

    // Снижаем частоту CPU перед запуском WiFi
    setCpuFrequencyMhz(80); // вместо 160 MHz

    WiFi.mode(WIFI_STA);

    // От слабого к сильному сигналу:
    // WIFI_POWER_MINUS_1dBm    // ~2 dBm   (~80 мА пик)
    // WIFI_POWER_2dBm          // ~5 dBm   (~100 мА пик)
    // WIFI_POWER_8_5dBm        // ~8.5 dBm (~150 мА пик) ← вы сейчас тут
    // WIFI_POWER_11dBm         // ~11 dBm  (~200 мА пик) ← попробуйте это
    // WIFI_POWER_13dBm         // ~13 dBm  (~250 мА пик)
    // WIFI_POWER_15dBm         // ~15 dBm  (~300 мА пик)
    // WIFI_POWER_17dBm         // ~17 dBm  (~350 мА пик)
    // WIFI_POWER_19dBm         // ~19 dBm  (~420 мА пик)
    // WIFI_POWER_19_5dBm       // ~20 dBm  (~480 мА пик) ← максимум

    // КРИТИЧНО: снижаем мощность TX
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // ~11 dBm вместо 20 dBm
    // Это снизит пиковый ток на 50-100 мА!

    WiFi.begin(eeprom.WIFI_SSID, eeprom.WIFI_PASS);
    // WiFi.begin("TP-Link_BC0C", "58133514");

    Serial.println(eeprom.WIFI_SSID);
    Serial.println(eeprom.WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(eeprom.WIFI_SSID);
        Serial.println(eeprom.WIFI_PASS);
        delay(500);
        Serial.print(".");
        count++;
        if (count > 20)
        {
            // Сеть не найдена
            Serial.println("Wifi STA не найдена");
            needAP = true; // Нужно создать точку доступа
            break;         // Выходим
        }
    }

    // Создаем точку доступа
    if (needAP)
    {
        Serial.println("Запуск точки доступа");
        // запускаем точку доступа
        WiFi.mode(WIFI_AP);
        WiFi.softAP("AP ESP32");
        digitalWrite(8, LOW);
    }

    Serial.println();
    Serial.print("Connected: ");
    String LocalIp = WiFi.localIP().toString();
    Serial.println(WiFi.localIP());

    sett.begin();
    sett.onBuild(build);

    // Теперь можно повысить мощность если нужно
    // WiFi.setTxPower(WIFI_POWER_19_5dBm);

    #define SERIAL2_SIZE_RX 1024 * 32

    uart_config_t config = {
        .baud_rate = eeprom.Serial2Bitrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122};

    uart_param_config(UART_NUM_0, &config);
    uart_driver_install(UART_NUM_0, SERIAL2_SIZE_RX, 256, 100, &uartQueue, 0);
    uart_set_pin(UART_NUM_0, 21, 20, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_flush_input(UART_NUM_0);
    xTaskCreate(uartTask, "uartTask", 10000, NULL, 1, NULL);

    sendUdpMessage("UART to UDP C3 V1.4\n", eeprom.ipClient.c_str());

    // Инициализация UDP на порту 82
    udp.begin(82);
}

static unsigned long lastPrint = 0;

void loop()
{
    EEPROM &eeprom = EEPROM::getInstance();

    static GTimer<millis> tmr(5000, true);

    sett.tick();
    db.tick();

    if (tmr)
    {
        Serial.println(WiFi.localIP());
    }

    if (eeprom.externalScreen)
    {
        // Обработка входящих UDP-пакетов
        int packetSize = udp.parsePacket();
        if (packetSize)
        {
            // Serial.printf("Получен UDP-пакет размером %d байт от %s\n", packetSize, udp.remoteIP().toString().c_str());
            if (packetSize == 1024) // Ожидаем ровно 1024 байта
            {
                int bytesRead = udp.read(display.getBuffer(), 1024);
                if (bytesRead == 1024)
                    display.display(); // Получено 1024 байт, обновляем дисплей
                else
                    Serial.printf("Ошибка: прочитано только %d байт\n", bytesRead);
            }
            else
            {
                // Serial.printf("Ошибка: размер пакета %d байт, ожидалось 1024\n", packetSize);
                //  Очистка буфера, если пакет некорректного размера
                while (udp.available())
                {
                    udp.read();
                }
            }
        }
    }
}