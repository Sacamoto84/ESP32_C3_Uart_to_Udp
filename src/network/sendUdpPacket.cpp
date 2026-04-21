#include "network_internal.h"
#include "status_led.h"

// Общая helper-функция отправки UDP-пакета.
// В текущей архитектуре используется только для broadcast и внешнего экрана.
bool sendUdpPacket(const char *tag, const char *payload, int len, const sockaddr_in &dest, bool enableBroadcast)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
        Serial.printf("%s: socket creation failed\n", tag);
        return false;
    }

    if (enableBroadcast)
    {
        int broadcastEnable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
        {
            Serial.printf("%s: setsockopt failed\n", tag);
            close(sock);
            return false;
        }
    }

    int sent = sendto(sock, payload, len, 0, (const struct sockaddr *)&dest, sizeof(dest));
    if (sent < 0)
    {
        Serial.printf("%s: sendto failed\n", tag);
        close(sock);
        return false;
    }

    sendStatusLedCommand(StatusLedCommand::PulseNetworkActivity);
    close(sock);
    return true;
}
