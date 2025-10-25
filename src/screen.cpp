#include "define.h"

extern Adafruit_SSD1306 display;

int screen = 0; // 0-подключение

void screenLoop()
{
    EEPROM &eeprom = EEPROM::getInstance();

    // Получение уровня сигнала WiFi (RSSI)
    long rssi = WiFi.RSSI();
    IPAddress ip = WiFi.localIP();

    display.clearDisplay();

    //display.drawRect(0, 0, 128, 64, WHITE);

    display.setCursor(0, 2);
    display.print("IP: ");
    display.println(ip);

    display.setCursor(0, 11);
    display.print("Client: ");
    display.print(db.get(kk::ipClient));

    display.setCursor(0, 20);
    display.print("Birtate: ");
    display.print(db.get(kk::Serial2Bitrate));

    display.setCursor(0, 29);
    display.print("Broadcast: ");
    if (db.get(kk::broadcast)) display.print("True"); else  display.print("False");
  

    display.setCursor(0, 38);
    display.print("Echo: ");
    if (db.get(kk::echo)) display.print("True"); else  display.print("False");

    display.setCursor(0, 48);
    display.print("TX: ");
    display.print(eeprom.all_TX_to_UDP);


    display.setCursor(110, 55);
    display.println(rssi);

    display.display();

    delay(1000);
}