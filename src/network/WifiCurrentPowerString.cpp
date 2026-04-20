#include "network_internal.h"

namespace
{
// Таблица для красивого отображения текущей мощности Wi-Fi.
struct WifiPowerLabel
{
    int dbValue;
    const char *label;
};

constexpr WifiPowerLabel kWifiPowerLabels[] = {
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
    {WIFI_POWER_21dBm, "21 dBm"},
};
} // namespace

String WifiCurrentPowerString(int power)
{
    Serial.print("Current TX Power code: ");
    Serial.println(power);

    for (const auto &label : kWifiPowerLabels)
    {
        if (label.dbValue == power)
        {
            return label.label;
        }
    }

    return String(power) + " raw";
}
