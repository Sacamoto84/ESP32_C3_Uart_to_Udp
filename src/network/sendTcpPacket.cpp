#include "network_internal.h"
#include "status_led.h"

namespace
{
constexpr uint32_t kTcpWriteTimeoutMs = 3000;
}

// Надёжная отправка в один TCP-поток.
// Порядок байт сохраняется стеком TCP, а при частичной записи дожимаем остаток циклом.
bool sendTcpPacket(const char *tag, const uint8_t *payload, size_t len, const char *ip, uint16_t port)
{
    if (!ensureTcpConnection(ip, port))
    {
        return false;
    }

    size_t sentTotal = 0;
    uint32_t lastProgressAt = millis();

    while (sentTotal < len)
    {
        size_t sent = tcpClient.write(payload + sentTotal, len - sentTotal);
        if (sent > 0)
        {
            sentTotal += sent;
            lastProgressAt = millis();
            continue;
        }

        if (!tcpClient.connected())
        {
            Serial.printf("%s: TCP disconnected, reconnecting\n", tag);
            tcpClient.stop();
            if (!ensureTcpConnection(ip, port))
            {
                return false;
            }
            continue;
        }

        if (millis() - lastProgressAt > kTcpWriteTimeoutMs)
        {
            Serial.printf("%s: TCP write timeout after %u ms\n", tag, kTcpWriteTimeoutMs);
            tcpClient.stop();
            return false;
        }

        delay(1);
    }

    Serial.printf("%s: sent %u bytes over TCP\n", tag, (unsigned)sentTotal);
    sendStatusLedCommand(StatusLedCommand::PulseNetworkActivity);
    return true;
}
