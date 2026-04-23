#include "status_led.h"

#include <freertos/queue.h>
#include <freertos/task.h>

#if defined(HW_VARIANT_ESP32_C3)
#include <esp32-hal-rgb-led.h>
#endif

namespace
{
#if PROJECT_HAS_BOARD_LED
constexpr uint32_t kConnectingBlinkMs = 350;
constexpr uint32_t kAccessPointBlinkMs = 120;
// Длительность одной полуволны мигания LED при активности (on/off).
// ~20 Гц — визуально похоже на activity-LED у RJ-45 разъёмов.
constexpr uint32_t kActivityBlinkHalfMs = 25;
// Сколько миллисекунд без активности держим LED в solid ON.
constexpr uint32_t kActivityHoldMs = 120;
constexpr uint8_t kStatusLedTaskCore = 0;
// Приоритет выше loopTask (он запускается с приоритетом 1), чтобы мигалка
// не замирала на время блокирующих операций главного цикла — например,
// при I2C-пересылке фреймбуфера OLED раз в 2 с.
constexpr UBaseType_t kStatusLedTaskPriority = 3;
constexpr uint16_t kStatusLedTaskStack = 2048;
constexpr uint8_t kStatusLedQueueLength = 8;
constexpr uint32_t kStatusLedPwmFreq = 5000;
constexpr uint8_t kStatusLedPwmResolution = 8;
constexpr uint32_t kStatusLedPwmMaxDuty = (1UL << kStatusLedPwmResolution) - 1;
constexpr uint8_t kStatusLedPwmChannel = 0;

enum class StatusLedBackend : uint8_t
{
    None = 0,
    DigitalGpio,
    PwmGpio,
    NeoPixel,
};

QueueHandle_t statusLedQueue = nullptr;
TaskHandle_t statusLedTaskHandle = nullptr;
bool statusLedPwmReady = false;
StatusLedBackend statusLedBackend = StatusLedBackend::None;

struct StatusLedState
{
    StatusLedCommand mode = StatusLedCommand::Off;
    bool blinkState = false;
    uint32_t lastActivityMs = 0;
};

uint8_t statusLedOnLevel()
{
    return STATUS_LED_ACTIVE_LOW ? LOW : HIGH;
}

uint8_t statusLedOffLevel()
{
    return STATUS_LED_ACTIVE_LOW ? HIGH : LOW;
}

uint32_t statusLedOnDuty()
{
    return (STATUS_LED_BRIGHTNESS > kStatusLedPwmMaxDuty) ? kStatusLedPwmMaxDuty : STATUS_LED_BRIGHTNESS;
}

uint32_t statusLedRawDuty(bool enabled)
{
    const uint32_t onDuty = statusLedOnDuty();

    if (STATUS_LED_ACTIVE_LOW)
    {
        return enabled ? (kStatusLedPwmMaxDuty - onDuty) : kStatusLedPwmMaxDuty;
    }

    return enabled ? onDuty : 0;
}

void writeStatusLed(bool enabled)
{
    switch (statusLedBackend)
    {
    case StatusLedBackend::NeoPixel:
#if defined(HW_VARIANT_ESP32_C3)
    {
        const uint8_t level = enabled ? static_cast<uint8_t>(statusLedOnDuty()) : 0;
        neopixelWrite(STATUS_LED_BOARD_PIN, level, level, level);
        break;
    }
#else
        break;
#endif

    case StatusLedBackend::PwmGpio:
        if (statusLedPwmReady)
        {
            ledcWrite(kStatusLedPwmChannel, statusLedRawDuty(enabled));
        }
        break;

    case StatusLedBackend::DigitalGpio:
        digitalWrite(STATUS_LED_BOARD_PIN, enabled ? statusLedOnLevel() : statusLedOffLevel());
        break;

    case StatusLedBackend::None:
    default:
        break;
    }
}

bool initStatusLedBackend()
{
#if defined(HW_VARIANT_ESP32_C3)
    statusLedBackend = StatusLedBackend::NeoPixel;
    writeStatusLed(false);
    return true;
#else
    pinMode(STATUS_LED_BOARD_PIN, OUTPUT);

    if (STATUS_LED_BRIGHTNESS == 0 || STATUS_LED_BRIGHTNESS >= kStatusLedPwmMaxDuty)
    {
        statusLedBackend = StatusLedBackend::DigitalGpio;
        writeStatusLed(false);
        return true;
    }

    if (ledcSetup(kStatusLedPwmChannel, kStatusLedPwmFreq, kStatusLedPwmResolution) == 0)
    {
        Serial.println("Status LED PWM setup failed, fallback to digital mode");
        statusLedBackend = StatusLedBackend::DigitalGpio;
        writeStatusLed(false);
        return true;
    }

    ledcAttachPin(STATUS_LED_BOARD_PIN, kStatusLedPwmChannel);
    statusLedPwmReady = true;
    statusLedBackend = StatusLedBackend::PwmGpio;
    writeStatusLed(false);
    return true;
#endif
}

uint32_t currentBlinkPeriod(StatusLedCommand mode)
{
    return (mode == StatusLedCommand::AccessPoint) ? kAccessPointBlinkMs : kConnectingBlinkMs;
}

bool activityRecent(const StatusLedState &state, uint32_t now)
{
    return state.lastActivityMs && (now - state.lastActivityMs) < kActivityHoldMs;
}

void applyStatusLedState(const StatusLedState &state)
{
    const uint32_t now = millis();

    switch (state.mode)
    {
    case StatusLedCommand::Off:
        writeStatusLed(false);
        break;

    case StatusLedCommand::WaitingForClient:
        writeStatusLed(false);
        break;

    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::AccessPoint:
        writeStatusLed(state.blinkState);
        break;

    case StatusLedCommand::ClientConnected:
        // Нет активности — ровно горит. Есть активность — мигаем с фиксированной
        // частотой. Фазу берём напрямую из millis(), чтобы она не зависела
        // от того, как часто срабатывает таймаут очереди (при непрерывном
        // потоке она вообще не срабатывает).
        if (activityRecent(state, now))
        {
            const bool phase = ((now / kActivityBlinkHalfMs) & 1u) != 0;
            writeStatusLed(phase);
        }
        else
        {
            writeStatusLed(true);
        }
        break;

    case StatusLedCommand::PulseNetworkActivity:
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
        state.lastActivityMs = 0;
        break;

    case StatusLedCommand::PulseNetworkActivity:
        // Только отмечаем факт активности; саму мигалку крутит периодический
        // таймаут задачи, чтобы мигание было видно при любом темпе событий.
        if (state.mode == StatusLedCommand::ClientConnected)
        {
            state.lastActivityMs = millis();
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
    {
        const uint32_t now = millis();
        if (activityRecent(state, now))
        {
            // Пока активность считается «свежей» — просыпаемся на каждую
            // полуволну, чтобы инвертировать activityBlink.
            return pdMS_TO_TICKS(kActivityBlinkHalfMs);
        }
        if (state.lastActivityMs)
        {
            // Активность была, но уже устарела: просыпаемся ровно ко времени
            // возврата в solid ON, а дальше — спим до новой команды.
            const uint32_t elapsed = now - state.lastActivityMs;
            if (elapsed >= kActivityHoldMs)
            {
                return 0;
            }
            return pdMS_TO_TICKS(kActivityHoldMs - elapsed);
        }
        return portMAX_DELAY;
    }

    case StatusLedCommand::Off:
    case StatusLedCommand::PulseNetworkActivity:
    case StatusLedCommand::WaitingForClient:
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
        // Сама фаза мигания считается из millis() в applyStatusLedState,
        // здесь только сбрасываем метку, если активность устарела.
        if (state.lastActivityMs && !activityRecent(state, millis()))
        {
            state.lastActivityMs = 0;
        }
        break;

    case StatusLedCommand::Off:
    case StatusLedCommand::PulseNetworkActivity:
    case StatusLedCommand::WaitingForClient:
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
        const TickType_t waitTicks = statusLedWaitTicks(state);

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

    if (!initStatusLedBackend())
    {
        return;
    }

    statusLedQueue = xQueueCreate(kStatusLedQueueLength, sizeof(StatusLedCommand));
    if (!statusLedQueue)
    {
        if (statusLedPwmReady)
        {
            ledcDetachPin(STATUS_LED_BOARD_PIN);
            statusLedPwmReady = false;
        }
        statusLedBackend = StatusLedBackend::None;
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
        if (statusLedPwmReady)
        {
            ledcDetachPin(STATUS_LED_BOARD_PIN);
            statusLedPwmReady = false;
        }
        statusLedBackend = StatusLedBackend::None;
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
