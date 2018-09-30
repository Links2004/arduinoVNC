/*
 * VNC_ILI9341.ino
 *
 *  Created on: 07.01.2015
 *
 * required librarys:
 *  - SPI (arduino core)
 *  - WiFi (arduino core)
 *  - Adafruit_GFX (https://github.com/adafruit/Adafruit-GFX-Library)
 *  - Adafruit_ILI9341 (https://github.com/Links2004/Adafruit_ILI9341)
 *  - arduinoVNC (https://github.com/Links2004/arduinoVNC)
 */

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include <VNC_ILI9341.h>
#include <VNC.h>

// ILI9341
#define TFT_DC      (5)
#define TFT_CS      (15)
#define TFT_RESET   (4)
// SPI:
// SCK to 14 (18 on esp32)
// MISO to 12 (19 on esp32)
// MOSI to 13 (23 on esp32)

const char * vnc_ip = "192.168.1.12";
const uint16_t vnc_port = 5900;
const char * vnc_pass = "12345678";

const char* ssid = "your-ssid";
const char* password = "your-password";

ILI9341VNC tft = ILI9341VNC(TFT_CS, TFT_DC, TFT_RESET);
arduinoVNC vnc = arduinoVNC(&tft);

void TFTnoWifi(void) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, ((tft.getHeight() / 2) - (5 * 8)));
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(5);
    tft.println("NO WIFI!");
    tft.setTextSize(2);
    tft.println();
}


void TFTnoVNC(void) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, ((tft.getHeight() / 2) - (4 * 8)));
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(4);
    tft.println("connect VNC");
    tft.setTextSize(2);
    tft.println();
    tft.print(vnc_ip);
    tft.print(":");
    tft.println(vnc_port);
}

void setup(void) {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();

    // Init ILI9341
    tft.begin();
    delay(10);
    tft.setRotation(1);

    tft.fillScreen(ILI9341_BLUE);

    // disable sleep mode for better data rate
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    TFTnoWifi();
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    TFTnoVNC();

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println(F("[SETUP] VNC..."));

    vnc.begin(vnc_ip, vnc_port);
    vnc.setPassword(vnc_pass); // optional
}

void loop() {
    if(WiFi.status() != WL_CONNECTED) {
        vnc.reconnect();
        TFTnoWifi();
        delay(100);
    } else {
        vnc.loop();
        if(!vnc.connected()) {
            TFTnoVNC();
            // some delay to not flood the server
            delay(5000);
        }
    }
}
