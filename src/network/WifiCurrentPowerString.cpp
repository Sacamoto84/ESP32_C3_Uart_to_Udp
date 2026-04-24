#include "network_internal.h"

namespace
{
// Таблица отображения текущей мощности Wi-Fi в читаемую строку.
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
};
}

// Преобразует текущий код мощности в строку, которую можно показать в интерфейсе и логах.
String WifiCurrentPowerString(int power)
{
    for (const auto &label : kWifiPowerLabels)
    {
        if (label.dbValue == power)
        {
            return label.label;
        }
    }

    return String(power) + " raw";
}
