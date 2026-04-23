#ifndef _eeprom_h
#define _eeprom_h

#include <WiFi.h>
#include <GyverDBFile.h>
#include <LittleFS.h>

extern GyverDBFile db;

#if defined(PROJECT_NETWORK_TX_QUEUE_LENGTH)
constexpr int kDefaultNetworkTxQueueLength = PROJECT_NETWORK_TX_QUEUE_LENGTH;
#else
constexpr int kDefaultNetworkTxQueueLength = 32;
#endif

constexpr int kDefaultSerialRxBufferKb = 64;
constexpr int kSerialRxBufferMinKb = 1;
constexpr int kSerialRxBufferMaxKb = 256;
constexpr int kDefaultStatusLedBrightness = 255;
constexpr int kStatusLedBrightnessMin = 0;
constexpr int kStatusLedBrightnessMax = 255;

// База данных для хранения настроек.
// GyverDBFile сам сохраняет изменения в файл при update().

// Имена ячеек базы данных.
// ipClient, broadcast и useTcpTransport оставлены как legacy-ключи,
// чтобы старые базы настроек спокойно открывались после обновления прошивки.
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
    statusLedBrightness);

// Получаем единственный экземпляр:
// EEPROM& settings = EEPROM::getInstance();
class EEPROM
{
public:
    int all_TX_to_network = 0;
    int all_RX_from_network = 0;

    // Единственный способ получить экземпляр.
    static EEPROM &getInstance()
    {
        static EEPROM instance;
        return instance;
    }

private:
    // Запрет копирования.
    EEPROM(const EEPROM &) = delete;
    EEPROM &operator=(const EEPROM &) = delete;

    // Приватный конструктор: экземпляр нельзя создать извне.
    EEPROM()
    {
        // Инициализация по умолчанию, аналог init-блока в Kotlin:
        // запускаем БД и создаём все поля, если их ещё нет.
        db.begin();

        // Создаёт ячейку соответствующего типа и записывает начальные данные,
        // если такой ячейки ещё нет в БД.
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
    }
};

#endif
