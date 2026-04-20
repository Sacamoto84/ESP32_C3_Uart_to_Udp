#include "display.h"
#include "hardware.h"
#include "network_bridge.h"
#include "settings_portal.h"
#include "uart_bridge.h"

// Порядок важен:
// сначала базовое железо и FS, потом дисплей и сеть, и только после этого
// стартуем портал настроек и UART-задачу.
void setup()
{
    initPins();
    initSerialAndFS();
    initDisplay();
    initWiFi();
    initSettings();
    initUART();
    initUDP();
}
