#include "network_internal.h"

// Отправка заданного количества байт на конкретный IP.
void sendUdpMessageLen(const char *msg, int len, const char *ip)
{
    sockaddr_in dest = {};
    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(kNetworkDataPort);

    sendUdpPacket("sendUdpMessageLen", msg, len, dest, false);
}
