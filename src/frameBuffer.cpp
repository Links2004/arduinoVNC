/*
 * @file frameBuffer.cpp
 * @date 13.05.2015
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

#include <Arduino.h>
#include <stdlib.h>
#include <stdint.h>
#include "frameBuffer.h"

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 32
#endif

/// debugging
#ifdef ESP32
#define DEBUG_VNC(...) Serial.printf( __VA_ARGS__ )
#else
#define DEBUG_VNC(...) os_printf( __VA_ARGS__ )
#endif

FrameBuffer::FrameBuffer() {
    w = 0;
    h = 0;
    size = 0;
    buffer = 0;
}

FrameBuffer::~FrameBuffer() {
    freeBuffer();
}

bool FrameBuffer::begin(uint32_t _w, uint32_t _h) {
    w = _w;
    h = _h;

    // force Size to be a multible of 32Bit
    uint32_t newSize = ((w * h * 2) + 3) & ~((uint32_t) 0x00000003);

    //DEBUG_VNC("[FrameBuffer::begin] w: %d h: %d size: %d newSize: %d buffer: 0x%08X Heap: %d\n", w, h, size, newSize, buffer, ESP.getFreeHeap());
    //delay(10);

    if(buffer) {
        if((size < newSize)) {
            //DEBUG_VNC("[FrameBuffer::begin] (size < newSize)  realloc... <--------------------------------------\n");
            delay(10);
            uint8_t * newbuffer = (uint8_t *) realloc(buffer, newSize);
            //DEBUG_VNC("[FrameBuffer::begin] newbuffer: 0x%08X\n", newbuffer);
            if(!newbuffer) {
                freeBuffer();
                return false;
            } else {
                size = newSize;
                buffer = newbuffer;
            }
        }
        return true;
    }

    buffer = (uint8_t *) malloc(newSize);
    if(buffer) {
        size = newSize;
        return true;
    } else {
        DEBUG_VNC("[FrameBuffer::begin] no buffer: 0x%08X Heap: %d <--------------------------------------\n", (size_t)buffer, ESP.getFreeHeap());
        buffer = 0;
        size = 0;
        return false;
    }
}

uint8_t * FrameBuffer::getPtr(void) {
    return buffer;
}

void FrameBuffer::freeBuffer(void) {
    if(buffer) {
        //DEBUG_VNC("[FrameBuffer::draw_rect] free: 0x%08X\n", buffer);
        free(buffer);
        buffer = 0;
        size = 0;
        //DEBUG_VNC("[FrameBuffer::draw_rect] free: 0x%08X Done.\n", buffer);
    }
}

void FrameBuffer::draw_rect(uint32_t x, uint32_t y, uint32_t rw, uint32_t rh, uint16_t color) {
    if(!buffer) {
        //DEBUG_VNC("[FrameBuffer::draw_rect] buffer == null! <--------------------------------------\n");
        return;
    }

    if((((y + rh) * (x + rw)) * sizeof(color)) > size) {
        DEBUG_VNC("[FrameBuffer::draw_rect] out of index!  <--------------------------------------\n");
        DEBUG_VNC("[FrameBuffer::draw_rect] w: %d h: %d - x: %d y: %d rw: %d rh: %d color: 0x%04X\n", w, h, x, y, rw, rh, color);
        delay(50);
        return;
    }

      //  DEBUG_VNC("[FrameBuffer::draw_rect] x: %d y: %d rw: %d rh: %d color: 0x%04X\n", x, y, rw, rh, color);
       // delay(10);

    uint8_t * ptr = buffer + (((y * w) + x) * sizeof(color));
    uint32_t offset = ((w - rw) * 2);
    uint32_t xc;

    uint8_t colorLow = (color & 0xFF);
    uint8_t colorHigh = ((color >> 8) & 0xFF);

    //DEBUG_VNC("[FrameBuffer::draw_rect] offset: %d buffer: 0x%08X ptr: 0x%08X color: 0x%02X 0x%02X\n", offset, buffer, ptr, colorLow, colorHigh);
    //delay(10);

    while(rh--) {
        xc = rw;
        while(xc--) {
            *ptr = colorLow;
            ptr++;
            *ptr = colorHigh;
            ptr++;
        }
        ptr += offset;
    }
}

