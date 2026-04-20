#include "app_globals.h"

// Общие объекты вынесены в отдельный translation unit,
// чтобы не размазывать определения по main.cpp и setup.cpp.
WiFiUDP udp;
QueueHandle_t uartQueue;

// База настроек и веб-портал живут всё время работы прошивки.
GyverDBFile db(&LittleFS, "/data.db", 500);
SettingsGyver sett("My Settings", &db);

// Конфигурация дисплея зависит от варианта железа.
#if PROJECT_HAS_SCREEN
#if OLED_USE_I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
#else
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI_PIN, OLED_CLK_PIN, OLED_DC_PIN, OLED_RESET_PIN, OLED_CS_PIN);
#endif
#endif
