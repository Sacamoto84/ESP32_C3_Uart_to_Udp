#ifndef _define_h
#define _define_h

#include <Arduino.h>
#include <ArduinoOTA.h>

#include <SettingsGyver.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

#define FW_VERSION "1.5.6"

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
#else
#error "Unsupported hardware variant. Add a build flag for the target board."
#endif

//MARK: Define
#define AP_SSID "TP-Link_BC0C"
#define AP_PASS "58133514"

extern QueueHandle_t uartQueue;
extern GyverDBFile db;
extern void uartTask(void* arg);
extern void sendUdpMessage(const char* msg, const char* ip);
extern void build(sets::Builder &b);

// Функции инициализации из setup.cpp
extern void initPins();
extern void initSerialAndFS();
extern void initDisplay();
extern void initWiFi();
extern void initSettings();
extern void initUART();
extern void initUDP();
extern void setup();

// Функция экрана
extern void screenLoop();

#endif
