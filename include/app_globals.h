#pragma once

#include "define.h"
#include "eeprom.h"

#include <SettingsGyver.h>

#include <atomic>

// Общие глобальные объекты проекта, которые создаются один раз в app_globals.cpp.
extern WiFiUDP udp;
extern WiFiUDP heartbeatUdp;
extern WiFiServer tcpServer;
extern WiFiClient tcpClient;
extern WiFiServer tcpCommandServer;
extern WiFiClient tcpCommandClient;
extern QueueHandle_t uartQueue;
extern QueueHandle_t networkTxQueue;
extern GyverDBFile db;
extern SettingsGyver sett;
extern std::atomic<bool> tcpClientConnected;
extern std::atomic<uint32_t> droppedNetworkTxBytes;
extern std::atomic<uint32_t> actualNetworkTxQueueLength;
#if PROJECT_HAS_SCREEN
extern OledDisplay display;
#endif
