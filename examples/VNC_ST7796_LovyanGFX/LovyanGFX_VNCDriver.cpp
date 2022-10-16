#include "LovyanGFX_VNCDriver.h"

VNCDriver::VNCDriver(LGFX *lgfx) {
  _lcd = lgfx;
  _lcd->setRotation(1);
  _lcd->setBrightness(255);
  _lcd->fillScreen(TFT_BLACK);
}

VNCDriver::~VNCDriver() {
}

bool VNCDriver::hasCopyRect(void) {
  return false;
}

uint32_t VNCDriver::getHeight(void) {
  return _lcd->height();
}

uint32_t VNCDriver::getWidth(void) {
  return _lcd->width();
}

void VNCDriver::draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) {
  _lcd->pushImage(x, y, w, h, (uint16_t *)data);
}

void VNCDriver::draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color) {
  _lcd->fillRect(x, y, w, h, ((((color)&0xff) << 8) | (((color) >> 8))));
}

void VNCDriver::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) {
}

void VNCDriver::area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
  _lcd->setAddrWindow(x, y, w, h);
}

void VNCDriver::area_update_data(char *data, uint32_t pixel) {
  _lcd->pushPixels((uint16_t *)data, pixel);
}

void VNCDriver::area_update_end(void) {
  _lcd->endWrite();
}

void VNCDriver::vnc_options_override(dfb_vnc_options *opt) {
  opt->client.bigendian = 1;
}

void VNCDriver::print_screen(String title, String msg, int color) {
  _lcd->fillScreen(TFT_BLACK);
  _lcd->setCursor(0, _lcd->height() / 3);
  _lcd->setTextColor(color);
  _lcd->setTextSize(5);
  _lcd->println(title);
  _lcd->setTextSize(3);
  _lcd->println(msg);
}

void VNCDriver::print(String text) {
  _lcd->print(text);
}