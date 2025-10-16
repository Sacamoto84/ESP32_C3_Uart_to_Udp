#ifndef _define_h
#define _define_h

#include <Arduino.h>
#include <ArduinoOTA.h>

#include <SettingsGyver.h>

#include "mString.h"

#include <GyverDBFile.h>

#include "eeprom.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "driver/uart.h"

//MARK: Define
#define AP_SSID "TP-Link_BC0C"
#define AP_PASS "58133514"

extern QueueHandle_t uartQueue;

extern GyverDBFile db;

extern void uartTask(void* arg);

extern void sendUdpMessage(const char* msg, const char* ip);



#endif