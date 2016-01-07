/*
 * @file VNC_RA8875.cpp
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

#ifdef VNC_RA8875

#include "VNC.h"

#include <Adafruit_RA8875.h>
#include "VNC_RA8875.h"

RA8875VNC::RA8875VNC(int8_t _CS, int8_t _RST = -1) :
        Adafruit_RA8875(_CS, _RST = -1) {
}

bool RA8875VNC::hasCopyRect(void) {
    return false;
}

uint32_t RA8875VNC::getHeight(void) {
    return Adafruit_RA8875::_height;
}

uint32_t RA8875VNC::getWidth(void) {
    return Adafruit_RA8875::_width;
}

void RA8875VNC::draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) {
   // Adafruit_RA8875::area_update_start(x, y, w, h);
    //Adafruit_RA8875::area_update_data(data, (w*h));
   // Adafruit_RA8875::area_update_end();
}


void RA8875VNC::draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color) {
    Adafruit_RA8875::fillRect(x, y, w, h, ((((color) & 0xff) << 8) | (((color) >> 8))));
}

void RA8875VNC::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) {

}

void RA8875VNC::area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    Adafruit_RA8875::area_update_start(x, y, w, h);
}

void RA8875VNC::area_update_data(char *data, uint32_t pixel){
    Adafruit_RA8875::area_update_data((uint8_t *)data, pixel);
}

void RA8875VNC::area_update_end(void){
    Adafruit_RA8875::area_update_end();
}

#endif
