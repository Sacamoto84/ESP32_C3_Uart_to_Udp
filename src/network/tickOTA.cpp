#include "network_internal.h"

// Обслуживает входящий OTA-трафик и связанные обработчики из главного цикла.
void tickOTA()
{
    ArduinoOTA.handle();
}
