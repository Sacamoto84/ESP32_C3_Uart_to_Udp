#include "network_internal.h"

#include <ESPmDNS.h>

// Публикует устройство в локальной сети, чтобы к нему можно было обращаться по `<host>.local`.
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

    initialized = true;
    Serial.printf("mDNS ready: %s.local\n", PROJECT_DEVICE_HOSTNAME);
}
