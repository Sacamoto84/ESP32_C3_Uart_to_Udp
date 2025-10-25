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

extern void screenLoop();

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

    if (db.get(kk::externalScreen))
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
    }else
    {
        screenLoop();
    }
}



