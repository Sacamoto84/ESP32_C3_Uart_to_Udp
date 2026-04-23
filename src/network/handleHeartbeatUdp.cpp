#include "network_internal.h"

#include <cstdio>
#include <cstring>

namespace
{
constexpr char kHeartbeatPingPrefix[] = "tm3 hb ping";
constexpr char kHeartbeatPongPrefix[] = "tm3 hb pong";
constexpr size_t kHeartbeatBufferSize = 128;
}

// Отвечает на UDP-пакеты `tm3 hb ping` зеркальным ответом `tm3 hb pong`.
void handleHeartbeatUdp()
{
    int packetSize = heartbeatUdp.parsePacket();
    if (!packetSize)
    {
        return;
    }

    char buffer[kHeartbeatBufferSize];
    const int maxReadable = static_cast<int>(sizeof(buffer) - 1);
    const int bytesToRead = packetSize > maxReadable ? maxReadable : packetSize;
    const int bytesRead = heartbeatUdp.read(buffer, bytesToRead);
    if (bytesRead <= 0)
    {
        return;
    }

    buffer[bytesRead] = '\0';

    while (heartbeatUdp.available())
    {
        heartbeatUdp.read();
    }

    if (std::strncmp(buffer, kHeartbeatPingPrefix, std::strlen(kHeartbeatPingPrefix)) != 0)
    {
        return;
    }

    const char *suffix = buffer + std::strlen(kHeartbeatPingPrefix);
    char response[kHeartbeatBufferSize];
    std::snprintf(response, sizeof(response), "%s%s", kHeartbeatPongPrefix, suffix);

    heartbeatUdp.beginPacket(heartbeatUdp.remoteIP(), heartbeatUdp.remotePort());
    heartbeatUdp.write(reinterpret_cast<const uint8_t *>(response), std::strlen(response));
    heartbeatUdp.endPacket();
}
