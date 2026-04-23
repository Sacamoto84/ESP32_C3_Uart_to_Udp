#ifndef _define_h
#define _define_h

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <SettingsGyver.h>

// Для безэкранной сборки добавьте флаг:
// -DPROJECT_NO_SCREEN=1
#if defined(PROJECT_NO_SCREEN)
#define PROJECT_HAS_SCREEN 0
#else
#define PROJECT_HAS_SCREEN 1
#endif

#if PROJECT_HAS_SCREEN
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#endif
#include "mString.h"
#include <GyverDBFile.h>
#include "eeprom.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "driver/gpio.h"
#include "driver/uart.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define FW_VERSION "1.6.7"

// Параметры платы и назначение пинов зависят от выбранного environment в PlatformIO.
#if defined(HW_VARIANT_ESP32_S2_MINI)
#define BOARD_LABEL "ESP32-S2 Mini"
#define OLED_USE_I2C 1
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 34
#define OLED_I2C_ADDR 0x3C
#define OLED_RESET_PIN -1
#define UART_TX_PIN 5
#define UART_RX_PIN 3
#define BOOT_HIGH_PIN 36                 // VCC для OLED
#define BOOT_LOW_PIN 38                  // GND для OLED
#define AP_MODE_PIN -1
#define RESET_PULSE_PIN 9                // Пин для сброса открытый коллектор
#define STATUS_LED_BOARD_PIN 15          // Светодиод на плате
#elif defined(HW_VARIANT_ESP32_C3)
#define BOARD_LABEL "ESP32-C3"
#define OLED_USE_I2C 0
#define OLED_MOSI_PIN 6
#define OLED_CLK_PIN 4
#define OLED_DC_PIN 3
#define OLED_CS_PIN 1
#define OLED_RESET_PIN 2
#define UART_TX_PIN 21
#define UART_RX_PIN 20
#define BOOT_HIGH_PIN -1
#define BOOT_LOW_PIN -1
#define AP_MODE_PIN 8
#define RESET_PULSE_PIN 9
#define STATUS_LED_BOARD_PIN 8
#else
#error "Unsupported hardware variant. Add a build flag for the target board."
#endif

#if PROJECT_HAS_SCREEN
#if OLED_USE_I2C
using OledDisplay = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
#else
using OledDisplay = U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI;
#endif

#if OLED_RESET_PIN < 0
#define OLED_RESET_U8G2_PIN U8X8_PIN_NONE
#else
#define OLED_RESET_U8G2_PIN OLED_RESET_PIN
#endif
#endif

#if defined(PROJECT_NETWORK_TX_CHUNK_SIZE)
#define NETWORK_TX_CHUNK_SIZE PROJECT_NETWORK_TX_CHUNK_SIZE
#else
#define NETWORK_TX_CHUNK_SIZE 1024
#endif

#if defined(PROJECT_NETWORK_TX_QUEUE_LENGTH)
#define NETWORK_TX_QUEUE_LENGTH PROJECT_NETWORK_TX_QUEUE_LENGTH
#else
#define NETWORK_TX_QUEUE_LENGTH 32
#endif

// Подробный лог каждого успешно обработанного UART-пакета заметно нагружает горячий путь.
// Включайте только для отладки:
// -DPROJECT_UART_VERBOSE_LOG=1
#if defined(PROJECT_UART_VERBOSE_LOG)
#define PROJECT_UART_VERBOSE_LOG_ENABLED 1
#else
#define PROJECT_UART_VERBOSE_LOG_ENABLED 0
#endif

// Wait for the USB serial monitor at boot so early diagnostics are visible.
// Override with -DPROJECT_BOOT_SERIAL_DELAY_MS=0 to disable.
#if defined(PROJECT_BOOT_SERIAL_DELAY_MS)
#define BOOT_SERIAL_DELAY_MS PROJECT_BOOT_SERIAL_DELAY_MS
#else
#define BOOT_SERIAL_DELAY_MS 5000
#endif

// Значения по умолчанию для Wi-Fi.
#define PROJECT_OTA_PORT 3232

#if defined(PROJECT_OTA_HOSTNAME)
#define PROJECT_DEVICE_HOSTNAME PROJECT_OTA_HOSTNAME
#elif defined(HW_VARIANT_ESP32_S2_MINI)
#define PROJECT_DEVICE_HOSTNAME "esp32-s2-uart"
#elif defined(HW_VARIANT_ESP32_C3)
#define PROJECT_DEVICE_HOSTNAME "esp32-c3-uart"
#else
#define PROJECT_DEVICE_HOSTNAME "esp32-uart"
#endif

#if defined(PROJECT_OTA_PASSWORD)
#define PROJECT_OTA_PASSWORD_VALUE PROJECT_OTA_PASSWORD
#else
#define PROJECT_OTA_PASSWORD_VALUE ""
#endif

#define AP_SSID "TP-Link_BC0C"
#define AP_PASS "58133514"

#endif
