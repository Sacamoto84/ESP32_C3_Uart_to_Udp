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

// Для использования встроенного светодиода платы добавьте флаг:
// -DPROJECT_USE_BOARD_LED=1
#if defined(PROJECT_USE_BOARD_LED)
#define PROJECT_HAS_BOARD_LED 1
#else
#define PROJECT_HAS_BOARD_LED 0
#endif

// Для переопределения полярности светодиода добавьте:
// -DPROJECT_BOARD_LED_ACTIVE_LOW=1  -> LED светит при уровне LOW
// -DPROJECT_BOARD_LED_ACTIVE_LOW=0  -> LED светит при уровне HIGH

#if PROJECT_HAS_SCREEN
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

#define FW_VERSION "1.6.0"

// Параметры платы и назначение пинов зависят от выбранного environment в PlatformIO.
#if defined(HW_VARIANT_ESP32_S2_MINI)
#define BOARD_LABEL "ESP32-S2 Mini"
#define OLED_USE_I2C 1
#define OLED_SDA_PIN 18
#define OLED_SCL_PIN 33
#define OLED_I2C_ADDR 0x3C
#define OLED_RESET_PIN -1
#define UART_TX_PIN 5
#define UART_RX_PIN 3
#define BOOT_HIGH_PIN 35
#define BOOT_LOW_PIN 37
#define AP_MODE_PIN -1
#define RESET_PULSE_PIN BOOT_LOW_PIN
#define STATUS_LED_BOARD_PIN 15
#define STATUS_LED_ACTIVE_LOW_DEFAULT 1
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
#define STATUS_LED_ACTIVE_LOW_DEFAULT 1
#else
#error "Unsupported hardware variant. Add a build flag for the target board."
#endif

#if defined(PROJECT_BOARD_LED_ACTIVE_LOW)
#define STATUS_LED_ACTIVE_LOW PROJECT_BOARD_LED_ACTIVE_LOW
#else
#define STATUS_LED_ACTIVE_LOW STATUS_LED_ACTIVE_LOW_DEFAULT
#endif

// Значения по умолчанию для Wi-Fi.
#define AP_SSID "TP-Link_BC0C"
#define AP_PASS "58133514"

#endif
