#include "network_internal.h"
#include "status_led.h"

// Пишет один чанк из очереди UART в активного TCP-клиента с защитой по таймауту.
bool sendTcpChunk(const uint8_t *payload, size_t len)
{
    if (payload == nullptr || len == 0)
    {
        return true;
    }

    if (!refreshTcpClientConnectionState("sendTcpChunk: TCP client disconnected before write"))
    {
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

        if (!refreshTcpClientConnectionState("sendTcpChunk: TCP client disconnected during write"))
        {
            return false;
        }

        if (millis() - lastProgressAt > kTcpWriteTimeoutMs)
        {
            Serial.printf("sendTcpChunk: write timeout after %u ms\n", kTcpWriteTimeoutMs);
            disconnectTcpClient(nullptr);
            return false;
        }

        delay(1);
    }

    sendStatusLedCommand(StatusLedCommand::PulseNetworkActivity);
    return true;
}
