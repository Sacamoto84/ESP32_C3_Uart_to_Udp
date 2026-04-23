#include "network_internal.h"

// Обновляет кэшированное состояние потокового клиента без опоры на короткое замыкание bool().
bool refreshTcpClientConnectionState(const char *disconnectReason)
{
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
