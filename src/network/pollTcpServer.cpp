#include "network_internal.h"
#include "status_led.h"

void pollTcpServer()
{
    refreshTcpClientConnectionState("pollTcpServer: TCP client disconnected");

    if (!tcpServer.hasClient())
    {
        return;
    }

    WiFiClient newClient = tcpServer.accept();
    if (!newClient)
    {
        return;
    }

    if (refreshTcpClientConnectionState("pollTcpServer: TCP client disconnected"))
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
    sendStatusLedCommand(StatusLedCommand::ClientConnected);

    Serial.printf("pollTcpServer: TCP client connected from %s:%u\n",
                  tcpClient.remoteIP().toString().c_str(),
                  tcpClient.remotePort());
}
