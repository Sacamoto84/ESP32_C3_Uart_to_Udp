#include "status_led.h"

#include "eeprom.h"

#include <freertos/queue.h>
#include <freertos/task.h>

#if defined(HW_VARIANT_ESP32_C3)
#include <esp32-hal-rgb-led.h>
#endif

namespace
{
constexpr uint32_t kConnectingBlinkMs = 350;
constexpr uint32_t kAccessPointBlinkMs = 120;
constexpr uint32_t kActivityBlinkHalfMs = 25;
constexpr uint32_t kActivityHoldMs = 120;
constexpr uint8_t kStatusLedTaskCore = 0;
constexpr UBaseType_t kStatusLedTaskPriority = 3;
constexpr uint16_t kStatusLedTaskStack = 2048;
constexpr uint8_t kStatusLedQueueLength = 8;
constexpr uint32_t kStatusLedPwmFreq = 5000;
constexpr uint8_t kStatusLedPwmResolution = 8;
constexpr uint32_t kStatusLedPwmMaxDuty = (1UL << kStatusLedPwmResolution) - 1;
constexpr uint8_t kStatusLedPwmChannel = 0;

// Выбирает физический драйвер светодиода для текущей платы.
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

// Изменяемое состояние, которым владеет фоновая LED-задача.
struct StatusLedState
{
    StatusLedCommand mode = StatusLedCommand::Off;
    bool blinkState = false;
    uint32_t lastActivityMs = 0;
};

// Читает из настроек, разрешена ли индикация светодиодом.
bool isStatusLedEnabled()
{
    return db.get(kk::statusLedEnabled);
}

// Читает полярность LED, чтобы одна и та же логика работала на платах с инверсной логикой.
bool isStatusLedActiveLow()
{
    return db.get(kk::statusLedActiveLow);
}

// Возвращает уровень GPIO, который включает светодиод при текущей полярности.
uint8_t statusLedOnLevel()
{
    return isStatusLedActiveLow() ? LOW : HIGH;
}

// Возвращает уровень GPIO, который выключает светодиод при текущей полярности.
uint8_t statusLedOffLevel()
{
    return isStatusLedActiveLow() ? HIGH : LOW;
}

// Ограничивает сохранённую яркость допустимым PWM-диапазоном.
uint32_t getConfiguredStatusLedBrightness()
{
    const int brightness = db.get(kk::statusLedBrightness);

    if (brightness < kStatusLedBrightnessMin)
    {
        return kStatusLedBrightnessMin;
    }

    if (brightness > kStatusLedBrightnessMax)
    {
        return kStatusLedBrightnessMax;
    }

    return (uint32_t)brightness;
}

// Преобразует сохранённую яркость в коэффициент заполнения для состояния "включено".
uint32_t statusLedOnDuty()
{
    const uint32_t brightness = getConfiguredStatusLedBrightness();

    return (brightness > kStatusLedPwmMaxDuty) ? kStatusLedPwmMaxDuty : brightness;
}

// Переводит логическое включение и выключение в реальное PWM-значение с учётом инверсной логики.
uint32_t statusLedRawDuty(bool enabled)
{
    const uint32_t onDuty = statusLedOnDuty();

    if (isStatusLedActiveLow())
    {
        return enabled ? (kStatusLedPwmMaxDuty - onDuty) : kStatusLedPwmMaxDuty;
    }

    return enabled ? onDuty : 0;
}

// Применяет нужное состояние LED через активный драйвер.
void writeStatusLed(bool enabled)
{
    const bool outputEnabled = enabled && isStatusLedEnabled();

    switch (statusLedBackend)
    {
    case StatusLedBackend::NeoPixel:
#if defined(HW_VARIANT_ESP32_C3)
    {
        const uint8_t level = outputEnabled ? static_cast<uint8_t>(statusLedOnDuty()) : 0;
        neopixelWrite(STATUS_LED_BOARD_PIN, level, level, level);
        break;
    }
#else
        break;
#endif

    case StatusLedBackend::PwmGpio:
        if (statusLedPwmReady)
        {
            ledcWrite(kStatusLedPwmChannel, statusLedRawDuty(outputEnabled));
        }
        break;

    case StatusLedBackend::DigitalGpio:
    {
        const bool effectiveEnabled = outputEnabled && statusLedOnDuty() > 0;
        digitalWrite(STATUS_LED_BOARD_PIN, effectiveEnabled ? statusLedOnLevel() : statusLedOffLevel());
        break;
    }

    case StatusLedBackend::None:
    default:
        break;
    }
}

// Определяет и инициализирует лучший доступный драйвер светодиода для этой платы.
bool initStatusLedBackend()
{
#if defined(HW_VARIANT_ESP32_C3)
    statusLedBackend = StatusLedBackend::NeoPixel;
    writeStatusLed(false);
    return true;
#else
    pinMode(STATUS_LED_BOARD_PIN, OUTPUT);

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

// Возвращает период мигания для выбранного режима LED.
uint32_t currentBlinkPeriod(StatusLedCommand mode)
{
    return (mode == StatusLedCommand::AccessPoint) ? kAccessPointBlinkMs : kConnectingBlinkMs;
}

// Проверяет, считается ли последнее сетевое событие ещё "свежим".
bool activityRecent(const StatusLedState &state, uint32_t now)
{
    return state.lastActivityMs && (now - state.lastActivityMs) < kActivityHoldMs;
}

// Отрисовывает текущее логическое состояние на физическом светодиоде.
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
        // Без трафика светодиод горит ровно. При активности мигает с фиксированной
        // частотой от millis(), чтобы фаза не зависела от того, как часто будится задача.
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
    case StatusLedCommand::RefreshBrightness:
        writeStatusLed(false);
        break;
    }
}

// Применяет входящую LED-команду к локальному состоянию LED-задачи.
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
        // Здесь только отмечаем факт недавней активности. Само мигание обслуживается
        // периодическим таймаутом, чтобы оно было видно даже при бурсте событий.
        if (state.mode == StatusLedCommand::ClientConnected)
        {
            state.lastActivityMs = millis();
        }
        break;

    case StatusLedCommand::RefreshBrightness:
        break;
    }
}

// Считает, сколько задача LED может спать до следующего перехода состояния.
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
            return pdMS_TO_TICKS(kActivityBlinkHalfMs);
        }
        if (state.lastActivityMs)
        {
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
    case StatusLedCommand::RefreshBrightness:
    case StatusLedCommand::WaitingForClient:
    default:
        return portMAX_DELAY;
    }
}

// Продвигает внутренние таймеры мигания и активности по таймауту ожидания очереди.
void onStatusLedTimeout(StatusLedState &state)
{
    switch (state.mode)
    {
    case StatusLedCommand::ConnectingToStation:
    case StatusLedCommand::AccessPoint:
        state.blinkState = !state.blinkState;
        break;

    case StatusLedCommand::ClientConnected:
        // Сама фаза мигания считается от millis() в applyStatusLedState.
        // Здесь только убираем устаревшую отметку активности после истечения окна удержания.
        if (state.lastActivityMs && !activityRecent(state, millis()))
        {
            state.lastActivityMs = 0;
        }
        break;

    case StatusLedCommand::Off:
    case StatusLedCommand::PulseNetworkActivity:
    case StatusLedCommand::RefreshBrightness:
    case StatusLedCommand::WaitingForClient:
    default:
        break;
    }
}

// Владеет всей машиной состояний светодиода и реагирует на команды и таймауты.
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
}

// Создаёт очередь и задачу, через которые дальше управляется весь LED.
void initStatusLed()
{
    if (statusLedQueue)
    {
        return;
    }

    Serial.printf("Status LED init: pin=%u enabled=%u active_low=%u brightness=%u\n",
                  (unsigned)STATUS_LED_BOARD_PIN,
                  (unsigned)isStatusLedEnabled(),
                  (unsigned)isStatusLedActiveLow(),
                  (unsigned)getConfiguredStatusLedBrightness());

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
}

// Добавляет команду в очередь LED и схлопывает переполнение для непульсовых команд.
void sendStatusLedCommand(StatusLedCommand command)
{
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
}
