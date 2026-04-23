#include "command_cli.h"
#include "network_internal.h"
#include "status_led.h"

namespace
{
constexpr size_t kTcpCommandReadBufferSize = 256;
constexpr size_t kTcpCommandLogMaxChars = 1024;
constexpr uint32_t kTcpCommandIdleTimeoutMs = 200;

String gTcpCommandBuffer;
bool gTcpCommandTruncated = false;
uint32_t gTcpCommandLastByteAt = 0;

// Сбрасывает всё состояние, связанное со сборкой текущей команды из TCP 8900.
void resetTcpCommandCapture()
{
    gTcpCommandBuffer = "";
    gTcpCommandTruncated = false;
    gTcpCommandLastByteAt = 0;
}

// Завершает накопленную строку и передаёт её в SimpleCLI.
void dispatchTcpCommandBuffer(const char *reason)
{
    if (gTcpCommandBuffer.isEmpty() && !gTcpCommandTruncated)
    {
        return;
    }

    String commandLine = gTcpCommandBuffer;
    const bool truncated = gTcpCommandTruncated;
    resetTcpCommandCapture();

    commandLine.trim();
    if (commandLine.isEmpty())
    {
        return;
    }

    Serial.printf("TCP 8900 command (%s): %s%s\n",
                  (reason != nullptr && reason[0] != '\0') ? reason : "received",
                  commandLine.c_str(),
                  truncated ? " [truncated]" : "");
    executeCliCommandLine(commandLine);
}

// Досылает накопленную команду, пишет причину в лог и закрывает командный сокет.
void finishTcpCommandClient(const char *reason)
{
    dispatchTcpCommandBuffer(reason);

    if (reason != nullptr && reason[0] != '\0')
    {
        Serial.println(reason);
    }

    tcpCommandClient.stop();
    resetTcpCommandCapture();
}

// Обрабатывает один входящий байт и обновляет буфер/таймер сборки команды.
void appendTcpCommandChar(char value)
{
    gTcpCommandLastByteAt = millis();

    if (value == '\r' || value == '\n')
    {
        dispatchTcpCommandBuffer("line complete");
        return;
    }

    if (gTcpCommandTruncated)
    {
        return;
    }

    const size_t currentLength = gTcpCommandBuffer.length();
    if (currentLength >= kTcpCommandLogMaxChars)
    {
        gTcpCommandTruncated = true;
        return;
    }

    gTcpCommandBuffer += value;
}

// Передаёт прочитанный блок данных в посимвольный сборщик команды.
void appendTcpCommandChunk(const char *chunk, size_t len)
{
    for (size_t index = 0; index < len; ++index)
    {
        appendTcpCommandChar(chunk[index]);
    }
}

// Принимает нового клиента TCP 8900, если командный канал сейчас свободен.
void pollTcpCommandServer()
{
    if (!tcpCommandServer.hasClient())
    {
        return;
    }

    WiFiClient newClient = tcpCommandServer.accept();
    if (!newClient)
    {
        return;
    }

    if (tcpCommandClient.connected())
    {
        Serial.printf("pollTcpCommandServer: rejecting extra client %s:%u\n",
                      newClient.remoteIP().toString().c_str(),
                      newClient.remotePort());
        newClient.stop();
        return;
    }

    if (!tcpCommandClient.connected() && !tcpCommandClient.available())
    {
        tcpCommandClient.stop();
        resetTcpCommandCapture();
    }

    newClient.setNoDelay(true);
    newClient.setTimeout(20);
    tcpCommandClient = newClient;
    resetTcpCommandCapture();

    Serial.printf("pollTcpCommandServer: TCP 8900 client connected from %s:%u\n",
                  tcpCommandClient.remoteIP().toString().c_str(),
                  tcpCommandClient.remotePort());
}

// Вычитывает ожидающие байты команды и завершает её по newline, idle timeout или disconnect.
void serviceTcpCommandClient()
{
    if (!tcpCommandClient.connected() && !tcpCommandClient.available())
    {
        return;
    }

    char buffer[kTcpCommandReadBufferSize];

    while (tcpCommandClient.available() > 0)
    {
        const size_t bytesToRead = static_cast<size_t>(tcpCommandClient.available()) > sizeof(buffer)
            ? sizeof(buffer)
            : static_cast<size_t>(tcpCommandClient.available());
        const int bytesRead = tcpCommandClient.read(reinterpret_cast<uint8_t *>(buffer), bytesToRead);
        if (bytesRead <= 0)
        {
            break;
        }

        appendTcpCommandChunk(buffer, static_cast<size_t>(bytesRead));
        sendStatusLedCommand(StatusLedCommand::PulseNetworkActivity);
    }

    const bool hasCommandData = gTcpCommandBuffer.length() > 0 || gTcpCommandTruncated;

    if (hasCommandData && (millis() - gTcpCommandLastByteAt) > kTcpCommandIdleTimeoutMs)
    {
        finishTcpCommandClient("TCP 8900 command completed");
        return;
    }

    if (!tcpCommandClient.connected() && !tcpCommandClient.available())
    {
        if (hasCommandData)
        {
            finishTcpCommandClient("TCP 8900 client disconnected");
        }
        else
        {
            tcpCommandClient.stop();
            resetTcpCommandCapture();
        }
    }
}
}

// Обслуживает командный сервер отдельно от высоконагруженного потока UART->TCP.
void networkRxTask(void *arg)
{
    (void)arg;

    for (;;)
    {
        pollTcpCommandServer();
        serviceTcpCommandClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
