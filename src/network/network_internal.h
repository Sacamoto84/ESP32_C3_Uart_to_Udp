#pragma once

#include "network_bridge.h"

#include "app_globals.h"

#include "lwip/sockets.h"
#include <lwip/inet.h>

// Внутренние helper-функции сетевого модуля.
// Они нужны нескольким .cpp в папке network, но наружу их не экспортируем.
constexpr uint16_t kNetworkDataPort = 8888;

bool sendUdpPacket(const char *tag, const char *payload, int len, const sockaddr_in &dest, bool enableBroadcast);
bool ensureTcpConnection(const char *ip, uint16_t port);
bool sendTcpPacket(const char *tag, const uint8_t *payload, size_t len, const char *ip, uint16_t port);

#if PROJECT_HAS_SCREEN
void drawStartupVersionFooter();
#endif
