#pragma once

#include "define.h"

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
extern volatile bool tcpClientConnected;
extern volatile uint32_t droppedNetworkTxBytes;
extern volatile uint32_t actualNetworkTxQueueLength;
#if PROJECT_HAS_SCREEN
extern OledDisplay display;
#endif
