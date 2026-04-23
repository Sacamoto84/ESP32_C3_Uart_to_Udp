#pragma once

#include "define.h"

#include <SimpleCLI.h>

// Глобальный экземпляр SimpleCLI, который использует командный TCP-порт и локальные обработчики.
extern SimpleCLI cli;

// Регистрирует все поддерживаемые CLI-команды и общий обработчик ошибок парсинга.
void initCommandCli();

// Нормализует и разбирает одну строку команды, пришедшую по TCP 8900.
void executeCliCommandLine(const String &line);
