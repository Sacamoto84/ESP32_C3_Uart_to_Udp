#ifndef _eeprom_h
#define _eeprom_h

#include <GyverDBFile.h>
#include <LittleFS.h>

extern GyverDBFile db;

// база данных для хранения настроек
// будет автоматически записываться в файл при изменениях

// имена ячеек базы данных
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
    wifiPower
    );

// ## Получаем единственный экземпляр
// EEPROM& settings = EEPROM::getInstance();
class EEPROM
{
public:

    int all_TX_to_UDP;
    int all_RX_from_UDP;

    // Единственный способ получить экземпляр
    static EEPROM &getInstance()
    {
        static EEPROM instance;
        return instance;
    }

private:
    // Запрет копирования
    EEPROM(const EEPROM &) = delete;
    EEPROM &operator=(const EEPROM &) = delete;

    // Приватный конструктор - нельзя создать извне
    EEPROM()
    {
        // Инициализация по умолчанию, аналог init блока в Kotlin
        // запуск и инициализация полей БД
        db.begin();
    
        // создаёт ячейку соответствующего типа и записывает "начальные" данные,
        // если такой ячейки ещё нет в БД
        db.init(kk::ipClient, "192.168.0.100");
        db.init(kk::echo, true);
        db.init(kk::broadcast, false);
        db.init(kk::timeout, 1000);
        db.init(kk::Serial2Bitrate, 9800);
        db.init(kk::WIFI_SSID, "TP-Link_BC0C");
        db.init(kk::WIFI_PASS, "58133514");
        db.init(kk::externalScreen, false);
        db.init(kk::wifiPower, 34);

    }
};

#endif