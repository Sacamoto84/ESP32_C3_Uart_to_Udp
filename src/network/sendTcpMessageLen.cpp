#include "network_internal.h"

// Отправка заданного количества байт в надёжный TCP-канал.
void sendTcpMessageLen(const char *msg, int len, const char *ip)
{
    sendTcpPacket("sendTcpMessageLen", (const uint8_t *)msg, (size_t)len, ip, kNetworkDataPort);
}
