#include "display.h"
#include "eeprom.h"
#include "hardware.h"
#include "command_cli.h"
#include "network_bridge.h"
#include "settings_portal.h"
#include "status_led.h"
#include "uart_bridge.h"

#include <esp_heap_caps.h>
#include "esp32-hal-psram.h"

// Порядок запуска здесь важен: сначала поднимаем низкоуровневые сервисы,
// потом сеть и интерфейс, и только после этого задачи, которые зависят от инициализации стека.
void setup()
{
    initPins();
    initSerialAndFS();
    // База настроек должна существовать до любого модуля, читающего db.get(...).
    EEPROM::getInstance();
    initStatusLed();
    initDisplay();
    initWiFi();
    initSettings();
    initCommandCli();
    initTcpServer();
    initUDP();
    initMdns();
    initOTA();
    initUART();
}
