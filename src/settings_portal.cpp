#include "settings_portal.h"

#include "app_globals.h"
#include "display.h"
#include "network_bridge.h"

#include <lwip/inet.h>
#include <esp_wifi.h>

namespace
{
// Набор кнопок для выбора фиксированной мощности передатчика Wi-Fi.
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

// Кнопка активной мощности подсвечивается, остальные серые.
void addWifiPowerButton(sets::Builder &b, size_t id, const WifiPowerOption &option, int currentPower)
{
    uint32_t color = (currentPower != option.dbValue) ? 0x808080 : 0xd55f30;
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

// Простая валидация IPv4-адреса перед сохранением в БД.
bool isValidIp(const char *ip)
{
    struct in_addr addr;
    return inet_aton(ip, &addr);
}

// Унифицированное сохранение IPv4-настроек в БД.
// Если адрес некорректный, просто не применяем его и оставляем старое значение.
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
} // namespace

void initSettings()
{
    // Запускаем SettingsGyver и привязываем callback,
    // который строит всю страницу настроек.
    sett.begin();
    sett.onBuild(build);
}

// Здесь описан весь веб-интерфейс портала настроек.
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

    // Для Input нужен изменяемый буфер, поэтому берём временную строку.
    String tempIpClient = db.get(kk::ipClient);
    if (b.Input("Ip Адрес клиента", &tempIpClient))
    {
        Serial.println(b.build.value);
        const char *ip = b.build.value.c_str();
        if (isValidIp(ip))
        {
            Serial.printf("Correct IP: %s\n", ip);
            db.set(kk::ipClient, ip);
            db.update();
            sett.reload(true);
        }
    }

    b.Number(kk::Serial2Bitrate, "Битрейт", nullptr, 300, 4000000);
    b.Switch(kk::broadcast, "Броадкаст");
    b.Switch(kk::echo, "Эхо");

#if PROJECT_HAS_SCREEN
    // Яркость задаётся напрямую в диапазоне 0..255,
    // чтобы без преобразований писать её в контроллер дисплея.
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
        // Группа Wi-Fi-настроек.
        sets::Group g(b, "WiFi");
        b.Input(kk::WIFI_SSID, "SSID");
        b.Input(kk::WIFI_PASS, "Password");
        if (b.Switch(kk::useStaticIp, "Использовать static IP"))
        {
            sett.reload(true);
        }

        if (db.get(kk::useStaticIp))
        {
            // Для реальной статической конфигурации недостаточно одного IP,
            // поэтому даём настроить адрес устройства, шлюз и маску сети.
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

    // Полная очистка БД с настройками.
    if (b.Button("Очистка базы", 0x25b18f))
    {
        Serial.println("Очистка базы");
        db.clear();
        db.update();
    }

    {
        // Отдельное меню выбора мощности Wi-Fi.
        sets::Menu m(b, "Мощность Wifi");
        b.enterMenu();
        int currentPower = db.get(kk::wifiPower);
        b.Label("Мощность", WifiCurrentPowerString(WiFi.getTxPower()));
        for (size_t i = 0; i < sizeof(kWifiPowerOptions) / sizeof(WifiPowerOption); ++i)
        {
            addWifiPowerButton(b, 1000 + i, kWifiPowerOptions[i], currentPower);
        }
    }

    b.Label("Версия " FW_VERSION);
}
