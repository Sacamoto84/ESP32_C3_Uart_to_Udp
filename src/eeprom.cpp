#include "eeprom.h"

#include <LittleFS.h>
#include <WiFi.h>

// Возвращает единственный экземпляр EEPROM-хелпера, используемый прошивкой.
EEPROM &EEPROM::getInstance()
{
    static EEPROM instance;
    return instance;
}

// Заполняет базу начальными значениями, если нужных ключей ещё нет.
EEPROM::EEPROM()
{
    db.begin();

    db.init(kk::ipClient, "192.168.0.100");
    db.init(kk::echo, true);
    db.init(kk::timeout, 1000);
    db.init(kk::Serial2Bitrate, 9800);
    db.init(kk::WIFI_SSID, "TP-Link_BC0C");
    db.init(kk::WIFI_PASS, "58133514");
    db.init(kk::externalScreen, false);
    db.init(kk::useTcpTransport, false);
    db.init(kk::useStaticIp, false);
    db.init(kk::staticIp, "192.168.0.222");
    db.init(kk::staticGateway, "192.168.0.1");
    db.init(kk::staticSubnet, "255.255.255.0");
    db.init(kk::wifiPower, WIFI_POWER_8_5dBm);
    db.init(kk::screenBrightness, 207);
    db.init(kk::networkTxQueueLength, kDefaultNetworkTxQueueLength);
    db.init(kk::serialRxBufferKb, kDefaultSerialRxBufferKb);
    db.init(kk::statusLedBrightness, kDefaultStatusLedBrightness);
    db.init(kk::statusLedEnabled, kDefaultStatusLedEnabled);
    db.init(kk::statusLedActiveLow, kDefaultStatusLedActiveLow);
}
