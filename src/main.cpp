#define WIFI_LoRa_32
#include <heltec.h>
#include "board_def.h"
#include <SPI.h>
#include <lora/LoRa.h>
#include "./ds3231.h"
#include <WiFi.h>
#include <SD.h>

// Create a file the defines:
//#define WIFI_SSID       "SSID"
//#define WIFI_PASSWORD   "PASSWD"

#include "wifipasswd.h"

OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL, OLED_RST);


void setup()
{
    Serial.begin(9600);
    while (!Serial);

    if (OLED_RST > 0) {
        pinMode(OLED_RST, OUTPUT);
        digitalWrite(OLED_RST, HIGH);
        delay(100);
        digitalWrite(OLED_RST, LOW);
        delay(100);
        digitalWrite(OLED_RST, HIGH);
    }

    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, LORA_SENDER ? "LoRa Sender" : "LoRa Receiver");
    display.display();
    delay(2000);

    if (SDCARD_CS >  0) {
        display.clear();
        SPIClass sdSPI(VSPI);
        sdSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
        if (!SD.begin(SDCARD_CS, sdSPI)) {
            display.drawString(display.getWidth() / 2, display.getHeight() / 2, "SDCard  FAIL");
        } else {
            display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 16, "SDCard  PASS");
            uint32_t cardSize = SD.cardSize() / (1024 * 1024);
            display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Size: " + String(cardSize) + "MB");
        }
        display.display();
        delay(2000);
    }


    String info = ds3231_test();
    if (info != "") {
        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, info);
        display.display();
        delay(2000);
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        Serial.print("Connected : ");
        Serial.println(WiFi.SSID());
        Serial.print("IP:");
        Serial.println(WiFi.localIP().toString());
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, "IP:" + WiFi.localIP().toString());
        display.display();
    }
    else
    {
        display.clear();
        Serial.println("WiFi Connect Fail");
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, "WiFi Connect Fail");
        display.display();
    }
    
    delay(2500);

    SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
    LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
    LoRa.setSPIFrequency(500000);

    if (!LoRa.begin(BAND, false)) {
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoRa failed!");
        display.display();
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    if (!LORA_SENDER) {
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoraRecv Ready");
        display.display();
    }
}

int count = 0;

void loop()
{
#if LORA_SENDER
    int32_t rssi;
    if (WiFi.status() == WL_CONNECTED) {
        rssi = WiFi.RSSI();
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Send RSSI:" + String(rssi));
        display.display();
        Serial.printf("Wifi RSSI: %d\n", rssi);
        LoRa.beginPacket();
        LoRa.print("WiFi RSSI: ");
        LoRa.print(rssi);
        LoRa.endPacket();
    } else {
        Serial.println("WiFi Connect lost ...");
    }
    delay(2500);
#else
    if (LoRa.parsePacket()) {
        String recv = "";
        while (LoRa.available()) {
            recv += (char)LoRa.read();
        }
        count++;
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, recv);
        String info = "[" + String(count) + "]" + "RSSI " + String(LoRa.packetRssi());
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 16, info);
        display.display();
    }
#endif
}
