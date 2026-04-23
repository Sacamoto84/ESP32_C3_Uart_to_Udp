#pragma once

#include "define.h"

// Высокоуровневые состояния светодиода, которые понимает отдельная LED-задача.
enum class StatusLedCommand : uint8_t
{
    Off = 0,
    ConnectingToStation,
    WaitingForClient,
    ClientConnected,
    AccessPoint,
    PulseNetworkActivity,
    RefreshBrightness,
};

// Запускает драйвер светодиода и фоновую задачу, которая рисует его состояния.
void initStatusLed();

// Кладёт новое состояние или импульс активности в очередь LED-задачи.
void sendStatusLedCommand(StatusLedCommand command);
