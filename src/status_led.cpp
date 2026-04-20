#include "status_led.h"

#include <freertos/queue.h>
#include <freertos/task.h>

namespace
{
#if PROJECT_HAS_BOARD_LED
constexpr uint32_t kConnectingBlinkMs = 350;
constexpr uint32_t kAccessPointBlinkMs = 120;
constexpr uint32_t kNetworkPulseMs = 60;
constexpr uint8_t kStatusLedTaskCore = 0;
constexpr UBaseType_t kStatusLedTaskPriority = tskIDLE_PRIORITY;
constexpr uint16_t kStatusLedTaskStack = 2048;
constexpr uint8_t kStatusLedQueueLength = 8;

QueueHandle_t statusLedQueue = nullptr;
TaskHandle_t statusLedTaskHandle = nullptr;

constexpr uint8_t kLedOnLevel = STATUS_LED_ACTIVE_LOW ? LOW : HIGH;
constexpr uint8_t kLedOffLevel = STATUS_LED_ACTIVE_LOW ? HIGH : LOW;

struct StatusLedState
{
    StatusLedCommand mode = StatusLedCommand::Off;
    bool blinkState = false;
    uint32_t activityPulseUntil = 0;
};

void writeStatusLed(bool enabled)
{
    digitalWrite(STATUS_LED_BOARD_PIN, enabled ? kLedOnLevel : kLedOffLevel);
}

uint32_t currentBlinkPeriod(StatusLedCommand mode)
{
    return (mode == StatusLedCommand::AccessPoint) ? kAccessPointBlinkMs : kConnectingBlinkMs;
}

bool activityPulseActive(const StatusLedState &state, uint32_t now)
{
    return state.activityPulseUntil && (int32_t)(state.activityPulseUntil - now) > 0;
}

void applyStatusLedState(const StatusLedState &state)
{
    uint32_t now = millis();

    switch (state.mode)
    {
    case StatusLedCommand::Off:
        writeStatusLed(false);
        break;

    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::AccessPoint:
        writeStatusLed(state.blinkState);
        break;

    case StatusLedCommand::StationConnected:
        // В STA-режиме LED горит постоянно, а на сетевую передачу кратко гаснет.
        writeStatusLed(!activityPulseActive(state, now));
        break;

    case StatusLedCommand::PulseNetworkActivity:
        // Это одноразовая команда, отдельного состояния у неё нет.
        writeStatusLed(false);
        break;
    }
}

void handleStatusLedCommand(StatusLedState &state, StatusLedCommand command)
{
    switch (command)
    {
    case StatusLedCommand::Off:
    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::StationConnected:
    case StatusLedCommand::AccessPoint:
        state.mode = command;
        state.blinkState = false;
        state.activityPulseUntil = 0;
        break;

    case StatusLedCommand::PulseNetworkActivity:
        if (state.mode == StatusLedCommand::StationConnected)
        {
            state.activityPulseUntil = millis() + kNetworkPulseMs;
        }
        break;
    }
}

TickType_t statusLedWaitTicks(const StatusLedState &state)
{
    switch (state.mode)
    {
    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::AccessPoint:
        return pdMS_TO_TICKS(currentBlinkPeriod(state.mode));

    case StatusLedCommand::StationConnected:
        if (state.activityPulseUntil)
        {
            uint32_t now = millis();
            if ((int32_t)(state.activityPulseUntil - now) <= 0)
            {
                return 0;
            }
            return pdMS_TO_TICKS(state.activityPulseUntil - now);
        }
        return portMAX_DELAY;

    case StatusLedCommand::Off:
    case StatusLedCommand::PulseNetworkActivity:
    default:
        return portMAX_DELAY;
    }
}

void onStatusLedTimeout(StatusLedState &state)
{
    switch (state.mode)
    {
    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::AccessPoint:
        state.blinkState = !state.blinkState;
        break;

    case StatusLedCommand::StationConnected:
        if (state.activityPulseUntil && (int32_t)(millis() - state.activityPulseUntil) >= 0)
        {
            state.activityPulseUntil = 0;
        }
        break;

    case StatusLedCommand::Off:
    case StatusLedCommand::PulseNetworkActivity:
    default:
        break;
    }
}

void statusLedTask(void *arg)
{
    (void)arg;

    StatusLedState state;
    applyStatusLedState(state);

    while (true)
    {
        StatusLedCommand command = StatusLedCommand::Off;
        TickType_t waitTicks = statusLedWaitTicks(state);

        if (xQueueReceive(statusLedQueue, &command, waitTicks) == pdTRUE)
        {
            handleStatusLedCommand(state, command);
        }
        else
        {
            onStatusLedTimeout(state);
        }

        applyStatusLedState(state);
    }
}
#endif
} // namespace

void initStatusLed()
{
#if PROJECT_HAS_BOARD_LED
    if (statusLedQueue)
    {
        return;
    }

    pinMode(STATUS_LED_BOARD_PIN, OUTPUT);
    writeStatusLed(false);

    statusLedQueue = xQueueCreate(kStatusLedQueueLength, sizeof(StatusLedCommand));
    if (!statusLedQueue)
    {
        return;
    }

    xTaskCreatePinnedToCore(
        statusLedTask,
        "statusLedTask",
        kStatusLedTaskStack,
        nullptr,
        kStatusLedTaskPriority,
        &statusLedTaskHandle,
        kStatusLedTaskCore);
#endif
}

void sendStatusLedCommand(StatusLedCommand command)
{
#if PROJECT_HAS_BOARD_LED
    if (!statusLedQueue)
    {
        return;
    }

    // Очередь используется как mailbox команд.
    // Для Pulse-команд допускаем потерю лишних импульсов при очень частой сетевой активности.
    if (xQueueSendToBack(statusLedQueue, &command, 0) != pdTRUE &&
        command != StatusLedCommand::PulseNetworkActivity)
    {
        xQueueReset(statusLedQueue);
        xQueueSendToBack(statusLedQueue, &command, 0);
    }
#else
    (void)command;
#endif
}
