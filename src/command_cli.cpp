#include "command_cli.h"

#include "app_globals.h"
#include "hardware.h"
#include "network/network_internal.h"

// Общий экземпляр CLI, который обслуживает командный канал TCP 8900.
SimpleCLI cli;

namespace
{
Command gHelpCommand;
Command gStatusCommand;
Command gRebootCommand;
Command gResetPulseCommand;

// Отправляет ответ CLI в USB-лог и, если клиент подключён, обратно в TCP-командный сокет.
void sendCliResponse(const String &message)
{
    if (message.isEmpty())
    {
        return;
    }

    Serial.println(message);
    tcpCommandClient.print(message);
    tcpCommandClient.print("\n");
    tcpCommandClient.flush();
}

// Печатает список зарегистрированных CLI-команд.
void handleHelpCommand()
{
    sendCliResponse("Help:\n" + cli.toString());
}

// Возвращает компактную сводку о текущем состоянии моста и сети.
void handleStatusCommand()
{
    String status = "ip=" + WiFi.localIP().toString() +
                    " tcpRead=" + String(kTcpServerPort) +
                    " tcpCmd=" + String(kTcpCommandPort) +
                    " queue=" + String(getQueuedNetworkTxChunks()) + "/" + String(getNetworkTxQueueCapacity()) +
                    " dropped=" + String(getDroppedNetworkTxBytes());
    sendCliResponse(status);
}

// Перезагружает ESP32 после подтверждения действия вызывающей стороне.
void handleRebootCommand()
{
    sendCliResponse("rebooting");
    delay(20);
    ESP.restart();
}

// Даёт такой же низкий импульс сброса, как и кнопка в settings-портале.
void handleResetPulseCommand()
{
    sendCliResponse("reset pulse");
    pulseResetLine();
}

// Сливает очередь ошибок SimpleCLI в логи и TCP-ответ.
void drainCliErrors()
{
    while (cli.errored())
    {
        CommandError error = cli.getError();
        const String message = "CLI error: " + error.toString();
        sendCliResponse(message);
    }
}

// Забирает все распознанные команды из очереди SimpleCLI и выполняет нужный обработчик.
void drainCliCommands()
{
    while (cli.available())
    {
        Command command = cli.getCmd();
        const String commandName = command.getName();

        if (command == gHelpCommand)
        {
            handleHelpCommand();
        }
        else if (command == gStatusCommand)
        {
            handleStatusCommand();
        }
        else if (command == gRebootCommand)
        {
            handleRebootCommand();
        }
        else if (command == gResetPulseCommand)
        {
            handleResetPulseCommand();
        }
        else
        {
            sendCliResponse("CLI warning: unhandled command " + commandName);
        }
    }
}
}

// Регистрирует встроенные команды, доступные через TCP 8900.
void initCommandCli()
{
    gHelpCommand = cli.addCmd("help");
    gHelpCommand.setDescription("Print registered SimpleCLI commands");

    gStatusCommand = cli.addCmd("status");
    gStatusCommand.setDescription("Print current bridge status");

    gRebootCommand = cli.addCmd("reboot");
    gRebootCommand.setDescription("Restart ESP32");

    gResetPulseCommand = cli.addCmd("resetpulse");
    gResetPulseCommand.setDescription("Send low pulse to RESET_PULSE_PIN");

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

    cli.parse(normalized);
    drainCliCommands();
    drainCliErrors();
}
