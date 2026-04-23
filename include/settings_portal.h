#pragma once

#include "define.h"

// Запускает веб-портал SettingsGyver и привязывает обработчик построения страницы.
void initSettings();

// Строит весь интерфейс портала настроек при каждом запросе или перезагрузке страницы.
void build(sets::Builder &b);
