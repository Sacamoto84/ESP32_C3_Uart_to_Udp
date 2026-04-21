#include "network_internal.h"

namespace
{
IPAddress currentTcpTarget;
bool hasCurrentTcpTarget = false;
} // namespace

// Поддерживаем одно persistent TCP-подключение к одному приёмнику.
// Если IP в настройках поменялся или сокет умер, переподключаемся.
bool ensureTcpConnection(const char *ip, uint16_t port)
{
    IPAddress requestedTarget;
    if (!requestedTarget.fromString(ip))
    {
        Serial.printf("ensureTcpConnection: invalid IP %s\n", ip ? ip : "(null)");
        return false;
    }

    if (tcpClient.connected() && hasCurrentTcpTarget && currentTcpTarget == requestedTarget)
    {
        return true;
    }

    if (tcpClient.connected())
    {
        tcpClient.stop();
    }

    tcpClient.setNoDelay(true);
    tcpClient.setTimeout(2000);

    Serial.printf("TCP connect to %s:%u\n", ip, port);
    if (!tcpClient.connect(requestedTarget, port))
    {
        Serial.printf("TCP connect failed: %s:%u\n", ip, port);
        hasCurrentTcpTarget = false;
        return false;
    }

    currentTcpTarget = requestedTarget;
    hasCurrentTcpTarget = true;
    tcpClient.setNoDelay(true);
    return true;
}
