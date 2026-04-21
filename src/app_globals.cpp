#include "app_globals.h"

// Общие объекты вынесены в отдельный translation unit,
// чтобы не размазывать определения по main.cpp и setup.cpp.
WiFiUDP udp;
WiFiServer tcpServer(8888);
WiFiClient tcpClient;
QueueHandle_t uartQueue;
QueueHandle_t networkTxQueue;
volatile bool tcpClientConnected = false;
volatile uint32_t droppedNetworkTxBytes = 0;
volatile uint32_t actualNetworkTxQueueLength = 0;

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
