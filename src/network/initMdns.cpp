#include "network_internal.h"

#include <ESPmDNS.h>

void initMdns()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    if (!MDNS.begin(PROJECT_DEVICE_HOSTNAME))
    {
        Serial.printf("mDNS start failed for %s.local\n", PROJECT_DEVICE_HOSTNAME);
        return;
    }

    MDNS.setInstanceName(PROJECT_DEVICE_HOSTNAME);

    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "path", "/");
    MDNS.addServiceTxt("http", "tcp", "board", BOARD_LABEL);

    MDNS.addService("uartbridge", "tcp", kNetworkDataPort);
    MDNS.addServiceTxt("uartbridge", "tcp", "transport", "tcp-server");
    MDNS.addServiceTxt("uartbridge", "tcp", "board", BOARD_LABEL);

#if PROJECT_HAS_SCREEN
    if (db.get(kk::externalScreen))
    {
        MDNS.addService("external-screen", "udp", 82);
        MDNS.addServiceTxt("external-screen", "udp", "role", "oled-framebuffer");
    }
#endif

    initialized = true;
    Serial.printf("mDNS ready: %s.local\n", PROJECT_DEVICE_HOSTNAME);
}
