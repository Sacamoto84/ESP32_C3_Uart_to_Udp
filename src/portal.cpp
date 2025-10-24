#include "define.h"
#include <lwip/inet.h>

extern SettingsGyver sett;
extern EEPROM &eeprom;

bool isValidIp(const char *ip)
{
    struct in_addr addr;
    return inet_aton(ip, &addr); // вернёт true, если ip корректный
}

void build(sets::Builder &b)
{
    EEPROM &eeprom = EEPROM::getInstance();

    Serial.println("build");

    String tempIpClient = eeprom.ipClient;

    if (b.Input("Ip Адресс клиента", &eeprom.ipClient))
    {
        Serial.println(b.build.value);
        const char *ip = b.build.value.c_str();

        if (isValidIp(ip))
        {
            Serial.printf("✅ Корректный IP: %s\n", ip);
            db.set(kk::ipClient, ip);
            tempIpClient = ip;
            db.update();
        }
        else
        {
            Serial.printf("❌ Некорректный IP: %s\n", ip);
            eeprom.ipClient = tempIpClient;
            sett.reload(true);
        }
    }

    if (b.Number("Битрейт", &eeprom.Serial2Bitrate, 300, 4000000))
    {
        Serial.println(b.build.value);
        int bits = b.build.value;
        db.set(kk::Serial2Bitrate, bits);
        db.update();
    }

    if (b.Switch("Эхо", &eeprom.echo))
    {
        Serial.println(b.build.value);
        db.set(kk::echo, b.build.value);
        db.update();
    }

    if (b.Switch("Броадкаст", &eeprom.broadcast))
    {
        Serial.println(b.build.value);
        db.set(kk::broadcast, b.build.value);
        db.update();
    }

    {
        sets::Group g(b, "WiFi");
        b.Input(kk::WIFI_SSID, "SSID");
        b.Input(kk::WIFI_PASS, "Password");
    }


    if (b.Switch("Внешний экран по UDP 82 порту", &eeprom.externalScreen))
    {
        Serial.println(b.build.value);
        db.set(kk::externalScreen, b.build.value);
        db.update();
    }

    if (b.Button("Сброс ESP32"))
    {
        ESP.restart();
    }

    if (b.Button("Выход -> Сброс", 0x25b18f))
    {
        Serial.println("✅ Выход -> Сброс");
        pinMode(9, OUTPUT);
        digitalWrite(9, LOW);
        delay(100);
        pinMode(9, OPEN_DRAIN);
        digitalWrite(9, HIGH);
    }
}

