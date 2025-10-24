#include "define.h"
#include <lwip/inet.h>
#include <esp_wifi.h>

extern SettingsGyver sett;
extern EEPROM &eeprom;

bool isValidIp(const char *ip)
{
    struct in_addr addr;
    return inet_aton(ip, &addr); // вернёт true, если ip корректный
}

String WifiCurrentPowerString(int);

void build(sets::Builder &b)
{
    EEPROM &eeprom = EEPROM::getInstance();

    Serial.println("build");

    String tempIpClient = eeprom.ipClient;

    if (b.Input("Ip Адрес клиента", &eeprom.ipClient))
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

    {
        b.Label("Версия 1.5.1");
    }

    {
        sets::Menu m(b, "Мощность Wifi");

        String power;
        if (b.enterMenu())
        {
            Serial.println("menu 1");
            Serial.print("Current TX Power: ");

            power = WifiCurrentPowerString(eeprom.wifiPower);
        }

        b.Label(power);

        {
            sets::Buttons g(b);

            if (b.Button("-1dbm"))
            {
                Serial.println("WIFI_POWER_MINUS_1dBm");
                db.set(kk::wifiPower, WIFI_POWER_MINUS_1dBm);
                db.update();
            }

            if (b.Button("2dBm"))
            {
                Serial.println("WIFI_POWER_2dBm");
                db.set(kk::wifiPower, WIFI_POWER_2dBm);
                db.update();
            }
            if (b.Button("8.5dBm"))
            {
                Serial.println("WIFI_POWER_8_5dBm");
                db.set(kk::wifiPower, WIFI_POWER_8_5dBm);
                db.update();
            }
        }
        {
            sets::Buttons g(b);
            if (b.Button("11dBm"))
            {
                Serial.println("WIFI_POWER_11dBm");
                db.set(kk::wifiPower, WIFI_POWER_11dBm);
                db.update();
            }
            if (b.Button("13dBm"))
            {
                Serial.println("WIFI_POWER_13dBm");
                db.set(kk::wifiPower, WIFI_POWER_13dBm);
                db.update();
            }
            if (b.Button("15dBm"))
            {
                Serial.println("WIFI_POWER_15dBm");
                db.set(kk::wifiPower, WIFI_POWER_15dBm);
                db.update();
            }
        }
        {
            sets::Buttons g(b);

            if (b.Button("17dBm"))
            {
                Serial.println("WIFI_POWER_17dBm");
                db.set(kk::wifiPower, WIFI_POWER_17dBm);
                db.update();
            }
            if (b.Button("19dBm"))
            {
                Serial.println("WIFI_POWER_19dBm");
                db.set(kk::wifiPower, WIFI_POWER_19dBm);
                db.update();
            }
            if (b.Button("19.5dBm"))
            {
                Serial.println("WIFI_POWER_19_5dBm");
                db.set(kk::wifiPower, WIFI_POWER_19_5dBm);
                db.update();
            }
        }
    }
}

String WifiCurrentPowerString(int power)
{
    Serial.print("Current TX Power: ");
    String  res;
    switch (power)
    {
    case WIFI_POWER_MINUS_1dBm:
        Serial.println("-1 dBm");
        res = "-1 dBm";
        break;
    case WIFI_POWER_2dBm:
        Serial.println("2 dBm");
        res = "2 dBm";
        break;
    case WIFI_POWER_8_5dBm:
        Serial.println("8.5 dBm");
        res = "8.5 dBm";
        break;

    case WIFI_POWER_11dBm:
        Serial.println("11 dBm");
        res = "11 dBm";
        break;

    case WIFI_POWER_13dBm:
        Serial.println("13 dBm");
        res = "13 dBm";
        break;

    case WIFI_POWER_15dBm:
        Serial.println("15 dBm");
        res = "15 dBm";
        break;

    case WIFI_POWER_17dBm:
        Serial.println("17 dBm");
        res = "17 dBm";
        break;

    case WIFI_POWER_19dBm:
        Serial.println("19 dBm");
        res = "19 dBm";
        break;

    case WIFI_POWER_19_5dBm:
        Serial.println("19.5 dBm");
        res = "19.5 dBm";
        break;

    default:
        Serial.print("Unknown code: ");
        Serial.println((int)power);
        res = "Unknown code: ";
        break;
    }

    return res;
}
