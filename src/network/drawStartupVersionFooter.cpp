#include "network_internal.h"

#if PROJECT_HAS_SCREEN

// На стартовом экране снизу всегда показываем версию прошивки.
void drawStartupVersionFooter()
{
    int16_t cursorX = display.getCursorX();
    int16_t cursorY = display.getCursorY();

    display.fillRect(0, SCREEN_HEIGHT - 8, SCREEN_WIDTH, 8, SSD1306_BLACK);
    display.setCursor(0, SCREEN_HEIGHT - 8);
    display.print("v");
    display.print(FW_VERSION);
    display.setCursor(cursorX, cursorY);
}

#endif
