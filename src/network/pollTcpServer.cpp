#include "network_internal.h"

void pollTcpServer()
{
    if (tcpClient && !tcpClient.connected())
    {
        Serial.println("pollTcpServer: TCP client disconnected");
        tcpClient.stop();
        tcpClientConnected = false;
    }

    if (!tcpServer.hasClient())
    {
        return;
    }

    WiFiClient newClient = tcpServer.accept();
    if (!newClient)
    {
        return;
    }

    if (tcpClient && tcpClient.connected())
    {
        Serial.printf("pollTcpServer: rejecting extra client %s:%u\n",
                      newClient.remoteIP().toString().c_str(),
                      newClient.remotePort());
        newClient.stop();
        return;
    }

    newClient.setNoDelay(true);
    newClient.setTimeout(2000);

    tcpClient = newClient;
    tcpClientConnected = true;

    Serial.printf("pollTcpServer: TCP client connected from %s:%u\n",
                  tcpClient.remoteIP().toString().c_str(),
                  tcpClient.remotePort());
}
