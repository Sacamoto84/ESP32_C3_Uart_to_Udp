#include "network_internal.h"

// Отправка null-terminated строки в TCP-поток на указанный IP.
void sendTcpMessage(const char *msg, const char *ip)
{
    sendTcpMessageLen(msg, strlen(msg), ip);
}
