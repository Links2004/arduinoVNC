/*
 * VNC_ILI9341.cpp
 *
 *  Created on: 12.05.2015
 *      Author: links
 */

#include "VNC.h"

#include <Adafruit_ILI9341.h>
#include "VNC_ILI9341.h"

ILI9341VNC::ILI9341VNC(Adafruit_ILI9341 * _tft) {
    tft = _tft;
}

bool ILI9341VNC::hasCopyRect(void) {
    return false;
}

uint32_t ILI9341VNC::getWidth(void) {
    return tft->getWidth();
}

uint32_t ILI9341VNC::getHeight(void) {
    return tft->getHeight();
}

int ILI9341VNC::draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) {
    uint32_t pixel = w * h;
    uint16_t * color = (uint16_t *) data;
    tft->setAddrWindow(x, y, x + w, y + h);

    while(pixel--) {
        tft->pushColor(*color);
        color++;
    }
    return 1;
}

int ILI9341VNC::draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b) {
    tft->fillRect(x, y, w, h, tft->color565(r, g, b));
    return 1;
}

int ILI9341VNC::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) {
    return 0;
}



