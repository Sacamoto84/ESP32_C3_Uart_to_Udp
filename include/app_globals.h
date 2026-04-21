#pragma once

#include "define.h"

// Общие глобальные объекты проекта.
// Здесь только объявления, реальные определения лежат в app_globals.cpp.
extern WiFiUDP udp;
extern WiFiClient tcpClient;
extern QueueHandle_t uartQueue;
extern GyverDBFile db;
extern SettingsGyver sett;
#if PROJECT_HAS_SCREEN
extern Adafruit_SSD1306 display;
#endif
