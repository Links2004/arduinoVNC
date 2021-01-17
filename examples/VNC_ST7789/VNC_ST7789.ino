/*
 * VNC_ST7789.ino
 *
 *  Created on: 17.01.2021
 *
 * required librarys:
 *  - SPI (arduino core)
 *  - WiFi (arduino core)
 *  - arduinoVNC (https://github.com/Links2004/arduinoVNC)
 *  - TFT_eSPI (https://github.com/Bodmer/TFT_eSPI)
 */

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <SPI.h>

#include <TFT_eSPI.h>
#include <VNC_ST7789.h>
#include <VNC.h>

const char * vnc_ip = "192.168.1.12";
const uint16_t vnc_port = 5900;
const char * vnc_pass = "12345678";

const char* ssid = "your-ssid";
const char* password = "your-password";

ST7789VNC tft = ST7789VNC();
arduinoVNC vnc = arduinoVNC(&tft);

void setup(void) {
    tft.begin();
    Serial.begin(115200);

    tft.setRotation(0);
    tft.setTextSize(2);
    tft.setCursor(0, 0, 1);
    tft.println("Ready");

    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println();
    Serial.println();

    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println(F("[SETUP] VNC..."));

    vnc.begin(vnc_ip, vnc_port);
    //vnc.setPassword(vnc_pass); // check for vnc server settings
}

void loop() {
    if(WiFi.status() != WL_CONNECTED) {
        vnc.reconnect();
        delay(100);
    } else {
        vnc.loop();
        if(!vnc.connected()) {
            delay(5000);
        }
    }
}