#include "app_globals.h"

#include "display.h"
#include "network_bridge.h"

#include <GTimer.h>

// Основной цикл максимально короткий:
// крутим SettingsGyver, обслуживаем БД и обновляем нужный режим экрана.
void loop()
{
    // OTA must be serviced in the main loop, otherwise PlatformIO can open
    // the OTA session but the firmware will not consume incoming data.
    tickOTA();
    sett.tick();
    db.tick();

#if PROJECT_HAS_SCREEN
    // Локальный экран состояния обновляем не каждый цикл,
    // а раз в 2 секунды, чтобы не тратить время впустую.
    static GTimer<millis> statusScreenTimer(2000, true);

    bool externalScreenEnabled = db.get(kk::externalScreen);
    if (externalScreenEnabled)
    {
        // Когда включён внешний экран, локальный OLED работает как framebuffer,
        // который целиком обновляется из входящих UDP-пакетов.
        handleExternalScreenUdp();
        return;
    }

    if (statusScreenTimer)
    {
        // Иначе показываем обычный экран состояния устройства.
        screenLoop();
    }
#endif

}
