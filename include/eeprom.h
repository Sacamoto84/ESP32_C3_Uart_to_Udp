#ifndef _eeprom_h
#define _eeprom_h

#include <GyverDBFile.h>

extern GyverDBFile db;

#if defined(HW_VARIANT_ESP32_S2_MINI)
constexpr int kDefaultNetworkTxQueueLength = 256;
#elif defined(HW_VARIANT_ESP32_C3)
constexpr int kDefaultNetworkTxQueueLength = 64;
#else
constexpr int kDefaultNetworkTxQueueLength = 32;
#endif

constexpr int kDefaultSerialRxBufferKb = 64;
constexpr int kSerialRxBufferMinKb = 1;
constexpr int kSerialRxBufferMaxKb = 256;
constexpr int kDefaultStatusLedBrightness = 255;
constexpr int kStatusLedBrightnessMin = 0;
constexpr int kStatusLedBrightnessMax = 255;
constexpr bool kDefaultStatusLedEnabled = true;
#if defined(HW_VARIANT_ESP32_S2_MINI)
constexpr bool kDefaultStatusLedActiveLow = false;
#elif defined(HW_VARIANT_ESP32_C3)
constexpr bool kDefaultStatusLedActiveLow = true;
#else
constexpr bool kDefaultStatusLedActiveLow = false;
#endif

// Ключи базы GyverDB. Legacy-ключи оставлены, чтобы старые базы продолжали открываться.
DB_KEYS(
    kk,
    ipClient,
    echo,
    broadcast,
    SerialBitrate,
    Serial2Bitrate,
    timeout,
    WIFI_SSID,
    WIFI_PASS,
    apply,
    externalScreen,
    useTcpTransport,
    useStaticIp,
    staticIp,
    staticGateway,
    staticSubnet,
    wifiPower,
    screenBrightness,
    networkTxQueueLength,
    serialRxBufferKb,
    statusLedBrightness,
    statusLedEnabled,
    statusLedActiveLow);

// Небольшой одиночный хелпер, который гарантирует однократную инициализацию БД настроек.
class EEPROM
{
public:
    int all_TX_to_network = 0;
    int all_RX_from_network = 0;

    // Возвращает единственный экземпляр EEPROM-хелпера, используемый прошивкой.
    static EEPROM &getInstance();

private:
    EEPROM(const EEPROM &) = delete;
    EEPROM &operator=(const EEPROM &) = delete;

    // Заполняет базу начальными значениями, если нужных ключей ещё нет.
    EEPROM();
};

#endif
