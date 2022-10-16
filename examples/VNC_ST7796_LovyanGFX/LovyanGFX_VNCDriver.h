#pragma once
#include "VNC_config.h"
#include "VNC.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 80000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc = 21;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 15;
      cfg.pin_rst = 22;
      cfg.pin_busy = -1;
      cfg.memory_width = 320;
      cfg.memory_height = 480;
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 23;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _touch_instance.config();

      cfg.i2c_port = 1;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda = 18;
      cfg.pin_scl = 19;
      cfg.freq = 400000;
      cfg.x_min = 0;
      cfg.x_max = 320;
      cfg.y_min = 0;
      cfg.y_max = 480;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

class VNCDriver : public VNCdisplay {
public:

  VNCDriver(LGFX* lgfx);
  ~VNCDriver();

  bool hasCopyRect(void);
  uint32_t getHeight(void);
  uint32_t getWidth(void);
  void draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t* data);
  void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);
  void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h);
  void area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
  void area_update_data(char* data, uint32_t pixel);
  void area_update_end(void);

  void vnc_options_override(dfb_vnc_options* opt);
  void print_screen(String title, String msg, int color);
  void print(String text);

private:
  LGFX* _lcd;
};