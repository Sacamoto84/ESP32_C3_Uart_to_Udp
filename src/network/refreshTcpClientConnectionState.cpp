#include "network_internal.h"

bool refreshTcpClientConnectionState(const char *disconnectReason)
{
    // Важно: не используем шаблон `tcpClient && !tcpClient.connected()`.
    // У NetworkClient оператор bool() внутри сам вызывает connected(), и из-за
    // короткого замыкания можно пропустить разрыв и оставить stale-состояние.
    if (tcpClient.connected())
    {
        tcpClientConnected = true;
        return true;
    }

    if (tcpClientConnected)
    {
        disconnectTcpClient(disconnectReason);
    }

    return false;
}
