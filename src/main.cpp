#include "define.h"

QueueHandle_t uartQueue;

// #include "ESPAsyncUDP.h"

SettingsGyver sett("My Settings");

// AsyncUDP udp;

int slider;
String input1;
bool swit;

sets::Logger logger(150); // размер буфера

void build(sets::Builder &b)
{
    b.Slider("My slider", 0, 50, 1, "", &slider);
    b.Input("My input", &input1);
    b.Switch("Броадкаст", &swit);

    b.HTML("", R"(<a href="http://google.com">Google</a>)");

    if (b.Button())
    {
        Serial.println("Click!");
        logger.println(millis());
    }

    b.Log(H(log), logger);
    if (b.Button("Test"))
    {
        // печатать как в Serial в любом месте в программе
        logger.println(millis());
    }

    if (b.beginGroup("Group 1"))
    {
        b.Input();

        b.endGroup(); // закрыть группу
    }

    if (b.beginGroup("Group 2"))
    {
        b.Input();

        b.endGroup(); // закрыть группу
    }

    {
        sets::Menu m(b, "menu 1");
        if (b.enterMenu())
            Serial.println("menu 1");
        // b.Input();
    }
    {
        sets::Menu m(b, "menu 2");
        if (b.enterMenu())
            Serial.println("menu 2");
        // b.Input();
        {
            sets::Menu m(b, "menu 2.1");
            if (b.enterMenu())
                Serial.println("menu 2.1");
            // b.Input();
        }

        {
            sets::Row g(b);
            // sets::Row g(b, "Row");

            b.Slider("Slider");
            b.LED();
            b.Switch();
        }

        {
            sets::Row g(b, "Row");

            b.Slider("Slider");
            b.LED();
            b.Switch();
        }
    }
}

void setup()
{
    Serial.begin(460800);
    LittleFS.begin(true);

    EEPROM &eeprom = EEPROM::getInstance();

    int count = 0;
    bool needAP = false; // Если сети нет создаем точку доступа

    WiFi.mode(WIFI_STA);
    WiFi.begin(eeprom.WIFI_SSID, eeprom.WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
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

    sendUdpMessage("UART to UDP C3 V1.0\n", eeprom.ipClient.c_str());

}

static unsigned long lastPrint = 0;

void loop()
{
    sett.tick();
    db.tick();

    if (millis() - lastPrint >= 2000)
    {
        lastPrint = millis();
        Serial.println(WiFi.localIP());
    }
}