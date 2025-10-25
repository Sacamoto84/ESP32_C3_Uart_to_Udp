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

    b.Number(kk::Serial2Bitrate, "Битрейт",nullptr ,300, 4000000);
    b.Switch(kk::echo, "Эхо");
    b.Switch(kk::broadcast, "Броадкаст");
  
    {
        sets::Group g(b, "WiFi");
        b.Input(kk::WIFI_SSID, "SSID");
        b.Input(kk::WIFI_PASS, "Password");
    }

    b.Switch(kk::externalScreen,"Внешний экран по UDP 82 порту");
   
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
        b.Label("Версия 1.5.1");
    }

    {
        sets::Menu m(b, "Мощность Wifi");

        String power;
        if (b.enterMenu())
        {
            // Serial.println("menu 1");
            // power = WifiCurrentPowerString(eeprom.wifiPower);
            // Serial.printf("Current TX Power: %s\n", power);
        }

        int poer = db.get(kk::wifiPower);

        b.Label("Мощность", WifiCurrentPowerString(db.get(kk::wifiPower)));

        {
            sets::Buttons g(b);

            uint32_t color0;
            uint32_t color1;
            uint32_t color2;
            
            if (poer != WIFI_POWER_MINUS_1dBm) color0 = 0x808080; else color0 = 0xd55f30;
            if (poer != WIFI_POWER_2dBm) color1 = 0x808080; else color1 = 0xd55f30;
            if (poer != WIFI_POWER_8_5dBm) color2 = 0x808080; else color2 = 0xd55f30;
            
            if (b.Button("-1dbm (-4)", color0))
            {
                Serial.println("WIFI_POWER_MINUS_1dBm");
                db.set(kk::wifiPower, -4);
                db.update();
                power = WifiCurrentPowerString(-4);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)-4); 
            }

            if (b.Button("2dBm (8)", color1))
            {
                Serial.println("WIFI_POWER_2dBm");
                db.set(kk::wifiPower, 8);
                db.update();
                power = WifiCurrentPowerString(8);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)8); 
            }
            if (b.Button("8.5dBm (34)", color2))
            {
                Serial.println("WIFI_POWER_8_5dBm");
                db.set(kk::wifiPower, 34);
                db.update();
                power = WifiCurrentPowerString(34);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)34); 
            }
        }
        {
            uint32_t color3;
            uint32_t color4;
            uint32_t color5;

            if (poer != WIFI_POWER_11dBm) color3 = 0x808080; else color3 = 0xd55f30;
            if (poer != WIFI_POWER_13dBm) color4 = 0x808080; else color4 = 0xd55f30;
            if (poer != WIFI_POWER_15dBm) color5 = 0x808080; else color5 = 0xd55f30;

            sets::Buttons g(b);
            if (b.Button("11dBm (44)", color3))
            {
                Serial.println("WIFI_POWER_11dBm");
                db.set(kk::wifiPower, 44);
                db.update();
                power = WifiCurrentPowerString(44);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)44); 
            }
            if (b.Button("13dBm (60)", color4))
            {
                Serial.println("WIFI_POWER_13dBm");
                db.set(kk::wifiPower, 52);
                db.update();
                power = WifiCurrentPowerString(52);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)52); 
            }
            if (b.Button("15dBm (60)", color5))
            {
                Serial.println("WIFI_POWER_15dBm");
                db.set(kk::wifiPower, 60);
                db.update();
                power = WifiCurrentPowerString(60);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)60); 
            }
        }
        {
            uint32_t color6;
            uint32_t color7;
            uint32_t color8;
            
            if (poer != WIFI_POWER_17dBm) color6 = 0x808080; else color6 = 0xd55f30;
            if (poer != WIFI_POWER_19dBm) color7 = 0x808080; else color7 = 0xd55f30;
            if (poer != WIFI_POWER_19_5dBm) color8 = 0x808080; else color8 = 0xd55f30;

            sets::Buttons g(b);
        
            if (b.Button("17dBm (68)", color6))
            {
                Serial.println("WIFI_POWER_17dBm");
                db.set(kk::wifiPower, 68);
                db.update();
                power = WifiCurrentPowerString(68);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)68); 
            }
            if (b.Button("19dBm (76)", color7))
            {
                Serial.println("WIFI_POWER_19dBm");
                db.set(kk::wifiPower, 76);
                db.update();
                power = WifiCurrentPowerString(76);
                b.reload();
            }
            if (b.Button("19.5dBm (78)", color8))
            {
                Serial.println("WIFI_POWER_19_5dBm");
                db.set(kk::wifiPower, 78);
                db.update();
                power = WifiCurrentPowerString(78);
                b.reload();
                //WiFi.setTxPower((wifi_power_t)78); 
            }
        }
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
