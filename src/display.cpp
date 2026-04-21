#include "display.h"

#include "app_globals.h"
#include "network_bridge.h"

// Яркость SSD1306 задаётся напрямую через регистр contrast.
void applyDisplayBrightness(uint8_t brightness)
{
#if PROJECT_HAS_SCREEN
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(brightness);
#else
    (void)brightness;
#endif
}

void initDisplay()
{
    // Инициализируем singleton с настройками заранее,
    // чтобы сразу применить сохранённую яркость экрана.
    EEPROM::getInstance();

#if !PROJECT_HAS_SCREEN
    return;
#else
#if OLED_USE_I2C
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR))
#else
    if (!display.begin(SSD1306_SWITCHCAPVCC))
#endif
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    applyDisplayBrightness((uint8_t)db.get(kk::screenBrightness));
    display.display();
#endif
}

// Локальный экран состояния показывается,
// когда внешний экран по UDP отключён.
void screenLoop()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
    EEPROM &eeprom = EEPROM::getInstance();

    // В STA показываем локальный IP, в AP - адрес точки доступа.
    const long rssi = WiFi.RSSI();
    const wifi_mode_t wifiMode = WiFi.getMode();
    const bool apMode = (wifiMode == WIFI_MODE_AP || wifiMode == WIFI_MODE_APSTA);
    const IPAddress ip = apMode ? WiFi.softAPIP() : WiFi.localIP();

    display.clearDisplay();
    display.setTextSize(1);

    display.setCursor(0, 2);
    display.print("IP: ");
    display.println(ip);

    display.setCursor(0, 11);
    if (db.get(kk::broadcast))
    {
        display.print("Mode: UDP bcast");
    }
    else
    {
        display.print("TCP: ");
        display.print(isTcpClientConnected() ? "client on" : "wait client");
    }

    display.setCursor(0, 20);
    display.print("Bitrate: ");
    display.print(db.get(kk::Serial2Bitrate));

    display.setCursor(0, 29);
    display.print(db.get(kk::echo) ? "Echo: True" : "Echo: False");

    display.setCursor(0, 38);
    display.print("Drop: ");
    display.print(getDroppedNetworkTxBytes());

    display.setTextSize(2);
    display.setCursor(0, 48);
    if (getNetworkTxQueueCapacity() == 0)
    {
        display.print("Q ERR");
    }
    else
    {
        display.print("TX:");
        display.print(eeprom.all_TX_to_network);
    }

    display.setTextSize(1);
    display.setCursor(104, 0);
    display.println(rssi);

    display.display();
#endif
}
