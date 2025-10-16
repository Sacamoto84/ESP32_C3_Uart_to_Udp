#include "define.h"

#include <GTimer.h>

#include <lwip/inet.h>

QueueHandle_t uartQueue;

SettingsGyver sett("My Settings", &db);

GyverDBFile db(&LittleFS, "/data.db", 500);

#include <SPI.h>

bool isValidIp(const char *ip)
{
    struct in_addr addr;
    return inet_aton(ip, &addr); // вернёт true, если ip корректный
}

void build(sets::Builder &b)
{
    Serial.println("build");

    EEPROM &eeprom = EEPROM::getInstance();

    String tempIpClient = eeprom.ipClient;

    if (b.Input("Ip Адресс клиента", &eeprom.ipClient))
    {
        Serial.println(b.build.value);
        const char *ip = b.build.value.c_str();

        if (isValidIp(ip))
        {
            Serial.printf("✅ Корректный IP: %s\n", ip);
            db.set(kk::ipClient, ip);
            tempIpClient = ip;
            db.update();
        }
        else
        {
            Serial.printf("❌ Некорректный IP: %s\n", ip);
            eeprom.ipClient = tempIpClient;
            sett.reload(true);
        }
    }

    b.Number(kk::Serial2Bitrate, "Битрейт");

    if (b.Switch("Эхо", &eeprom.echo))
    {
        Serial.println(b.build.value);
        db.set(kk::echo, b.build.value);
        db.update();
    }

    if (b.Switch("Броадкаст", &eeprom.broadcast))
    {
        Serial.println(b.build.value);
        db.set(kk::broadcast, b.build.value);
        db.update();
    }

    {
        sets::Group g(b, "WiFi");
        b.Input(kk::WIFI_SSID, "SSID");
        b.Input(kk::WIFI_PASS, "Password");
    }

    if (b.Button("Restart"))
    {
        ESP.restart();
    }
}

void setup()
{

    // tft.init();
    // tft.setRotation(3);
    // tft.fillScreen(TFT_BLACK);
    // tft.setTextSize(1);
    // tft.setTextColor(TFT_GREEN);
    // tft.setCursor(0, 0);

    Serial.begin(460800);
    LittleFS.begin(true);

    EEPROM &eeprom = EEPROM::getInstance();

    int count = 0;
    bool needAP = false; // Если сети нет создаем точку доступа

    delay(2000); // или больше

    WiFi.mode(WIFI_STA);
    // WiFi.begin(eeprom.WIFI_SSID, eeprom.WIFI_PASS);
    WiFi.begin("TP-Link_BC0C", "58133514");

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

#define SERIAL2_SIZE_RX 1024 * 96

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

    sendUdpMessage("UART to UDP C3 V1.1\n", eeprom.ipClient.c_str());
}

static unsigned long lastPrint = 0;

void loop()
{
    static GTimer<millis> tmr(5000, true);

    sett.tick();
    db.tick();

    if (tmr)
    {
        Serial.println(WiFi.localIP());
    }
}