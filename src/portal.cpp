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

struct WifiPowerLabel
{
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

WifiPowerLabel wifiPowerLabels[] = {
    {WIFI_POWER_MINUS_1dBm, "-1 dBm"},
    {WIFI_POWER_2dBm, "2 dBm"},
    {WIFI_POWER_5dBm, "5 dBm"},
    {WIFI_POWER_7dBm, "7 dBm"},
    {WIFI_POWER_8_5dBm, "8.5 dBm"},
    {WIFI_POWER_11dBm, "11 dBm"},
    {WIFI_POWER_13dBm, "13 dBm"},
    {WIFI_POWER_15dBm, "15 dBm"},
    {WIFI_POWER_17dBm, "17 dBm"},
    {WIFI_POWER_18_5dBm, "18.5 dBm"},
    {WIFI_POWER_19dBm, "19 dBm"},
    {WIFI_POWER_19_5dBm, "19.5 dBm"},
    {WIFI_POWER_20dBm, "20 dBm"},
    {WIFI_POWER_20_5dBm, "20.5 dBm"},
    {WIFI_POWER_21dBm, "21 dBm"}};

void addWifiPowerButton(sets::Builder &b, size_t id, const WifiPowerOption &opt, int currentPower)
{
    uint32_t color = (currentPower != opt.dbValue) ? 0x808080 : 0xd55f30;
    if (b.Button(id, opt.label, color))
    {
        Serial.print("Set TX power to ");
        Serial.println(opt.label);
        db.set(kk::wifiPower, opt.dbValue);
        db.update();
        WiFi.setTxPower((wifi_power_t)opt.dbValue);
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
    if (b.build.isAction() && b.build.id == kk::screenBrightness)
    {
        String rawBrightness = b.build.value;
        Serial.print("[Brightness] action received, raw value: ");
        Serial.println(rawBrightness);
    }

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

    if (b.Number(kk::screenBrightness, "Яркость экрана", nullptr, 0, 255))
    {
        int brightness = constrain((int)db.get(kk::screenBrightness), 0, 255);

        Serial.print("[Brightness] number callback, value: ");
        Serial.println(brightness);

        applyDisplayBrightness((uint8_t)brightness);

        bool saved = db.update();
        Serial.print("[Brightness] db.update result: ");
        Serial.println(saved ? "true" : "false");
        Serial.print("[Brightness] applied value: ");
        Serial.println(brightness);
        b.reload();
        sett.reload(true);
    }
    
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
        pinMode(RESET_PULSE_PIN, OUTPUT);
        digitalWrite(RESET_PULSE_PIN, LOW);
        delay(100);
        pinMode(RESET_PULSE_PIN, OPEN_DRAIN);
        digitalWrite(RESET_PULSE_PIN, HIGH);
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
        b.Label("Мощность", WifiCurrentPowerString(WiFi.getTxPower()));
        for (size_t i = 0; i < sizeof(wifiPowerOptions) / sizeof(WifiPowerOption); ++i)
        {
            addWifiPowerButton(b, 1000 + i, wifiPowerOptions[i], currentPower);
        }

    }

    {
        b.Label("Версия " FW_VERSION);
    }
}

String WifiCurrentPowerString(int power)
{
    Serial.print("Current TX Power code: ");
    Serial.println(power);

    for (int i = 0; i < sizeof(wifiPowerLabels) / sizeof(WifiPowerLabel); ++i)
    {
        if (wifiPowerLabels[i].dbValue == power)
        {
            return wifiPowerLabels[i].label;
        }
    }

    return String(power) + " raw";
}
