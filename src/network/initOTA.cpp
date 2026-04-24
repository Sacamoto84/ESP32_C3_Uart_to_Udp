#include "network_internal.h"

#include <ArduinoOTA.h>
#include <ESPmDNS.h>

namespace
{
// Преобразует текущий OTA-тип операции в читаемую строку для лога.
const char *otaCommandName()
{
    return (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
}
}

// Настраивает ArduinoOTA и гарантирует закрытие активных TCP-клиентов во время обновления.
void initOTA()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    ArduinoOTA.setPort(PROJECT_OTA_PORT);
    ArduinoOTA.setHostname(PROJECT_DEVICE_HOSTNAME);
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.setRebootOnSuccess(true);
    ArduinoOTA.setTimeout(10000);

    if (PROJECT_OTA_PASSWORD_VALUE[0] != '\0')
    {
        ArduinoOTA.setPassword(PROJECT_OTA_PASSWORD_VALUE);
    }

    ArduinoOTA.onStart([]() {
        Serial.printf("OTA start: %s\n", otaCommandName());
        disconnectTcpClient("OTA start: closing TCP client");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("OTA end");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint8_t lastLoggedPercent = 255;

        if (total == 0)
        {
            return;
        }

        const uint8_t percent = (uint8_t)((progress * 100U) / total);
        if (percent == lastLoggedPercent || (percent % 10U) != 0U)
        {
            return;
        }

        lastLoggedPercent = percent;
        Serial.printf("OTA progress: %u%%\n", percent);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA error[%u]\n", (unsigned)error);
    });

    ArduinoOTA.begin();
    MDNS.enableArduino(PROJECT_OTA_PORT, PROJECT_OTA_PASSWORD_VALUE[0] != '\0');

    initialized = true;
    Serial.printf("OTA ready: %s.local:%u\n", PROJECT_DEVICE_HOSTNAME, (unsigned)PROJECT_OTA_PORT);
}
