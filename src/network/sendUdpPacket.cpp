#include "network_internal.h"

// Общая функция отправки UDP-пакета.
// Нужна, чтобы не дублировать создание сокета и обработку ошибок
// в sendUdpMessageLen() и sendUdpBroadcast().
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

    Serial.printf("%s: sent %d bytes\n", tag, sent);
    close(sock);
    return true;
}
