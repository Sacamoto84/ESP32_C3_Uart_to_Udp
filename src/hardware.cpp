#include "hardware.h"

namespace
{
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
#else
    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
#if STATUS_LED_BOARD_PIN != 8
    pinMode(8, OUTPUT);
#endif
    pinMode(9, OPEN_DRAIN);
    pinMode(10, OUTPUT);
#endif
}

// Запускает логирование и монтирует LittleFS, где лежит база настроек.
void initSerialAndFS()
{
    Serial.begin(460800);
    waitForBootSerialMonitor();
    LittleFS.begin(true);
}
