#pragma once

#include "define.h"

// Служебное преобразование кода мощности Wi-Fi в строку для UI/дисплея.
String WifiCurrentPowerString(int power);

// Базовая сеть проекта: Wi-Fi, TCP server для Android и UDP для внешнего экрана.
void initWiFi();
void initUDP();
void initTcpServer();
void initMdns();
void initOTA();
void tickOTA();
void handleExternalScreenUdp();

// Помещаем UART-данные в ограниченную очередь.
// В обычном режиме они уйдут в TCP server, в broadcast-режиме - по UDP broadcast.
size_t enqueueNetworkTxData(const uint8_t *data, size_t len);

// UDP оставляем только для широковещательных пакетов.
bool sendUdpBroadcast(const char *msg, int len);

// Getter'ы для UI, локального экрана и отладки.
bool isTcpClientConnected();
uint32_t getDroppedNetworkTxBytes();
uint32_t getQueuedNetworkTxChunks();
uint32_t getNetworkTxQueueCapacity();
