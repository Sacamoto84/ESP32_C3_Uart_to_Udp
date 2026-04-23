#pragma once

#include "define.h"

// Инициализирует OLED-дисплей и применяет сохранённые настройки экрана.
void initDisplay();

// Напрямую записывает значение контраста в контроллер OLED.
void applyDisplayBrightness(uint8_t brightness);

// Рисует локальный экран состояния, когда внешний UDP-экран отключён.
void screenLoop();
