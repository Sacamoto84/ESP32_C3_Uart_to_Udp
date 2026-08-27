#include "esp32_log.h"

#include <atomic>

namespace
{
uint8_t gTcpLogBuffer[kEsp32TcpLogBufferSize];
size_t gTcpLogTail = 0;
size_t gTcpLogSize = 0;
uint32_t gTcpLogOldestSequence = 0;
uint32_t gTcpLogNextSequence = 0;
bool gTcpLogLineStart = true;
std::atomic<bool> gTcpLogEnabled{true};
portMUX_TYPE gTcpLogMux = portMUX_INITIALIZER_UNLOCKED;

constexpr char kLogPrefix[] = "[esp32] ";

void appendByteLocked(uint8_t byte)
{
    const size_t writeIndex = (gTcpLogTail + gTcpLogSize) % kEsp32TcpLogBufferSize;
    gTcpLogBuffer[writeIndex] = byte;

    if (gTcpLogSize == kEsp32TcpLogBufferSize)
    {
        gTcpLogTail = (gTcpLogTail + 1) % kEsp32TcpLogBufferSize;
        ++gTcpLogOldestSequence;
    }
    else
    {
        ++gTcpLogSize;
    }

    ++gTcpLogNextSequence;
}

void captureTcpLog(const uint8_t *buffer, size_t size)
{
    if (buffer == nullptr || size == 0 || !gTcpLogEnabled.load())
    {
        return;
    }

    portENTER_CRITICAL(&gTcpLogMux);
    if (gTcpLogEnabled.load())
    {
        for (size_t i = 0; i < size; ++i)
        {
            if (gTcpLogLineStart)
            {
                for (const char prefixByte : kLogPrefix)
                {
                    if (prefixByte == '\0')
                    {
                        break;
                    }

                    appendByteLocked((uint8_t)prefixByte);
                }
                gTcpLogLineStart = false;
            }

            appendByteLocked(buffer[i]);
            if (buffer[i] == '\n')
            {
                gTcpLogLineStart = true;
            }
        }
    }
    portEXIT_CRITICAL(&gTcpLogMux);
}

void clearTcpLogLocked()
{
    gTcpLogTail = 0;
    gTcpLogSize = 0;
    gTcpLogOldestSequence = gTcpLogNextSequence;
    gTcpLogLineStart = true;
}
}

Esp32LogStream projectLog;

void Esp32LogStream::begin(unsigned long baud)
{
    ::Serial.begin(baud);
}

Esp32LogStream::operator bool() const
{
    return static_cast<bool>(::Serial);
}

size_t Esp32LogStream::write(uint8_t byte)
{
    ::Serial.write(byte);
    captureTcpLog(&byte, 1);
    return 1;
}

size_t Esp32LogStream::write(const uint8_t *buffer, size_t size)
{
    if (buffer == nullptr || size == 0)
    {
        return 0;
    }

    ::Serial.write(buffer, size);
    captureTcpLog(buffer, size);
    return size;
}

void Esp32LogStream::flush()
{
    ::Serial.flush();
}

int Esp32LogStream::availableForWrite()
{
    return ::Serial.availableForWrite();
}

void setTcpEsp32LogEnabled(bool enabled)
{
    const bool wasEnabled = gTcpLogEnabled.exchange(enabled);
    if (wasEnabled == enabled)
    {
        return;
    }

    portENTER_CRITICAL(&gTcpLogMux);
    clearTcpLogLocked();
    portEXIT_CRITICAL(&gTcpLogMux);
}

bool isTcpEsp32LogEnabled()
{
    return gTcpLogEnabled.load();
}

uint32_t getEsp32TcpLogOldestSequence()
{
    portENTER_CRITICAL(&gTcpLogMux);
    const uint32_t oldestSequence = gTcpLogOldestSequence;
    portEXIT_CRITICAL(&gTcpLogMux);
    return oldestSequence;
}

size_t readEsp32TcpLog(uint32_t &cursor, uint8_t *destination, size_t capacity)
{
    if (destination == nullptr || capacity == 0 || !gTcpLogEnabled.load())
    {
        return 0;
    }

    portENTER_CRITICAL(&gTcpLogMux);

    if (cursor < gTcpLogOldestSequence)
    {
        cursor = gTcpLogOldestSequence;
    }
    else if (cursor > gTcpLogNextSequence)
    {
        cursor = gTcpLogNextSequence;
    }

    const size_t available = (size_t)(gTcpLogNextSequence - cursor);
    const size_t count = (available < capacity) ? available : capacity;
    const size_t startOffset = (size_t)(cursor - gTcpLogOldestSequence);

    for (size_t i = 0; i < count; ++i)
    {
        destination[i] = gTcpLogBuffer[(gTcpLogTail + startOffset + i) % kEsp32TcpLogBufferSize];
    }

    cursor += (uint32_t)count;
    portEXIT_CRITICAL(&gTcpLogMux);
    return count;
}
