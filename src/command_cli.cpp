#include "command_cli.h"

#include "app_globals.h"
#include "network/network_internal.h"

// Общий экземпляр CLI, который обслуживает командный канал TCP 8900.
SimpleCLI cli;

namespace
{
// Отправляет ответ CLI в USB-лог и, если клиент подключён, обратно в TCP-командный сокет.
void sendCliResponse(const String &message)
{
    Serial.println(message);

    if (tcpCommandClient.connected())
    {
        tcpCommandClient.println(message);
    }
}

// Печатает список зарегистрированных CLI-команд.
void helpCallback(cmd *commandPointer)
{
    (void)commandPointer;
    sendCliResponse(cli.toString());
}

// Возвращает компактную сводку о текущем состоянии моста и сети.
void statusCallback(cmd *commandPointer)
{
    (void)commandPointer;

    String status = "ip=" + WiFi.localIP().toString() +
                    " tcpRead=" + String(kTcpServerPort) +
                    " tcpCmd=" + String(kTcpCommandPort) +
                    " queue=" + String(getQueuedNetworkTxChunks()) + "/" + String(getNetworkTxQueueCapacity()) +
                    " dropped=" + String(getDroppedNetworkTxBytes());
    sendCliResponse(status);
}

// Перезагружает ESP32 после подтверждения действия вызывающей стороне.
void rebootCallback(cmd *commandPointer)
{
    (void)commandPointer;
    sendCliResponse("rebooting");
    delay(20);
    ESP.restart();
}

// Даёт такой же низкий импульс сброса, как и кнопка в settings-портале.
void resetPulseCallback(cmd *commandPointer)
{
    (void)commandPointer;

    sendCliResponse("reset pulse");
    pinMode(RESET_PULSE_PIN, OUTPUT);
    digitalWrite(RESET_PULSE_PIN, LOW);
    delay(100);
    pinMode(RESET_PULSE_PIN, OPEN_DRAIN);
    digitalWrite(RESET_PULSE_PIN, HIGH);
}

// Преобразует ошибку SimpleCLI в читаемый текстовый ответ.
void errorCallback(cmd_error *errorPointer)
{
    CommandError error(errorPointer);
    const String message = "CLI error: " + error.toString();
    sendCliResponse(message);
}
}

// Регистрирует встроенные команды, доступные через TCP 8900.
void initCommandCli()
{
    Command helpCommand = cli.addCmd("help", helpCallback);
    helpCommand.setDescription("Print registered SimpleCLI commands");

    Command statusCommand = cli.addCmd("status", statusCallback);
    statusCommand.setDescription("Print current bridge status");

    Command rebootCommand = cli.addCmd("reboot", rebootCallback);
    rebootCommand.setDescription("Restart ESP32");

    Command resetPulseCommand = cli.addCmd("resetpulse", resetPulseCallback);
    resetPulseCommand.setDescription("Send low pulse to RESET_PULSE_PIN");

    cli.setOnError(errorCallback);

    Serial.println("SimpleCLI initialized");
}

// Нормализует входящую строку и передаёт её в парсер SimpleCLI.
void executeCliCommandLine(const String &line)
{
    String normalized = line;
    normalized.trim();

    if (normalized.isEmpty())
    {
        return;
    }

    Serial.printf("CLI parse: %s\n", normalized.c_str());
    cli.parse(normalized);
}
