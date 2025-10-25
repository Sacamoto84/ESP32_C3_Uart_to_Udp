#include "define.h"

#include "lwip/sockets.h"

#define BUF_SIZE 2048

void sendUdpMessage(const char *msg, const char *ip)
{
    //Serial.printf("sendUdpMessage\n");
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        Serial.println("sendUdpMessage: ошибка создания сокета");
        return;
    }
    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    int sent = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (sent < 0) {
        Serial.println("sendUdpMessage: ошибка отправки");
    } else {
        Serial.printf("sendUdpMessage: отправлено %d байт\n", sent);
    }
    close(sock);
}

void sendUdpMessageLen(const char *msg, int len, const char *ip)
{
    //Serial.printf("sendUdpMessageLen: len %d\n", len);
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        Serial.println("sendUdpMessageLen: socket creation failed");
        return;
    }
    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    int sent = sendto(sock, msg, len, 0, (struct sockaddr *)&dest, sizeof(dest));
    if (sent < 0) {
        Serial.println("sendUdpMessageLen: ошибка отправки");
    } else {
        Serial.printf("sendUdpMessageLen: отправленно %d байт\n", sent);
    }
    close(sock);
}

void sendUdpBroadcast(const char *msg, int len)
{
    //Serial.printf("sendUdpBroadcast: len %d\n", len);
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        Serial.println("sendUdpBroadcast: ошибка создания сокета");
        return;
    }

    // Разрешаем широковещательную отправку
    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        Serial.println("sendUdpBroadcast: setsockopt failed");
        close(sock);
        return;
    }

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255"); // общий broadcast-адрес

    int sent = sendto(sock, msg, len, 0, (struct sockaddr *)&dest, sizeof(dest));
    if (sent < 0) {
        Serial.println("sendUdpBroadcast: sendto failed");
    } else {
        Serial.printf("sendUdpBroadcast: отпралено %d байт\n", sent);
    }

    close(sock);
}


unsigned long timingReadUART; // Переменная для хранения точки отсчета

char data[32768];

void uartTask(void *arg)
{

    EEPROM &eeprom = EEPROM::getInstance();

    uart_event_t event;

    while (true)
    {
        if (xQueueReceive(uartQueue, (void *)&event, portMAX_DELAY))
        {

            if (event.type == UART_DATA)
            {

                size_t max_read = (event.size < sizeof(data) - 1) ? event.size : sizeof(data) - 1;
                int available = uart_read_bytes(UART_NUM_0, data, max_read, 0);

                if (available == -1) {
                    Serial.println("UART ошибка чтения: available == -1");
                    continue;
                }

                if (available >= sizeof(data)) {
                    Serial.printf("UART предотвращение переполнения буфера: available %d >= %d\n", available, sizeof(data));
                    available = sizeof(data) - 1; // Truncate to prevent overflow
                }

                data[available] = '\0';
                Serial.printf("UART received %d bytes\n", available);

                if ((available > 1023) || (millis() - timingReadUART > 50))
                {
                    Serial.printf("UART processing: available %d, time diff %lu ms\n", available, millis() - timingReadUART);

                    if (data[0] != 0)
                    {
                        timingReadUART = millis();
                        eeprom.all_TX_to_UDP += available;
                        Serial.printf("TX to UDP: %d байт, общее TX: %d\n", available, eeprom.all_TX_to_UDP);

                        if (db.get(kk::echo))
                        {
                            Serial.println("🌐 >>UDP_tx_task>>Эхо:");
                            Serial.println(data);
                        }

                        if (db.get(kk::broadcast) == false) {
                            sendUdpMessageLen(&data[0], available, db.get(kk::ipClient).c_str());
                        } else {
                            sendUdpBroadcast(&data[0], available);
                        }
                    } else {
                        Serial.println("UART data[0] == 0, skipping");
                    }
                }
            }
        }
    }
}