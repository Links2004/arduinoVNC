/*
 * VNC_ILI9341.h
 *
 *  Created on: 12.05.2015
 *      Author: Markus Sattler
 */

#ifndef VNC_ILI9341_H_
#define VNC_ILI9341_H_

#include <Adafruit_ILI9341.h>
#include "VNC_ILI9341.h"
#include "VNC.h"

class ILI9341VNC: public VNCdisplay, public Adafruit_ILI9341 {
    public:
        ILI9341VNC(int8_t _CS, int8_t _DC, int8_t _RST);

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

#endif /* MARKUS_VNC_ILI9341_H_ */
