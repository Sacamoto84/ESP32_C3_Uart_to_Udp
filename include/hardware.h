#pragma once

#include "define.h"

// Подготавливает пины платы до запуска сетевых и прикладных модулей.
void initPins();

// Запускает Serial и монтирует LittleFS для логов и постоянных настроек.
void initSerialAndFS();

// Выполняет низкий импульс сброса на RESET_PULSE_PIN для перезагрузки внешнего устройства.
void pulseResetLine();

// Возвращает true, если Wi-Fi сейчас в режиме точки доступа (AP или APSTA).
bool isAccessPointMode();
