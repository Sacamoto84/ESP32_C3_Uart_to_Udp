#include "network_internal.h"
#include "status_led.h"

namespace
{
bool loadStaticIpConfig(IPAddress &localIp, IPAddress &gateway, IPAddress &subnet)
{
    String localIpString = db.get(kk::staticIp);
    String gatewayString = db.get(kk::staticGateway);
    String subnetString = db.get(kk::staticSubnet);

    const bool ok = localIp.fromString(localIpString) &&
                    gateway.fromString(gatewayString) &&
                    subnet.fromString(subnetString);

    if (!ok)
    {
        Serial.println("Static IP enabled, but one of the IP fields is invalid. Falling back to DHCP");
    }

    return ok;
}

#if PROJECT_HAS_SCREEN
constexpr const uint8_t *kStartupDisplayFont = u8g2_font_7x13_tr;

void beginStartupScreenFrame()
{
    display.clearBuffer();
    display.setDrawColor(1);
    display.setFontMode(1);
    display.setFont(kStartupDisplayFont);
    display.setFontPosTop();
}

void drawDotProgress(uint8_t y, int count)
{
    if (count <= 0)
    {
        return;
    }

    display.setCursor(0, y);
    for (int i = 0; i < count && i < 20; ++i)
    {
        display.print('.');
    }
}

void renderWiFiStartupScreen(const char *title, const char *subtitle, int dots, const char *message)
{
    beginStartupScreenFrame();

    if (title != nullptr && title[0] != '\0')
    {
        display.setCursor(0, 0);
        display.print(title);
    }

    if (subtitle != nullptr && subtitle[0] != '\0')
    {
        display.setCursor(0, 12);
        display.print(subtitle);
    }

    drawDotProgress(24, dots);

    if (message != nullptr && message[0] != '\0')
    {
        display.setCursor(0, 36);
        display.print(message);
    }

    drawStartupVersionFooter();
    display.sendBuffer();
}
#endif
} // namespace

void initWiFi()
{
    int count = 0;
    bool needAP = false;

    // Lower CPU clock before Wi-Fi start to reduce peak power draw.
    setCpuFrequencyMhz(80);

    WiFi.mode(WIFI_STA);

    if (db.get(kk::useStaticIp))
    {
        IPAddress localIp;
        IPAddress gateway;
        IPAddress subnet;

        if (loadStaticIpConfig(localIp, gateway, subnet))
        {
            if (WiFi.config(localIp, gateway, subnet))
            {
                Serial.print("Static IP enabled: ");
                Serial.print(localIp);
                Serial.print(" gateway ");
                Serial.print(gateway);
                Serial.print(" subnet ");
                Serial.println(subnet);
            }
            else
            {
                Serial.println("WiFi.config failed, falling back to DHCP");
            }
        }
    }

    // Tx power comes from settings and can later be lowered to 8.5 dBm on retry.
    const int power = db.get(kk::wifiPower);
    WiFi.setTxPower((wifi_power_t)power);
    WiFi.begin(db.get(kk::WIFI_SSID), db.get(kk::WIFI_PASS));
    sendStatusLedCommand(StatusLedCommand::ConnectingToStation);

#if PROJECT_HAS_SCREEN
    {
        const String powerLabel = WifiCurrentPowerString(WiFi.getTxPower());
        renderWiFiStartupScreen("Connecting", powerLabel.c_str(), 0, nullptr);
    }
#endif

    while (WiFi.status() != WL_CONNECTED)
    {
#if PROJECT_HAS_SCREEN
        {
            const String powerLabel = WifiCurrentPowerString(WiFi.getTxPower());
            renderWiFiStartupScreen("Connecting", powerLabel.c_str(), count + 1, nullptr);
        }
#endif
        Serial.print(db.get(kk::wifiPower));
        Serial.print(".");
        delay(500);
        count++;

        if (count <= 20)
        {
            continue;
        }

#if PROJECT_HAS_SCREEN
        renderWiFiStartupScreen("Connecting", "8.5 dBm", 0, "Retry power");
#endif
        Serial.println("\nWifi STA not found");
        Serial.println("Lowering power to 8.5dBm");

        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        WiFi.begin(db.get(kk::WIFI_SSID), db.get(kk::WIFI_PASS));
        count = 0;

        while (WiFi.status() != WL_CONNECTED)
        {
#if PROJECT_HAS_SCREEN
            renderWiFiStartupScreen("Connecting", "8.5 dBm", count + 1, "Retry power");
#endif
            Serial.print(".");
            delay(500);
            count++;

            if (count <= 20)
            {
                continue;
            }

            Serial.println("Wifi STA not found");
            needAP = true;
            break;
        }

        break;
    }

    if (needAP)
    {
#if PROJECT_HAS_SCREEN
        renderWiFiStartupScreen("Starting AP", "AP ESP32", 0, nullptr);
#endif
        Serial.println("Starting access point");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("AP ESP32");
        sendStatusLedCommand(StatusLedCommand::AccessPoint);

        if (AP_MODE_PIN >= 0
#if PROJECT_HAS_BOARD_LED
            && AP_MODE_PIN != STATUS_LED_BOARD_PIN
#endif
        )
        {
            pinMode(AP_MODE_PIN, OUTPUT);
            digitalWrite(AP_MODE_PIN, LOW);
        }
    }
    else
    {
        sendStatusLedCommand(StatusLedCommand::WaitingForClient);
    }

#if PROJECT_HAS_SCREEN
    {
        const String connectedHost = (needAP ? WiFi.softAPIP() : WiFi.localIP()).toString();
        renderWiFiStartupScreen("Connected", connectedHost.c_str(), 0, nullptr);
    }
#endif

    Serial.println();
    Serial.print("Connected: ");
    Serial.println(needAP ? WiFi.softAPIP() : WiFi.localIP());
}
