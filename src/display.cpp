#include "display.h"

#include "app_globals.h"
#include "hardware.h"
#include "network_bridge.h"

namespace
{
constexpr const uint8_t *kDisplayFont = u8g2_font_7x13_tr;
constexpr uint8_t kStatusLine0Y = 0;
constexpr uint8_t kStatusLine1Y = 13;
constexpr uint8_t kStatusLine2Y = 26;
constexpr uint8_t kStatusLine3Y = 39;
constexpr uint8_t kStatusLine4Y = 52;

// Сбрасывает общие параметры рисования текста перед выводом нового кадра состояния.
void prepareTextFrame()
{
    display.clearBuffer();
    display.setDrawColor(1);
    display.setFontMode(1);
    display.setFont(kDisplayFont);
    display.setFontPosTop();
}
}

// Применяет сохранённую яркость напрямую через API контраста u8g2.
void applyDisplayBrightness(uint8_t brightness)
{
#if PROJECT_HAS_SCREEN
    display.setContrast(brightness);
#else
    (void)brightness;
#endif
}

// Инициализирует шину, драйвер OLED и первый пустой кадр.
void initDisplay()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
#if OLED_USE_I2C
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    display.setI2CAddress(static_cast<uint8_t>(OLED_I2C_ADDR << 1));
    display.setBusClock(400000);
#else
    SPI.begin(OLED_CLK_PIN, -1, OLED_MOSI_PIN, OLED_CS_PIN);
#endif

    if (!display.begin())
    {
        Serial.println(F("u8g2 init failed"));
        for (;;)
            ;
    }

    display.setPowerSave(0);
    applyDisplayBrightness((uint8_t)db.get(kk::screenBrightness));
    prepareTextFrame();
    display.sendBuffer();
#endif
}

// Рисует встроенный экран состояния, который показывается при отключённом внешнем экране.
void screenLoop()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
    EEPROM &eeprom = EEPROM::getInstance();

    // В режиме STA показываем IP станции, а в режиме AP — адрес точки доступа.
    const long rssi = WiFi.RSSI();
    const IPAddress ip = isAccessPointMode() ? WiFi.softAPIP() : WiFi.localIP();

    char line[48];
    char rssiText[16];

    prepareTextFrame();

    std::snprintf(line, sizeof(line), "%s", ip.toString().c_str());
    display.setCursor(0, kStatusLine0Y);
    display.print(line);

    std::snprintf(rssiText, sizeof(rssiText), "%ld", rssi);
    display.setCursor(SCREEN_WIDTH - display.getStrWidth(rssiText), kStatusLine0Y);
    display.print(rssiText);

    display.setCursor(0, kStatusLine1Y);
    display.print("TCP: ");
    display.print(isTcpClientConnected() ? "client on" : "wait client");

    std::snprintf(line, sizeof(line), "%ld bps", (long)db.get(kk::Serial2Bitrate));
    display.setCursor(0, kStatusLine2Y);
    display.print(line);

    std::snprintf(line, sizeof(line), "Drop: %lu", (unsigned long)getDroppedNetworkTxBytes());
    display.setCursor(0, kStatusLine3Y);
    display.print(line);

    display.setCursor(0, kStatusLine4Y);
    if (getNetworkTxQueueCapacity() == 0)
    {
        display.print("Q ERR");
    }
    else
    {
        std::snprintf(line, sizeof(line), "TX:%d", eeprom.all_TX_to_network);
        display.print(line);
    }

    display.sendBuffer();
#endif
}
