#include "define.h"
#include <GTimer.h>
#include <SPI.h>
#include <lwip/inet.h>

extern String WifiCurrentPowerString(int);
extern WiFiUDP udp;       // Объект для работы с UDP
extern byte buffer[1024]; // Буфер для входящих данных
extern QueueHandle_t uartQueue;
extern SettingsGyver sett;
extern GyverDBFile db;
extern Adafruit_SSD1306 display;

// Функция инициализации пинов
void initPins()
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
}

// Функция инициализации Serial и LittleFS
void initSerialAndFS()
{
    Serial.begin(460800);
    LittleFS.begin(true);
}

// Функция инициализации дисплея
void initDisplay()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    display.display();
}

// Функция инициализации WiFi
void initWiFi()
{
    EEPROM &eeprom = EEPROM::getInstance();

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
    int power = db.get(kk::wifiPower);
    WiFi.setTxPower((wifi_power_t)power); // ~11 dBm вместо 20 dBm
    // Это снизит пиковый ток на 50-100 мА!

    WiFi.begin(db.get(kk::WIFI_SSID), db.get(kk::WIFI_PASS));

    display.setTextSize(1);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.cp437(true);                 // Use full 256 char 'Code Page 437' font
    display.clearDisplay();
    display.print("Connecting ");
    display.println(WifiCurrentPowerString(db.get(kk::wifiPower)));
    display.display();

    while (WiFi.status() != WL_CONNECTED)
    {
        display.print(".");
        display.display();
        Serial.print(db.get(kk::wifiPower));
        Serial.print(".");
        delay(500);
        count++;
        if (count > 20)
        {
            // Сеть не найдена
            display.println("\nSet Power to 8.5 dBm");
            display.display();
            Serial.println("\nWifi STA не найдена");
            Serial.println("Снижаем мощность до 8.5dBm");
            WiFi.setTxPower(WIFI_POWER_8_5dBm); // ~11 dBm вместо 20 dBm
            WiFi.begin(db.get(kk::WIFI_SSID), db.get(kk::WIFI_PASS));
            count = 0;
            while (WiFi.status() != WL_CONNECTED)
            {
                display.print(".");
                display.display();
                Serial.print(".");
                delay(500);
                count++;
                if (count > 20)
                {
                    // Сеть не найдена
                    Serial.println("Wifi STA не найдена");
                    needAP = true; // Нужно создать точку доступа
                    break;         // Выходим
                }
            }

            break; // Выходим
        }
    }

    // Создаем точку доступа
    if (needAP)
    {
        display.println("\nStart APmode");
        display.display();
        Serial.println("Запуск точки доступа");
        // запускаем точку доступа
        WiFi.mode(WIFI_AP);
        WiFi.softAP("AP ESP32");
        digitalWrite(8, LOW);
    }

    display.println("\nConnected");
    display.display();

    Serial.println();
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());
}

// Функция инициализации настроек
void initSettings()
{
    sett.begin();
    sett.onBuild(build);
}

// Функция инициализации UART
void initUART()
{
    EEPROM &eeprom = EEPROM::getInstance();

    // Теперь можно повысить мощность если нужно
    // WiFi.setTxPower(WIFI_POWER_19_5dBm);

#define SERIAL2_SIZE_RX 1024 * 32

    uart_config_t config = {
        .baud_rate = db.get(kk::Serial2Bitrate),
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

    sendUdpMessage("UART to UDP C3 V1.5.5\n", db.get(kk::ipClient).c_str());
}

// Функция инициализации UDP
void initUDP()
{
    // Инициализация UDP на порту 82
    udp.begin(82);
}

// Основная функция setup
void setup()
{
    initPins();
    initSerialAndFS();
    initDisplay();
    initWiFi();
    initSettings();
    initUART();
    initUDP();
}