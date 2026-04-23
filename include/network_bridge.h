#pragma once

#include "define.h"

constexpr int kNetworkTxQueueMinLength = 4;
constexpr int kNetworkTxQueueMaxLength = 1024;

// Преобразует код мощности Wi-Fi в понятную строку для интерфейса и логов.
String WifiCurrentPowerString(int power);

// Подключает устройство к Wi-Fi или переводит его в режим точки доступа при неудаче.
void initWiFi();

// Запускает UDP-порты для heartbeat и, при необходимости, внешнего экрана.
void initUDP();

// Поднимает TCP-серверы и фоновые задачи для потокового и командного каналов.
void initTcpServer();

// Публикует hostname устройства и HTTP-сервис через mDNS.
void initMdns();

// Настраивает ArduinoOTA и регистрирует OTA-обработчики.
void initOTA();

// Обслуживает состояние OTA; должна вызываться из главного цикла.
void tickOTA();

// Отвечает на heartbeat-пакеты, которые шлёт Android-приложение.
void handleHeartbeatUdp();

// Принимает сырой буфер кадра OLED-экрана от внешнего UDP-источника.
void handleExternalScreenUdp();

// Разбивает UART-данные на чанки фиксированного размера и кладёт их в сетевую очередь.
size_t IRAM_ATTR enqueueNetworkTxData(const uint8_t *data, size_t len);

// Возвращает true, пока основной TCP-клиент на порту 8888 подключён.
bool isTcpClientConnected();

// Возвращает накопленное число байтов, отброшенных из-за переполнения UART->TCP очереди.
uint32_t getDroppedNetworkTxBytes();

// Возвращает текущее число чанков в очереди UART->TCP.
uint32_t getQueuedNetworkTxChunks();

// Возвращает ёмкость очереди, выбранную во время старта устройства.
uint32_t getNetworkTxQueueCapacity();
