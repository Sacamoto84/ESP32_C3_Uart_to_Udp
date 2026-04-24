#include "hardware.h"
#include "network_internal.h"
#include "status_led.h"

// Закрывает активного клиента TCP 8888 и восстанавливает LED-состояние под текущий Wi-Fi режим.
void disconnectTcpClient(const char *reason)
{
    if (reason != nullptr && reason[0] != '\0')
    {
        Serial.println(reason);
    }

    tcpClient.stop();
    tcpClientConnected = false;

    if (isAccessPointMode())
    {
        sendStatusLedCommand(StatusLedCommand::AccessPoint);
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        sendStatusLedCommand(StatusLedCommand::WaitingForClient);
    }
    else
    {
        sendStatusLedCommand(StatusLedCommand::Off);
    }
}
