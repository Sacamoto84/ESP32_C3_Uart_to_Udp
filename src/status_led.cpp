#include "status_led.h"

#include <freertos/queue.h>
#include <freertos/task.h>

namespace
{
#if PROJECT_HAS_BOARD_LED
constexpr uint32_t kConnectingBlinkMs = 350;
constexpr uint32_t kAccessPointBlinkMs = 120;
constexpr uint32_t kNetworkPulseMs = 50; //60
constexpr uint8_t kStatusLedTaskCore = 0;
constexpr UBaseType_t kStatusLedTaskPriority = tskIDLE_PRIORITY;
constexpr uint16_t kStatusLedTaskStack = 2048;
constexpr uint8_t kStatusLedQueueLength = 8;
constexpr uint32_t kStatusLedPwmFreq = 5000;
constexpr uint8_t kStatusLedPwmResolution = 8;
constexpr uint32_t kStatusLedPwmMaxDuty = (1UL << kStatusLedPwmResolution) - 1;
constexpr uint8_t kStatusLedPwmChannel = 0;

QueueHandle_t statusLedQueue = nullptr;
TaskHandle_t statusLedTaskHandle = nullptr;
bool statusLedPwmReady = false;

struct StatusLedState
{
    StatusLedCommand mode = StatusLedCommand::Off;
    bool blinkState = false;
    uint32_t activityPulseUntil = 0;
};

uint32_t statusLedOnDuty()
{
    return (STATUS_LED_BRIGHTNESS > kStatusLedPwmMaxDuty) ? kStatusLedPwmMaxDuty : STATUS_LED_BRIGHTNESS;
}

uint32_t statusLedRawDuty(bool enabled)
{
    uint32_t onDuty = statusLedOnDuty();

    if (STATUS_LED_ACTIVE_LOW)
    {
        return enabled ? (kStatusLedPwmMaxDuty - onDuty) : kStatusLedPwmMaxDuty;
    }

    return enabled ? onDuty : 0;
}

void writeStatusLed(bool enabled)
{
    if (!statusLedPwmReady)
    {
        return;
    }

    // В старом Arduino-ESP32 PWM пишется в канал, а не напрямую в GPIO.
    ledcWrite(kStatusLedPwmChannel, statusLedRawDuty(enabled));
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
    case StatusLedCommand::WaitingForClient:
        writeStatusLed(false);
        break;

    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::AccessPoint:
        writeStatusLed(state.blinkState);
        break;

    case StatusLedCommand::ClientConnected:
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
    case StatusLedCommand::WaitingForClient:
    case StatusLedCommand::ClientConnected:
    case StatusLedCommand::AccessPoint:
        state.mode = command;
        state.blinkState = false;
        state.activityPulseUntil = 0;
        break;

    case StatusLedCommand::PulseNetworkActivity:
        if (state.mode == StatusLedCommand::ClientConnected)
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

    case StatusLedCommand::ClientConnected:
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

    case StatusLedCommand::ClientConnected:
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

    Serial.printf("Status LED init: pin=%u active_low=%u brightness=%u\n",
                  (unsigned)STATUS_LED_BOARD_PIN,
                  (unsigned)STATUS_LED_ACTIVE_LOW,
                  (unsigned)STATUS_LED_BRIGHTNESS);

    // В framework, который сейчас стоит в проекте, используется классическая
    // связка ledcSetup + ledcAttachPin. Частота 0 означает неуспешную настройку.
    if (ledcSetup(kStatusLedPwmChannel, kStatusLedPwmFreq, kStatusLedPwmResolution) == 0)
    {
        Serial.println("Status LED PWM setup failed");
        return;
    }

    // В этой версии Arduino-ESP32 ledcReadFreq() возвращает 0, если текущий duty
    // канала равен 0. Поэтому ей нельзя валидировать attach на холодном старте:
    // мы как раз начинаем с погашенного LED.
    ledcAttachPin(STATUS_LED_BOARD_PIN, kStatusLedPwmChannel);
    statusLedPwmReady = true;
    writeStatusLed(false);

    statusLedQueue = xQueueCreate(kStatusLedQueueLength, sizeof(StatusLedCommand));
    if (!statusLedQueue)
    {
        ledcDetachPin(STATUS_LED_BOARD_PIN);
        statusLedPwmReady = false;
        return;
    }

    if (xTaskCreatePinnedToCore(
        statusLedTask,
        "statusLedTask",
        kStatusLedTaskStack,
        nullptr,
        kStatusLedTaskPriority,
        &statusLedTaskHandle,
        kStatusLedTaskCore) != pdPASS)
    {
        Serial.println("Status LED task create failed");
        vQueueDelete(statusLedQueue);
        statusLedQueue = nullptr;
        ledcDetachPin(STATUS_LED_BOARD_PIN);
        statusLedPwmReady = false;
        return;
    }

    Serial.println("Status LED ready");
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
