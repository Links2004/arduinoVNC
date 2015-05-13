/*
 * FrameBuffer.cpp
 *
 *  Created on: 13.05.2015
 *      Author: links
 */

#include <Arduino.h>
#include <stdlib.h>
#include <stdint.h>
#include "frameBuffer.h"

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 32
#endif

FrameBuffer::FrameBuffer() {
    w = 0;
    h = 0;
    buffer = 0;
    size = 0;
}

FrameBuffer::~FrameBuffer() {
    if(buffer) {
        free(buffer);
    }
}

bool FrameBuffer::begin(uint32_t _w, uint32_t _h) {
    w = _w;
    h = _h;

    uint32_t newSize = (w * h * 2);

    if(buffer && size != newSize) {
        free(buffer);
    }

    if(!buffer) {
        buffer = (uint8_t *) malloc(newSize);
    }

    if(buffer) {
        size = newSize;
        return true;
    }

    size = 0;
    return false;
}

uint8_t * FrameBuffer::getPtr(void) {
    return buffer;
}

void FrameBuffer::draw_rect(uint32_t x, uint32_t y, uint32_t rw, uint32_t rh, uint16_t color) {
    if(!buffer) {
        os_printf("FrameBuffer::draw_rect == null!");
    }
    uint8_t * ptr = buffer + (((y * h) + x) * sizeof(color));
    uint32_t offset = ((w - rw) * 2);
    uint32_t xc;
    while(rh--) {
        xc = rw;
        while(xc--) {
            *ptr = (color & 0xFF);
            ptr++;
            *ptr = ((color >> 8) & 0xFF);
            ptr++;
        }
        ptr += offset;
    }
}

