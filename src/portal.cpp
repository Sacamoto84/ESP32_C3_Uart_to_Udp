#include "define.h"
#include <lwip/inet.h>
#include <esp_wifi.h>

extern SettingsGyver sett;
extern EEPROM &eeprom;

struct WifiPowerOption
{
    wifi_power_t powerEnum;
    int dbValue;
    const char *label;
};

WifiPowerOption wifiPowerOptions[] = {
    {WIFI_POWER_MINUS_1dBm, -4, "-1dbm (-4)"},
    {WIFI_POWER_2dBm, 8, "2dBm (8)"},
    {WIFI_POWER_8_5dBm, 34, "8.5dBm (34)"},
    {WIFI_POWER_11dBm, 44, "11dBm (44)"},
    {WIFI_POWER_13dBm, 52, "13dBm (52)"},
    {WIFI_POWER_15dBm, 60, "15dBm (60)"},
    {WIFI_POWER_17dBm, 68, "17dBm (68)"},
    {WIFI_POWER_19dBm, 76, "19dBm (76)"},
    {WIFI_POWER_19_5dBm, 78, "19.5dBm (78)"}};

void addWifiPowerButton(sets::Builder &b, const WifiPowerOption &opt, int currentPower)
{
    uint32_t color = (currentPower != opt.dbValue) ? 0x808080 : 0xd55f30;
    sets::Buttons g(b);
    if (b.Button(opt.label, color))
    {
        Serial.println(opt.label);
        db.set(kk::wifiPower, opt.dbValue);
        db.update();
        b.reload();
    }
}

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

    String tempIpClient = db.get(kk::ipClient);
    if (b.Input("Ip Адрес клиента", &tempIpClient))
    {
        Serial.println(b.build.value);
        const char *ip = b.build.value.c_str();
        if (isValidIp(ip))
        {
            Serial.printf("✅ Корректный IP: %s\n", ip);
            db.set(kk::ipClient, ip);
            db.update();
            sett.reload(true);
        }
    }

    b.Number(kk::Serial2Bitrate, "Битрейт", nullptr, 300, 4000000);
    b.Switch(kk::broadcast, "Броадкаст");
    b.Switch(kk::echo, "Эхо");
    
    {
        sets::Group g(b, "WiFi");
        b.Input(kk::WIFI_SSID, "SSID");
        b.Input(kk::WIFI_PASS, "Password");
    }

    b.Switch(kk::externalScreen, "Внешний экран по UDP 82 порту");

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

    if (b.Button("Очистка базы", 0x25b18f))
    {
        Serial.println("Очистка базы");
        db.clear();
        db.update();
    }

    {
        sets::Menu m(b, "Мощность Wifi");
        b.enterMenu();
        int currentPower = db.get(kk::wifiPower);
        b.Label("Мощность", WifiCurrentPowerString(db.get(kk::wifiPower)));
        for (int i = 0; i < sizeof(wifiPowerOptions) / sizeof(WifiPowerOption); ++i)
        {
            addWifiPowerButton(b, wifiPowerOptions[i], currentPower);
        }

    }

    {
        b.Label("Версия 1.5.5");
    }
}

String WifiCurrentPowerString(int power)
{
    Serial.print("Current TX Power: ");
    String res = "???";
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
