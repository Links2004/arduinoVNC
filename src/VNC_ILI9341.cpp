/*
 * VNC_ILI9341.cpp
 *
 *  Created on: 12.05.2015
 *      Author: links
 */

#include "VNC.h"

#include <Adafruit_ILI9341.h>
#include "VNC_ILI9341.h"

ILI9341VNC::ILI9341VNC(int8_t _CS, int8_t _DC, int8_t _RST = -1) :
        Adafruit_ILI9341(_CS, _DC, _RST = -1) {
}

bool ILI9341VNC::hasCopyRect(void) {
    return false;
}

uint32_t ILI9341VNC::getHeight(void) {
    return Adafruit_ILI9341::_height;
}

uint32_t ILI9341VNC::getWidth(void) {
    return Adafruit_ILI9341::_width;
}

void ILI9341VNC::draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) {
    Adafruit_ILI9341::area_update_start(x, y, w, h);
    Adafruit_ILI9341::area_update_data(data, (w*h));
    Adafruit_ILI9341::area_update_end();
}


void ILI9341VNC::draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color) {
    Adafruit_ILI9341::fillRect(x, y, w, h, ((((color) & 0xff) << 8) | (((color) >> 8))));
}

void ILI9341VNC::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) {

}

void ILI9341VNC::area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    Adafruit_ILI9341::area_update_start(x, y, w, h);
}

void ILI9341VNC::area_update_data(char *data, uint32_t pixel){
    Adafruit_ILI9341::area_update_data((uint8_t *)data, pixel);
}

void ILI9341VNC::area_update_end(void){
    Adafruit_ILI9341::area_update_end();
}
