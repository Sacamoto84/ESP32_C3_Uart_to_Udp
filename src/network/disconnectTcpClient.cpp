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

    const wifi_mode_t wifiMode = WiFi.getMode();
    const bool apMode = (wifiMode == WIFI_MODE_AP || wifiMode == WIFI_MODE_APSTA);

    if (apMode)
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
