#include "network_internal.h"

namespace
{
constexpr int kExternalScreenFrameSize = SCREEN_WIDTH * SCREEN_HEIGHT / 8;
}

// Принимает сырой буфер кадра SSD1306 по UDP и копирует его прямо в буфер OLED.
void handleExternalScreenUdp()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
    const int packetSize = udp.parsePacket();
    if (!packetSize)
    {
        return;
    }

    Serial.printf("UDP packet received: size %d from %s\n", packetSize, udp.remoteIP().toString().c_str());
    if (packetSize == kExternalScreenFrameSize)
    {
        const int bytesRead = udp.read(display.getBufferPtr(), kExternalScreenFrameSize);
        if (bytesRead == kExternalScreenFrameSize)
        {
            display.sendBuffer();
            Serial.println("Display updated with raw framebuffer");
        }
        else
        {
            Serial.printf("Error: read only %d bytes, expected %d\n", bytesRead, kExternalScreenFrameSize);
        }
        return;
    }

    Serial.printf("Error: packet size %d bytes, expected %d\n", packetSize, kExternalScreenFrameSize);

    int discarded = 0;
    while (udp.available())
    {
        udp.read();
        discarded++;
    }
    Serial.printf("Discarded %d bytes from invalid packet\n", discarded);
#endif
}
