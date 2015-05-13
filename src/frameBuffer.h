/*
 * frameBuffer.h
 *
 *  Created on: 13.05.2015
 *      Author: links
 */

#ifndef ARDUINOVNC_SRC_FB_H_
#define ARDUINOVNC_SRC_FB_H_

class FrameBuffer {
    public:
        FrameBuffer();
        ~FrameBuffer();
        bool begin(uint32_t _w, uint32_t _h);

        uint8_t * getPtr(void);

        void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);

    private:
        uint32_t w;
        uint32_t h;
        uint32_t size;
        uint8_t * buffer;

};


#endif /* ARDUINOVNC_SRC_FB_H_ */
