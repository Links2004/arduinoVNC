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

class ILI9341VNC: public VNCdisplay {
    public:
        ILI9341VNC(Adafruit_ILI9341 * _tft);

        inline bool hasCopyRect(void);

        inline uint32_t getHeight(void);
        inline uint32_t getWidth(void);

        inline int draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data);
        inline int draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b);
        inline int copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h);

        inline void area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        inline void area_update_data(uint8_t *data, uint32_t pixel);
        inline void area_update_end(void);

    private:
        Adafruit_ILI9341 * tft;
};

#endif /* MARKUS_VNC_ILI9341_H_ */
