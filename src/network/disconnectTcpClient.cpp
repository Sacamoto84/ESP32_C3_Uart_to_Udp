#include "network_internal.h"

void disconnectTcpClient(const char *reason)
{
    if (reason != nullptr && reason[0] != '\0')
    {
        Serial.println(reason);
    }

    tcpClient.stop();
    tcpClientConnected = false;
}
