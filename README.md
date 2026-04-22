# ESP32 UART to TCP Bridge

![ESP32 UART to TCP Bridge](images/img0.png)

Прошивка превращает ESP32 в сетевой мост `UART -> TCP`, чтобы Android-приложение `TerminalM3` могло подключаться к устройству как TCP-клиент, получать поток данных из UART и показывать его в терминале.

Актуальная схема работы такая:

- ESP32 поднимает `TCP server` на порту `8888`
- Android сам подключается к ESP32 как `TCP client`
- `UDP` больше не используется для передачи UART-данных
- `UDP` оставлен только для двух служебных задач:
  - heartbeat `ping/pong` на порту `8888`
  - внешний экран OLED framebuffer на порту `82`, если эта функция включена

## Что умеет проект

- Передавать поток данных из UART в TCP-соединение без UDP-broadcast.
- Держать отдельную очередь `UART -> TCP`, чтобы медленная сеть не блокировала чтение UART.
- Автоматически использовать `PSRAM` под сетевую очередь, если она есть.
- Поднимать web-портал настроек на ESP32.
- Работать с `DHCP` или `static IP`.
- Публиковать сервисы через `mDNS`.
- Поддерживать `OTA`-обновление.
- Показывать локальный статус на OLED, если сборка с экраном.
- Работать в режиме точки доступа `AP ESP32`, если подключение к Wi-Fi не удалось.

## Актуальная архитектура

### 1. Передача данных UART

Основной транспорт теперь только один:

```text
UART -> очередь -> TCP server:8888 -> Android TCP client
```

ESP32 не рассылает UART-поток через UDP-broadcast. Это удалено из проекта.

### 2. Heartbeat между Android и ESP32

Для контроля соединения используется отдельный `UDP heartbeat`.

- Android шлет на ESP32 строку вида:

```text
tm3 hb ping seq=123
```

- ESP32 отвечает обратно на IP и порт отправителя:

```text
tm3 hb pong seq=123
```

Текущие параметры по коду:

- UDP порт heartbeat: `8888`
- Android считает соединение пропавшим, если `pong` не приходит примерно `3` секунды
- в Android heartbeat сейчас запускается отдельным клиентом и используется как индикатор живого соединения

### 3. Внешний экран по UDP

Если сборка идет с экраном и в настройках включен внешний экран, ESP32 слушает UDP-порт `82` и принимает framebuffer для OLED.

Это отдельный режим и он не связан с передачей UART в Android.

## Сетевые порты и сервисы

### Порты

- `TCP 8888` - основной сервер UART -> Android
- `UDP 8888` - heartbeat `ping/pong`
- `UDP 82` - внешний экран OLED framebuffer, если включен
- `TCP 80` - web-портал настроек
- `TCP 3232` - OTA

### mDNS

ESP32 публикует:

- `http.tcp` на порту `80`
- `uartbridge.tcp` на порту `8888`
  - TXT: `transport=tcp-server`
  - TXT: `board=<имя платы>`
- `external-screen.udp` на порту `82`, если есть экран и включен внешний экран

Имя хоста задается через `PROJECT_OTA_HOSTNAME`, либо берется по умолчанию:

- `esp32-c3-uart`
- `esp32-s2-uart`

## Поддерживаемые платы

## ESP32-C3

Environment в `PlatformIO`:

```text
esp32-c3-devkitm-1
esp32-c3-devkitm-1_ota
```

Пины:

- `UART TX = GPIO21`
- `UART RX = GPIO20`
- `AP_MODE_PIN = GPIO8`
- `RESET_PULSE_PIN = GPIO9`
- встроенный LED: `GPIO8`

OLED `SSD1306` подключен по `SPI`:

```text
GPIO6  -> MOSI
GPIO4  -> CLK
GPIO3  -> DC
GPIO1  -> CS
GPIO2  -> RESET
```

## ESP32-S2 Mini

Environment в `PlatformIO`:

```text
lolin_s2_mini
lolin_s2_mini_ota
```

Пины:

- `UART TX = GPIO5`
- `UART RX = GPIO3`
- `RESET_PULSE_PIN = GPIO9`
- встроенный LED: `GPIO15`
- `BOOT_HIGH_PIN = GPIO36`
- `BOOT_LOW_PIN = GPIO38`

OLED `SSD1306` подключен по `I2C`:

```text
SDA   -> GPIO21
SCL   -> GPIO34
ADDR  -> 0x3C
RESET -> -1
```

## Порядок инициализации

В `setup()` проект стартует в таком порядке:

1. `initPins()`
2. `initSerialAndFS()`
3. `initStatusLed()`
4. `initDisplay()`
5. `initWiFi()`
6. `initSettings()`
7. `initTcpServer()`
8. `initUDP()`
9. `initMdns()`
10. `initOTA()`
11. `initUART()`

Такой порядок важен: сначала железо и сеть, потом web-портал, TCP и только после этого UART-задача.

## Как работает цикл loop()

В основном цикле выполняется:

- `tickOTA()`
- `sett.tick()`
- `db.tick()`
- `handleHeartbeatUdp()`

Если включен режим внешнего экрана, OLED обслуживается входящими UDP-пакетами framebuffer. Иначе локальный статусный экран обновляется примерно раз в `2` секунды.

## Wi-Fi логика

Прошивка работает так:

- стартует в режиме `STA`
- при включенном `static IP` сначала пытается применить адрес, шлюз и маску
- мощность Wi-Fi берется из настроек
- если подключение не удалось, делает повторную попытку с мощностью `8.5 dBm`
- если снова не удалось, переключается в `AP` режим
- имя точки доступа fallback-режима: `AP ESP32`

Перед инициализацией Wi-Fi частота CPU понижается до `80 MHz`.

## Очередь UART -> TCP и PSRAM

Проект использует очередь между UART и TCP, чтобы сеть не тормозила прием данных из UART.

Один элемент очереди:

```cpp
struct NetworkTxChunk {
    uint16_t len;
    uint8_t data[NETWORK_TX_CHUNK_SIZE];
};
```

Текущее поведение:

- сначала проект пытается создать очередь в `PSRAM`
- если не получилось, пытается создать очередь во внутренней RAM
- если памяти не хватает, автоматически уменьшает длину очереди в 2 раза, пока не найдет рабочий вариант
- если очередь так и не создалась, TCP server все равно стартует, чтобы это было видно в диагностике

Текущие compile-time размеры:

- `PROJECT_NETWORK_TX_CHUNK_SIZE = 1460`
- `PROJECT_NETWORK_TX_QUEUE_LENGTH = 64` для `ESP32-C3`
- `PROJECT_NETWORK_TX_QUEUE_LENGTH = 256` для `ESP32-S2 Mini`

Дополнительно:

- драйверный RX-буфер UART: `64 KB`
- локальный буфер чтения из UART: `NETWORK_TX_CHUNK_SIZE * 4`
- таймаут записи TCP: `3000 ms`

Если очередь переполняется, новые данные дропаются без краша прошивки, а в лог пишется счетчик потерь.

## Web-портал настроек

Портал настроек поднимается на ESP32 и позволяет менять:

- bitrate UART
- SSID и пароль Wi-Fi
- включение `static IP`
- `static IP`, `gateway`, `subnet`
- `echo`
- мощность передатчика Wi-Fi
- яркость OLED, если экран есть
- включение внешнего экрана по UDP `82`, если экран есть
- перезагрузку ESP32
- аппаратный импульс сброса на внешнее устройство
- очистку базы настроек

В портале также показываются:

- текущий IP ESP32
- строка OTA вида `hostname.local:3232`
- состояние OTA auth
- режим транспорта: `TCP server`
- фактический размер очереди `UART -> TCP`

## OTA

Параметры по умолчанию:

- OTA порт: `3232`
- hostname для `ESP32-C3`: `esp32-c3-uart`
- hostname для `ESP32-S2 Mini`: `esp32-s2-uart`

Environment для OTA:

```text
esp32-c3-devkitm-1_ota
lolin_s2_mini_ota
```

Примеры:

```bash
platformio run --environment esp32-c3-devkitm-1
platformio run --environment esp32-c3-devkitm-1_ota --target upload
platformio run --environment lolin_s2_mini
platformio run --environment lolin_s2_mini_ota --target upload
```

## Полезные build flags

### Без экрана

```text
-DPROJECT_NO_SCREEN=1
```

### Встроенный LED платы

```text
-DPROJECT_USE_BOARD_LED=1
```

### Полярность LED

```text
-DPROJECT_BOARD_LED_ACTIVE_LOW=1
-DPROJECT_BOARD_LED_ACTIVE_LOW=0
```

### Яркость LED через PWM

```text
-DPROJECT_BOARD_LED_BRIGHTNESS=32
```

### Размеры сетевых буферов

```text
-DPROJECT_NETWORK_TX_CHUNK_SIZE=1460
-DPROJECT_NETWORK_TX_QUEUE_LENGTH=256
```

### Подробный лог UART hot path

Включать только для отладки, потому что заметно нагружает быстрый путь:

```text
-DPROJECT_UART_VERBOSE_LOG=1
```

### OTA hostname и пароль

```text
-DPROJECT_OTA_HOSTNAME="my-esp32-bridge"
-DPROJECT_OTA_PASSWORD="change-me"
```

## Запуск с Android TerminalM3

Android-приложение работает с этой прошивкой по такой схеме:

1. Android определяет IP ESP32 автоматически или берет его из ручной настройки.
2. Android открывает `TCP` соединение на `8888`.
3. Параллельно Android шлет heartbeat `ping` по `UDP 8888`.
4. Если heartbeat пропадает, Android считает сервер потерянным, роняет TCP-сокет и пытается переподключиться.

Это позволяет отдельно контролировать жив ли сервер, даже если TCP еще не успел сам отвалиться по таймаутам ОС.

## Актуальные замечания

- Передача UART по `UDP broadcast` удалена и больше не используется.
- Ключ `broadcast` в EEPROM оставлен только как legacy-след для совместимости базы настроек.
- README составлен по текущему состоянию кода, а не по старой архитектуре проекта.

## Связанные документы

- [Корневой README проекта Android TerminalM3](../README.md)
- [README консольных виджетов Android](../app/src/main/java/com/example/terminalm3/console/README.md)
- [README библиотеки TimberWidget для Arduino/PlatformIO](../TimberWidget/README.md)
