#pragma once

#include <Arduino.h>

// Объём кольцевого буфера диагностических сообщений ESP32. Старые записи
// вытесняются новыми, поэтому сетевой поток не может занять неограниченную RAM.
constexpr size_t kEsp32TcpLogBufferSize = 8192;

// Поток-обёртка: дублирует вывод в обычный USB Serial и в TCP-диагностику.
class Esp32LogStream : public Print
{
public:
    void begin(unsigned long baud);
    operator bool() const;

    size_t write(uint8_t byte) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    void flush() override;

    int availableForWrite() override;
};

extern Esp32LogStream projectLog;

// Включает или выключает захват диагностик для TCP. При смене состояния буфер
// очищается, чтобы клиент не получил записи из предыдущего режима.
void setTcpEsp32LogEnabled(bool enabled);
bool isTcpEsp32LogEnabled();

// Возвращает позицию первой доступной записи. Новый TCP-клиент начинает
// чтение с неё и поэтому получает накопленную историю.
uint32_t getEsp32TcpLogOldestSequence();

// Копирует новые записи начиная с cursor и сдвигает cursor. Функция безопасна
// для вызова из сетевой задачи одновременно с выводом из других задач.
size_t readEsp32TcpLog(uint32_t &cursor, uint8_t *destination, size_t capacity);
