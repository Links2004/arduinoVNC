/*
 * VNC_ILI9341_touch.ino
 *
 *  Created on: 20.02.2016
 *
 * required librarys:
 *  - SPI (arduino core)
 *  - WiFi (arduino core)
 *  - Adafruit_GFX (https://github.com/adafruit/Adafruit-GFX-Library)
 *  - Adafruit_ILI9341 (https://github.com/Links2004/Adafruit_ILI9341)
 *  - arduinoVNC (https://github.com/Links2004/arduinoVNC)
 *  - XPT2046 (https://github.com/Links2004/XPT2046)
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

#include <XPT2046.h>

// ILI9341
// SPI:
// SCK to 14 (18 on esp32)
// MOSI to 13 (23 on esp32)
// (MISO not connected (or 19 on esp32)
// Reset to Reset of the ESP
#define TFT_DC      (5)
#define TFT_CS      (15)

// XPT2046
// SPI:
// SCK to 14
// MOSI to 13
// MISO to 12
#define TOUCH_IRQ 4
#define TOUCH_CS  16

const char * vnc_ip = "192.168.1.12";
const uint16_t vnc_port = 5900;
const char * vnc_pass = "12345678";

const char* ssid = "your-ssid";
const char* password = "your-password";

ILI9341VNC tft = ILI9341VNC(TFT_CS, TFT_DC, NOT_A_PIN);
arduinoVNC vnc = arduinoVNC(&tft);
XPT2046 touch = XPT2046(TOUCH_CS, TOUCH_IRQ);

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

    touch.begin(tft.getWidth(), tft.getHeight());
    touch.setRotation(1);
    touch.setCalibration(350, 550, 3550, 3600); // may need to be changed

    touch.onChange(6, 200, [](bool press, uint16_t x, uint16_t y, uint16_t z) {
        static unsigned long lastUpdateP;
        static unsigned long lastUpdateR;
        static uint16_t lx, ly;
        if(z > 600) {
            if((millis() - lastUpdateP) > 20) {
                vnc.mouseEvent(x, y, 0b001);
                lx = x;
                ly = y;
                lastUpdateP = millis();
                Serial.printf("[Touch] press: 1 X: %d Y: %d Z: %d\n", x, y, z);
            }
            lastUpdateR = 0;
        } else {
            if((millis() - lastUpdateR) > 20) {
                vnc.mouseEvent(lx, ly, 0b000);
                lastUpdateR = millis();
                Serial.printf("[Touch] press: 0 X: %d Y: %d Z: %d\n", lx, ly, z);
            }
            lastUpdateP = 0;
        }
    });


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
        if(vnc.connected()) {
            touch.loop();
        }
        vnc.loop();
        if(!vnc.connected()) {
            TFTnoVNC();
            // some delay to not flood the server
            delay(5000);
        }
    }
}
