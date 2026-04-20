#pragma once

#include "define.h"

// Команды для отдельной LED-задачи.
// Внешние модули не трогают GPIO напрямую, а только отправляют сюда нужное событие.
enum class StatusLedCommand : uint8_t
{
    Off = 0,
    ConnectingToStation,
    StationConnected,
    AccessPoint,
    PulseNetworkActivity,
};

// Стартует отдельную задачу управления LED на core 0 с минимальным приоритетом.
void initStatusLed();

// Кладёт команду в очередь LED-задачи.
void sendStatusLedCommand(StatusLedCommand command);
