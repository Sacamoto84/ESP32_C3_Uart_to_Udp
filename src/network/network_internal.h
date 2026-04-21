#pragma once

#include "network_bridge.h"

#include "app_globals.h"

#include "lwip/sockets.h"
#include <lwip/inet.h>

// Общие сетевые константы проекта.
constexpr uint16_t kNetworkDataPort = 8888;
constexpr uint32_t kTcpWriteTimeoutMs = 3000;

// Один элемент очереди между UART и сетью.
// Размер ограничен compile-time define, чтобы не было неограниченного роста памяти.
struct NetworkTxChunk
{
    uint16_t len;
    bool useBroadcast;
    uint8_t data[NETWORK_TX_CHUNK_SIZE];
};

// Внутренние helper-функции сетевого модуля.
bool sendUdpPacket(const char *tag, const char *payload, int len, const sockaddr_in &dest, bool enableBroadcast);
void networkTxTask(void *arg);
void pollTcpServer();
void disconnectTcpClient(const char *reason);
bool refreshTcpClientConnectionState(const char *disconnectReason);
bool sendTcpChunk(const uint8_t *payload, size_t len);

#if PROJECT_HAS_SCREEN
void drawStartupVersionFooter();
#endif
