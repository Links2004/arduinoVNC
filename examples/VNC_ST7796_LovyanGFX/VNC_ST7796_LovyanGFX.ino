/*
 * VNC_ST7796_LovyanGFX.ino
 *
 *  Created on: 10.15.2022
 *  Created by Eric Nam(https://github.com/0015)
 *
 * required librarys:
 *  - SPI (arduino core)
 *  - WiFi (arduino core)
 *  - arduinoVNC (https://github.com/Links2004/arduinoVNC)
 *  - LovyanGFX (https://github.com/lovyan03/LovyanGFX)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <VNC.h>
#include "LovyanGFX_VNCDriver.h"

const char* vnc_ip = "192.168.1.12";
const uint16_t vnc_port = 5900;
const char* vnc_pass = "12345678";

const char* ssid = "your-ssid";
const char* password = "your-password";


int touch_x = 0;
int touch_y = 0;
bool hadTouchEvent = false;

LGFX _lgfx;
VNCDriver* lcd = new VNCDriver(&_lgfx);
arduinoVNC vnc = arduinoVNC(lcd);

void setup(void) {

  Serial.begin(115200);
  _lgfx.init();

  Serial.print("Connecting to ");
  Serial.println(ssid);

  lcd->print_screen("Connecting to ", ssid, TFT_YELLOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd->print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  lcd->print_screen("WiFi connected!", WiFi.localIP().toString(), TFT_GREEN);
  Serial.println(F("[SETUP] VNC..."));

  vnc.begin(vnc_ip, vnc_port);
  vnc.setPassword(vnc_pass);  // check for vnc server settings

  xTaskCreatePinnedToCore(vnc_task,
                          "vnc_task",
                          10000,
                          NULL,
                          1,
                          NULL,
                          0);
}

void loop() {}

void vnc_task(void* pvParameters) {
  while (1) {
    if (WiFi.status() != WL_CONNECTED) {
      lcd->print_screen("WiFi Disconnected!", "Check your WiFi", TFT_RED);
      vnc.reconnect();
      vTaskDelay(100);
    } else {
      vnc.loop();
      if (!vnc.connected()) {
        lcd->print_screen("Connecting VNC", getVNCAddr(), TFT_GREEN);
        vTaskDelay(5000);
      } else {
        touchEvent();
      }
    }
    vTaskDelay(1);
  }
}

void touchEvent() {
  uint16_t x, y;
  if (_lgfx.getTouch(&x, &y)) {
    hadTouchEvent = true;
    touch_x = x;
    touch_y = y;
    vnc.mouseEvent(touch_x, touch_y, 0b001);

  } else if (hadTouchEvent) {
    hadTouchEvent = false;
    vnc.mouseEvent(touch_x, touch_y, 0b000);
  }
}

String getVNCAddr() {
  return String(vnc_ip) + String(":") + String(vnc_port);
}