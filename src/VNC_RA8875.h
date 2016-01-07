/*
 * @file VNC_RA8875.h
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

#ifndef VNC_RA8875_H_
#define VNC_RA8875_H_

#include <Adafruit_RA8875.h>
#include "VNC_RA8875.h"
#include "VNC.h"

class RA8875VNC: public VNCdisplay, public Adafruit_RA8875 {
    public:
        RA8875VNC(int8_t _CS, int8_t _RST);

        bool hasCopyRect(void);

        uint32_t getHeight(void);
        uint32_t getWidth(void);

        void draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data);


        void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);

        void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h);

        void area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        void area_update_data(char *data, uint32_t pixel);
        void area_update_end(void);

};

#endif /* MARKUS_VNC_RA8875_H_ */
