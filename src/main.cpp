#include "app_globals.h"

#include "display.h"
#include "network_bridge.h"

#include <GTimer.h>

// Держит главный цикл коротким: обслуживает OTA, внутренности настроек/БД,
// heartbeat-ответы и активный режим отображения.
void loop()
{
    // OTA нужно обслуживать именно в loop, иначе сессия обновления откроется,
    // но прошивка не начнёт принимать входящие данные.
    tickOTA();
    sett.tick();
    db.tick();
    handleHeartbeatUdp();

#if PROJECT_HAS_SCREEN
    // Локальный экран состояния обновляем периодически, а не на каждой итерации loop.
    static GTimer<millis> statusScreenTimer(2000, true);

    bool externalScreenEnabled = db.get(kk::externalScreen);
    if (externalScreenEnabled)
    {
        // Когда включён внешний экран, локальный OLED работает как сырой буфер кадра,
        // который обновляется входящими UDP-пакетами.
        handleExternalScreenUdp();
        return;
    }

    if (statusScreenTimer)
    {
        // Иначе показываем обычный встроенный экран состояния.
        screenLoop();
    }
#endif
}
