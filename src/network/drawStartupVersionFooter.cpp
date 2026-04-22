#include "network_internal.h"

#if PROJECT_HAS_SCREEN

namespace
{
constexpr const uint8_t *kFooterFont = u8g2_font_6x10_tr;
constexpr uint8_t kFooterY = SCREEN_HEIGHT - 10;
}

// Draw firmware version at the bottom of the startup screen.
// This helper is intended to be called as the last drawing step before sendBuffer().
void drawStartupVersionFooter()
{
    display.setDrawColor(0);
    display.drawBox(0, kFooterY, SCREEN_WIDTH, SCREEN_HEIGHT - kFooterY);

    display.setDrawColor(1);
    display.setFont(kFooterFont);
    display.setFontMode(1);
    display.setFontPosTop();
    display.setCursor(0, kFooterY);
    display.print("v");
    display.print(FW_VERSION);
}

#endif
