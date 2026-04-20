#pragma once

#include "define.h"

// Инициализация и обслуживание OLED-дисплея.
void initDisplay();
void applyDisplayBrightness(uint8_t brightness);
void screenLoop();
