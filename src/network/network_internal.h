#pragma once

#include "network_bridge.h"

#include "app_globals.h"

#include "lwip/sockets.h"
#include <lwip/inet.h>

// Внутренние helper-функции сетевого модуля.
// Они нужны нескольким .cpp в папке network, но наружу их не экспортируем.
bool sendUdpPacket(const char *tag, const char *payload, int len, const sockaddr_in &dest, bool enableBroadcast);

#if PROJECT_HAS_SCREEN
void drawStartupVersionFooter();
#endif
