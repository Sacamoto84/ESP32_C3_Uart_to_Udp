#include "display.h"

#include "app_globals.h"

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
    // чтобы можно было сразу применить сохранённую яркость экрана.
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

// Локальный экран состояния, который показывается,
// когда внешний экран по UDP отключен.
void screenLoop()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
    EEPROM &eeprom = EEPROM::getInstance();

    // Получение уровня сигнала WiFi (RSSI).
    long rssi = WiFi.RSSI();
    IPAddress ip = WiFi.localIP();

    display.clearDisplay();
    display.setTextSize(1);

    display.setCursor(0, 2);
    display.print("IP: ");
    display.println(ip);

    display.setCursor(0, 11);
    display.print("Client: ");
    display.print(db.get(kk::ipClient));

    display.setCursor(0, 20);
    display.print("Bitrate: ");
    display.print(db.get(kk::Serial2Bitrate));

    display.setCursor(0, 29);
    display.print(db.get(kk::broadcast) ? "Broadcast: True" : "Broadcast: False");

    display.setCursor(0, 38);
    display.print(db.get(kk::echo) ? "Echo:      True" : "Echo:      False");

    display.setTextSize(2);
    display.setCursor(0, 48);
    display.print("TX:");
    display.print(eeprom.all_TX_to_UDP);

    display.setTextSize(1);
    display.setCursor(110, 0);
    display.println(rssi);

    display.display();
#endif
}
