#include "define.h"

#include "lwip/sockets.h"

#define BUF_SIZE 2048

void sendUdpMessage(const char *msg, const char *ip)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
    close(sock);
}

void sendUdpMessageLen(const char *msg, int len, const char *ip)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    sendto(sock, msg, len, 0, (struct sockaddr *)&dest, sizeof(dest));
    close(sock);
}

void sendUdpBroadcast(const char *msg, int len)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        printf("Socket creation failed\n");
        return;
    }

    // Разрешаем широковещательную отправку
    int broadcastEnable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255"); // общий broadcast-адрес

    sendto(sock, msg, len, 0, (struct sockaddr *)&dest, sizeof(dest));

    close(sock);
}


unsigned long timingReadUART; // Переменная для хранения точки отсчета
unsigned long timingTemp;

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

                int avalible = uart_read_bytes(UART_NUM_0, data, event.size, 0);

                if (avalible == -1)
                    continue;

                data[avalible] = '\0';

                if ((avalible > 1023) || (millis() - timingReadUART > 50))
                {

                    if (data[0] != 0)
                    {
                        timingReadUART = millis();
                        eeprom.all_TX_to_UDP += avalible;

                        if (eeprom.echo)
                        {
                            Serial.println("🌐 >>UDP_tx_task>>Эхо:");
                            Serial.println(data);
                        }

                        if (broadcast == false)
                            sendUdpMessageLen(&data[0], avalible, eeprom.ipClient.c_str());
                        else
                            sendUdpBroadcast(&data[0], avalible);
                    }
                }
            }
        }
    }
}