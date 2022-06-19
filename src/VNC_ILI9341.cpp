/*
 * @file VNC_ILI9341.cpp
 * @date 12.05.2015
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the VNC client for Arduino.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, a copy can be downloaded from
 * http://www.gnu.org/licenses/gpl.html, or obtained by writing to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *
 */

#include "VNC_config.h"

#ifdef VNC_ILI9341

#include "VNC.h"

#include <Adafruit_ILI9341.h>
#include "VNC_ILI9341.h"

ILI9341VNC::ILI9341VNC(int8_t _CS, int8_t _DC, int8_t _RST = -1) :
        Adafruit_ILI9341(_CS, _DC, _RST) {
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
    Adafruit_ILI9341::drawRGBBitmap(x, y, (uint16_t*)data, w, h);
}


void ILI9341VNC::draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color) {
    Adafruit_ILI9341::fillRect(x, y, w, h, ((((color) & 0xff) << 8) | (((color) >> 8))));
}

void ILI9341VNC::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) {

}

void ILI9341VNC::area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    area_x = x;
    area_y = y;
    area_w = w;
    area_h = h;
}

void ILI9341VNC::area_update_data(char *data, uint32_t pixel){
    Adafruit_ILI9341::drawRGBBitmap(area_x, area_y, (uint16_t*)data, area_w, area_h);
}

void ILI9341VNC::area_update_end(void){
}

#endif
