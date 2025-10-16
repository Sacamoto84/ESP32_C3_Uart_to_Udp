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
    apply
    );

// ## Получаем единственный экземпляр
// EEPROM& settings = EEPROM::getInstance();
class EEPROM
{
public:

    String ipClient;

    // Глобальный настройки
    int SerialBitrate = 921600;   // Битрейт
    int Serial2Bitrate = 4000000; // Битрейт
    int timeout = 1000;           // Задержка новос строки
    bool echo = true;                 // Эхо на Serial
    bool broadcast = false;            // Использовать броадкаст пакеты

    String WIFI_SSID = "TP-Link_BC0C";
    String WIFI_PASS = "58133514";

    int all_TX_to_UDP;
    int all_RX_from_UDP;

    //-------------------------------------

    //-------------------------------------

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
        db.init(kk::Serial2Bitrate, 9600);

        db.init(kk::WIFI_SSID, "TP-Link_BC0C");
        db.init(kk::WIFI_PASS, "58133514");

        echo = db.get(kk::echo);
        broadcast = db.get(kk::broadcast);
        timeout = db.get(kk::timeout);
        Serial2Bitrate = db.get(kk::Serial2Bitrate);
        WIFI_SSID = db.get(kk::WIFI_SSID);
        WIFI_PASS = db.get(kk::WIFI_PASS);
        ipClient = db.get(kk::ipClient); 
    }
};

#endif