#pragma once

#include "define.h"

// Служебное преобразование кода мощности Wi-Fi в строку для UI/дисплея.
String WifiCurrentPowerString(int power);

// Сетевой стек проекта: Wi-Fi, UDP и внешний экран по UDP.
void initWiFi();
void initUDP();
void handleExternalScreenUdp();

// Отправка UART-данных в UDP.
void sendUdpMessage(const char *msg, const char *ip);
void sendUdpMessageLen(const char *msg, int len, const char *ip);
void sendUdpBroadcast(const char *msg, int len);
