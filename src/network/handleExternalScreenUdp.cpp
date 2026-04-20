#include "network_internal.h"

// Обработка входящих UDP-пакетов для внешнего экрана.
// Ожидаем ровно 1024 байта - это размер framebuffer для SSD1306 128x64.
void handleExternalScreenUdp()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
    int packetSize = udp.parsePacket();
    if (!packetSize)
    {
        return;
    }

    Serial.printf("UDP packet received: size %d from %s\n", packetSize, udp.remoteIP().toString().c_str());
    if (packetSize == 1024)
    {
        int bytesRead = udp.read(display.getBuffer(), 1024);
        if (bytesRead == 1024)
        {
            display.display();
            Serial.println("Display updated with 1024 bytes");
        }
        else
        {
            Serial.printf("Error: read only %d bytes, expected 1024\n", bytesRead);
        }
        return;
    }

    // Очистка буфера, если пакет некорректного размера.
    Serial.printf("Error: packet size %d bytes, expected 1024\n", packetSize);

    int discarded = 0;
    while (udp.available())
    {
        udp.read();
        discarded++;
    }
    Serial.printf("Discarded %d bytes from invalid packet\n", discarded);
#endif
}
