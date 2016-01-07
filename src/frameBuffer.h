/*
 * @file frameBuffer.h
 * @date 13.05.2015
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the VNC client for Arduino.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License
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

#ifndef ARDUINOVNC_SRC_FB_H_
#define ARDUINOVNC_SRC_FB_H_

class FrameBuffer {
    public:
        FrameBuffer();
        ~FrameBuffer();
        bool begin(uint32_t _w, uint32_t _h);

        uint8_t * getPtr(void);
        void freeBuffer(void);

        void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);

    private:
        uint32_t w;
        uint32_t h;
        uint32_t size;
        uint8_t * buffer;

};


#endif /* ARDUINOVNC_SRC_FB_H_ */
