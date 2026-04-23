#pragma once

#include "define.h"

// Подготавливает пины платы до запуска сетевых и прикладных модулей.
void initPins();

// Запускает Serial и монтирует LittleFS для логов и постоянных настроек.
void initSerialAndFS();
