#include "settings_portal.h"

#include "app_globals.h"
#include "display.h"
#include "network_bridge.h"

#include <esp_wifi.h>
#include <lwip/inet.h>

namespace
{
// Набор кнопок выбора фиксированной мощности Wi-Fi.
struct WifiPowerOption
{
    int dbValue;
    const char *label;
};

constexpr WifiPowerOption kWifiPowerOptions[] = {
    {-4, "-1dbm (-4)"},
    {8, "2dBm (8)"},
    {34, "8.5dBm (34)"},
    {44, "11dBm (44)"},
    {52, "13dBm (52)"},
    {60, "15dBm (60)"},
    {68, "17dBm (68)"},
    {76, "19dBm (76)"},
    {78, "19.5dBm (78)"},
};

// Подсвечиваем только активную мощность, чтобы в меню было легко ориентироваться.
void addWifiPowerButton(sets::Builder &b, size_t id, const WifiPowerOption &option, int currentPower)
{
    const uint32_t color = (currentPower != option.dbValue) ? 0x808080 : 0xd55f30;
    if (b.Button(id, option.label, color))
    {
        Serial.print("Set TX power to ");
        Serial.println(option.label);
        db.set(kk::wifiPower, option.dbValue);
        db.update();
        WiFi.setTxPower((wifi_power_t)option.dbValue);
        b.reload();
    }
}

// Простая проверка IPv4 перед сохранением в БД.
bool isValidIp(const char *ip)
{
    struct in_addr addr;
    return inet_aton(ip, &addr);
}

// Унифицированная обработка IP-полей для static IP режима.
template <typename TKey>
void handleIpInput(sets::Builder &b, TKey key, const char *label)
{
    String tempValue = db.get(key);
    if (!b.Input(label, &tempValue))
    {
        return;
    }

    const char *ip = b.build.value.c_str();
    if (!isValidIp(ip))
    {
        Serial.printf("Invalid IP for %s: %s\n", label, ip);
        sett.reload(true);
        return;
    }

    Serial.printf("Saved %s: %s\n", label, ip);
    db.set(key, ip);
    db.update();
    sett.reload(true);
}

// В STA показываем обычный IP, в AP - адрес точки доступа.
IPAddress currentPortalIp()
{
    const wifi_mode_t wifiMode = WiFi.getMode();
    const bool apMode = (wifiMode == WIFI_MODE_AP || wifiMode == WIFI_MODE_APSTA);
    return apMode ? WiFi.softAPIP() : WiFi.localIP();
}
} // namespace

void initSettings()
{
    // Инициализируем веб-портал настроек и привязываем callback,
    // который полностью строит интерфейс страницы.
    sett.begin(true);
    sett.onBuild(build);
}

// Здесь описан весь UI настроек проекта.
void build(sets::Builder &b)
{
    Serial.println("build");

#if PROJECT_HAS_SCREEN
    if (b.build.isAction() && b.build.id == kk::screenBrightness)
    {
        String rawBrightness = b.build.value;
        Serial.print("[Brightness] action received, raw value: ");
        Serial.println(rawBrightness);
    }
#endif

    b.Number(kk::Serial2Bitrate, "Битрейт", nullptr, 300, 4000000);

    const String currentIpLabel = "ESP32 IP: " + currentPortalIp().toString();
    b.Label(currentIpLabel.c_str());
    const String otaLabel = "OTA: " + String(PROJECT_DEVICE_HOSTNAME) + ".local:" + String(PROJECT_OTA_PORT);
    b.Label(otaLabel.c_str());
    b.Label(PROJECT_OTA_PASSWORD_VALUE[0] ? "OTA auth: enabled" : "OTA auth: disabled");

    {
        b.Label("Режим транспорта: TCP server");
        b.Label("Android подключается к этому ESP32 как TCP client");
        b.Label("Порт TCP сервера: 8888");

        String queueLabel = "Очередь UART->TCP: " + String(getNetworkTxQueueCapacity()) +
                            " x " + String(NETWORK_TX_CHUNK_SIZE) + " байт";
        b.Label(queueLabel.c_str());
        if (getNetworkTxQueueCapacity() == 0)
        {
            b.Label("Очередь не создалась, проверьте лог и уменьшите размеры буферов");
        }
        else
        {
            b.Label("При переполнении новые данные дропаются без краша прошивки");
        }
    }

    b.Switch(kk::echo, "Эхо");

#if PROJECT_HAS_SCREEN
    // Яркость задаётся напрямую в диапазоне 0..255,
    // чтобы без преобразований писать значение в контроллер дисплея.
    if (b.Number(kk::screenBrightness, "Яркость экрана", nullptr, 0, 255))
    {
        int brightness = constrain((int)db.get(kk::screenBrightness), 0, 255);

        Serial.print("[Brightness] number callback, value: ");
        Serial.println(brightness);

        applyDisplayBrightness((uint8_t)brightness);

        bool saved = db.update();
        Serial.print("[Brightness] db.update result: ");
        Serial.println(saved ? "true" : "false");
        Serial.print("[Brightness] applied value: ");
        Serial.println(brightness);
        b.reload();
        sett.reload(true);
    }
#endif

    {
        // Блок Wi-Fi настроек: SSID/пароль и опциональный static IP.
        sets::Group g(b, "WiFi");
        b.Input(kk::WIFI_SSID, "SSID");
        b.Input(kk::WIFI_PASS, "Password");

        if (b.Switch(kk::useStaticIp, "Использовать static IP"))
        {
            sett.reload(true);
        }

        if (db.get(kk::useStaticIp))
        {
            // Для static IP нужен не только адрес ESP32,
            // но и gateway плюс маска сети.
            handleIpInput(b, kk::staticIp, "Static IP");
            handleIpInput(b, kk::staticGateway, "Gateway");
            handleIpInput(b, kk::staticSubnet, "Subnet mask");
            b.Label("Static IP применяется после перезагрузки ESP32");
        }
    }

#if PROJECT_HAS_SCREEN
    b.Switch(kk::externalScreen, "Внешний экран по UDP 82 порту");
#endif

    if (b.Button("Сброс ESP32"))
    {
        ESP.restart();
    }

    // Аппаратный импульс сброса на внешнее устройство.
    if (b.Button("Выход -> Сброс", 0x25b18f))
    {
        Serial.println("Выход -> Сброс");
        pinMode(RESET_PULSE_PIN, OUTPUT);
        digitalWrite(RESET_PULSE_PIN, LOW);
        delay(100);
        pinMode(RESET_PULSE_PIN, OPEN_DRAIN);
        digitalWrite(RESET_PULSE_PIN, HIGH);
    }

    // Полная очистка БД настроек.
    if (b.Button("Очистка базы", 0x25b18f))
    {
        Serial.println("Очистка базы");
        db.clear();
        db.update();
    }

    {
        // Отдельное меню выбора мощности Wi-Fi передатчика.
        sets::Menu m(b, "Мощность Wifi");
        b.enterMenu();
        const int currentPower = db.get(kk::wifiPower);
        b.Label("Мощность", WifiCurrentPowerString(WiFi.getTxPower()));
        for (size_t i = 0; i < sizeof(kWifiPowerOptions) / sizeof(WifiPowerOption); ++i)
        {
            addWifiPowerButton(b, 1000 + i, kWifiPowerOptions[i], currentPower);
        }
    }

    b.Label("Версия " FW_VERSION);
}
