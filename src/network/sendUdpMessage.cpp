#include "network_internal.h"

// Отправка null-terminated строки на указанный IP.
void sendUdpMessage(const char *msg, const char *ip)
{
    sendUdpMessageLen(msg, strlen(msg), ip);
}
