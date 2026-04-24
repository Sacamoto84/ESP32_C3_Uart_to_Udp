#pragma once

#include "network_bridge.h"

#include "app_globals.h"

// Общие сетевые константы, которые использует мост ESP32.
constexpr uint16_t kTcpServerPort = 8888;
constexpr uint16_t kTcpCommandPort = 8900;
constexpr uint16_t kHeartbeatPort = 8888;
constexpr uint32_t kTcpWriteTimeoutMs = 3000;
// Таймауты WiFiClient.setTimeout для потокового и командного сокета.
constexpr uint32_t kTcpStreamClientTimeoutMs = 2000;
constexpr uint32_t kTcpCommandClientTimeoutMs = 20;

// Один фрагмент очереди UART->TCP. Размер чанка фиксирован во время компиляции.
struct NetworkTxChunk
{
    uint16_t len;
    uint8_t data[NETWORK_TX_CHUNK_SIZE];
};

// Фоновая задача, которая вычитывает UART->TCP очередь и отправляет данные в потоковый сокет.
void networkTxTask(void *arg);

// Фоновая задача, которая принимает и разбирает SimpleCLI-команды из TCP 8900.
void networkRxTask(void *arg);

// Принимает нового TCP-клиента на порту 8888, если он ожидает подключения.
void pollTcpServer();

// Закрывает текущего потокового TCP-клиента и обновляет LED/логи.
void disconnectTcpClient(const char *reason);

// Обновляет кэшированное состояние соединения для потокового TCP-сокета.
bool refreshTcpClientConnectionState(const char *disconnectReason);

// Отправляет один буфер активному потоковому TCP-клиенту с защитой по таймауту.
bool sendTcpChunk(const uint8_t *payload, size_t len);

// Обрабатывает один heartbeat-запрос по UDP, если пакет ожидает в сокете.
void handleHeartbeatUdp();

#if PROJECT_HAS_SCREEN
// Рисует footer с версией прошивки на стартовых экранах.
void drawStartupVersionFooter();
#endif
