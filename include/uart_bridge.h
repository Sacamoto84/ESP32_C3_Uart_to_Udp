#pragma once

#include "define.h"

// UART-мост: инициализация драйвера и фоновая задача чтения.
void initUART();
void uartTask(void *arg);
