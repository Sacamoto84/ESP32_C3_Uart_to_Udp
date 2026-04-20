#include "network_internal.h"
#include "status_led.h"

// Функция инициализации WiFi.
void initWiFi()
{
    int count = 0;
    bool needAP = false;

    // Снижаем частоту CPU перед запуском WiFi.
    setCpuFrequencyMhz(80); // вместо 160 MHz

    WiFi.mode(WIFI_STA);

    // От слабого к сильному сигналу:
    // WIFI_POWER_MINUS_1dBm    // ~2 dBm   (~80 мА пик)
    // WIFI_POWER_2dBm          // ~5 dBm   (~100 мА пик)
    // WIFI_POWER_8_5dBm        // ~8.5 dBm (~150 мА пик) <- вы сейчас тут
    // WIFI_POWER_11dBm         // ~11 dBm  (~200 мА пик) <- попробуйте это
    // WIFI_POWER_13dBm         // ~13 dBm  (~250 мА пик)
    // WIFI_POWER_15dBm         // ~15 dBm  (~300 мА пик)
    // WIFI_POWER_17dBm         // ~17 dBm  (~350 мА пик)
    // WIFI_POWER_19dBm         // ~19 dBm  (~420 мА пик)
    // WIFI_POWER_19_5dBm       // ~20 dBm  (~480 мА пик) <- максимум

    // КРИТИЧНО: снижаем мощность TX,
    // это заметно уменьшает пиковое потребление при старте радиомодуля.
    int power = db.get(kk::wifiPower);
    WiFi.setTxPower((wifi_power_t)power);
    WiFi.begin(db.get(kk::WIFI_SSID), db.get(kk::WIFI_PASS));
    sendStatusLedCommand(StatusLedCommand::ConnectingToStation);

#if PROJECT_HAS_SCREEN
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.cp437(true);
    display.clearDisplay();
    display.print("Connecting ");
    display.println(WifiCurrentPowerString(WiFi.getTxPower()));
    drawStartupVersionFooter();
    display.display();
#endif

    while (WiFi.status() != WL_CONNECTED)
    {
#if PROJECT_HAS_SCREEN
        display.print(".");
        drawStartupVersionFooter();
        display.display();
#endif
        Serial.print(db.get(kk::wifiPower));
        Serial.print(".");
        delay(500);
        count++;

        if (count <= 20)
        {
            continue;
        }

        // Сеть не найдена.
#if PROJECT_HAS_SCREEN
        display.println("\nSet Power to 8.5 dBm");
        drawStartupVersionFooter();
        display.display();
#endif
        Serial.println("\nWifi STA не найдена");
        Serial.println("Снижаем мощность до 8.5dBm");

        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        WiFi.begin(db.get(kk::WIFI_SSID), db.get(kk::WIFI_PASS));
        count = 0;

        while (WiFi.status() != WL_CONNECTED)
        {
#if PROJECT_HAS_SCREEN
            display.print(".");
            drawStartupVersionFooter();
            display.display();
#endif
            Serial.print(".");
            delay(500);
            count++;

            if (count <= 20)
            {
                continue;
            }

            // Сеть не найдена, включаем fallback в режим точки доступа.
            Serial.println("Wifi STA не найдена");
            needAP = true;
            break;
        }

        break;
    }

    // Создаём точку доступа.
    if (needAP)
    {
#if PROJECT_HAS_SCREEN
        display.println("\nStart APmode");
        drawStartupVersionFooter();
        display.display();
#endif
        Serial.println("Запуск точки доступа");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("AP ESP32");
        sendStatusLedCommand(StatusLedCommand::AccessPoint);

        if (AP_MODE_PIN >= 0
#if PROJECT_HAS_BOARD_LED
            && AP_MODE_PIN != STATUS_LED_BOARD_PIN
#endif
        )
        {
            pinMode(AP_MODE_PIN, OUTPUT);
            digitalWrite(AP_MODE_PIN, LOW);
        }
    }
    else
    {
        sendStatusLedCommand(StatusLedCommand::StationConnected);
    }

#if PROJECT_HAS_SCREEN
    display.println("\nConnected");
    drawStartupVersionFooter();
    display.display();
#endif

    Serial.println();
    Serial.print("Connected: ");
    Serial.println(needAP ? WiFi.softAPIP() : WiFi.localIP());
}
