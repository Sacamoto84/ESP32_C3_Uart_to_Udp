#pragma once

#include "define.h"

// Настраивает UART0, выделяет буферы драйвера и запускает задачу чтения UART.
void initUART();

// Читает события UART, вытаскивает входящие байты и отправляет их в TCP-очередь.
void uartTask(void *arg);
