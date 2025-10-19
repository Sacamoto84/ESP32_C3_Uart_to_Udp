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

#include "driver/uart.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI 6
#define OLED_CLK 4
#define OLED_DC 3
#define OLED_CS 1
#define OLED_RESET 2

//MARK: Define
#define AP_SSID "TP-Link_BC0C"
#define AP_PASS "58133514"

extern QueueHandle_t uartQueue;

extern GyverDBFile db;

extern void uartTask(void* arg);

extern void sendUdpMessage(const char* msg, const char* ip);

#endif