#include "network_internal.h"
#include "status_led.h"

namespace
{
// Баннер не относится к потоку UART. Отправляем его после подключения, чтобы
// он гарантированно дошёл до клиента, а не был очищен вместе с UART-очередью.
constexpr char kTcpConnectionBanner[] = "UART to TCP server " BOARD_LABEL " V" FW_VERSION "\n";
}

// Принимает основного клиента TCP 8888 и отклоняет параллельные лишние подключения.
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
    newClient.setTimeout(kTcpStreamClientTimeoutMs);

    tcpClient = newClient;
    tcpClientConnected = true;
    sendStatusLedCommand(StatusLedCommand::ClientConnected);

    Serial.printf("pollTcpServer: TCP client connected from %s:%u\n",
                  tcpClient.remoteIP().toString().c_str(),
                  tcpClient.remotePort());

    if (!sendTcpChunk(reinterpret_cast<const uint8_t *>(kTcpConnectionBanner),
                      sizeof(kTcpConnectionBanner) - 1))
    {
        Serial.println("pollTcpServer: failed to send connection banner");
    }
}
