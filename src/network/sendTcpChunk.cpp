#include "network_internal.h"
#include "status_led.h"

bool sendTcpChunk(const uint8_t *payload, size_t len)
{
    if (payload == nullptr || len == 0)
    {
        return true;
    }

    if (!(tcpClient && tcpClient.connected()))
    {
        tcpClientConnected = false;
        return false;
    }

    size_t sentTotal = 0;
    uint32_t lastProgressAt = millis();

    while (sentTotal < len)
    {
        const size_t sent = tcpClient.write(payload + sentTotal, len - sentTotal);
        if (sent > 0)
        {
            sentTotal += sent;
            lastProgressAt = millis();
            continue;
        }

        if (!(tcpClient && tcpClient.connected()))
        {
            Serial.println("sendTcpChunk: TCP client disconnected during write");
            tcpClient.stop();
            tcpClientConnected = false;
            return false;
        }

        if (millis() - lastProgressAt > kTcpWriteTimeoutMs)
        {
            Serial.printf("sendTcpChunk: write timeout after %u ms\n", kTcpWriteTimeoutMs);
            tcpClient.stop();
            tcpClientConnected = false;
            return false;
        }

        delay(1);
    }

    sendStatusLedCommand(StatusLedCommand::PulseNetworkActivity);
    return true;
}
