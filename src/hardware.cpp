#include "hardware.h"

#include <LittleFS.h>

namespace
{
constexpr uint32_t kUsbSerialBaud = 460800;
constexpr uint32_t kResetPulseLowMs = 100;

// Даёт USB Serial Monitor короткое окно на подключение, чтобы ранние логи не потерялись.
void waitForBootSerialMonitor()
{
#if BOOT_SERIAL_DELAY_MS > 0
    const uint32_t startedAt = millis();

#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
    while (!Serial && (millis() - startedAt) < (uint32_t)BOOT_SERIAL_DELAY_MS)
    {
        delay(10);
    }

    if (Serial)
    {
        delay(200);
    }
#else
    delay(BOOT_SERIAL_DELAY_MS);
#endif

    Serial.printf("Boot serial wait done after %u ms, limit=%u ms, connected=%s\n",
                  (unsigned)(millis() - startedAt),
                  (unsigned)BOOT_SERIAL_DELAY_MS,
                  Serial ? "yes" : "no");
#endif
}
}

// Выставляет безопасные начальные режимы пинов для текущего варианта платы.
void initPins()
{
#if defined(HW_VARIANT_ESP32_S2_MINI)
    pinMode(BOOT_LOW_PIN, OUTPUT);
    digitalWrite(BOOT_LOW_PIN, LOW);

    pinMode(BOOT_HIGH_PIN, OUTPUT);
    digitalWrite(BOOT_HIGH_PIN, HIGH);

    delay(200);
#endif

    // Держим линию сброса внешнего устройства отпущенной сразу на старте,
    // чтобы подключённое оборудование не перезапускалось при каждом ребуте ESP32.
    // Остальные пины (OLED, UART, LED) настраиваются своими драйверами позже.
    pinMode(RESET_PULSE_PIN, OPEN_DRAIN);
    digitalWrite(RESET_PULSE_PIN, HIGH);
}

// Запускает логирование и монтирует LittleFS, где лежит база настроек.
void initSerialAndFS()
{
    Serial.begin(kUsbSerialBaud);
    waitForBootSerialMonitor();
    LittleFS.begin(true);
}

// Выполняет низкий импульс сброса на RESET_PULSE_PIN для перезагрузки внешнего устройства.
void pulseResetLine()
{
    pinMode(RESET_PULSE_PIN, OUTPUT);
    digitalWrite(RESET_PULSE_PIN, LOW);
    delay(kResetPulseLowMs);
    pinMode(RESET_PULSE_PIN, OPEN_DRAIN);
    digitalWrite(RESET_PULSE_PIN, HIGH);
}

// Возвращает true, если Wi-Fi сейчас в режиме точки доступа (AP или APSTA).
bool isAccessPointMode()
{
    const wifi_mode_t mode = WiFi.getMode();
    return (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
}
