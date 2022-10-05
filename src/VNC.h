/*
 * @file VNC.h
 * @date 11.05.2015
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
 * Thanks to all that worked on the original VNC implementation
 *
 */

#ifndef VNC_H_
#define VNC_H_

#include "VNC_config.h"

/// more save free
#define freeSec(ptr) \
  free(ptr);         \
  ptr = 0

#include "Arduino.h"

#ifdef VNC_ZRLE
#if defined(ESP32)
#include "esp32/rom/miniz.h"
#else
#include "miniz.h"
#endif
#endif // #ifdef VNC_ZRLE

#ifdef USE_ARDUINO_TCP

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(RTL8722DM)
#include <WiFi.h>
#else // default platform

#include <UIPEthernet.h>
#ifndef UIPETHERNET_H
#include <Ethernet.h>
#include <SPI.h>
#endif // #ifndef UIPETHERNET_H

#endif // default platform

#endif // #ifdef USE_ARDUINO_TCP

#define MAXPWLEN 8
#define CHALLENGESIZE 16

#define MAX_ENCODINGS 20

#ifdef WORDS_BIGENDIAN
#define Swap16IfLE(s) (s)
#define Swap32IfLE(l) (l)
#else
#define Swap16IfLE(s) __builtin_bswap16(s)
#define Swap32IfLE(l) __builtin_bswap32(l)
#endif /* WORDS_BIGENDIAN */

typedef uint8_t CARD8;
typedef int8_t INT8;
typedef uint16_t CARD16;
typedef int16_t INT16;
typedef uint32_t CARD32;
typedef int32_t INT32;

typedef struct serversettings
{
  char *name;
  int width;
  int height;
  int bpp;
  int depth;
  int bigendian;
  int truecolour;
  int redmax;
  int greenmax;
  int bluemax;
  int redshift;
  int greenshift;
  int blueshift;
} serversettings_t;

typedef struct clientsettings
{
  int width;
  int height;
  int bpp;
  int depth;
  int bigendian;
  int truecolour;
  int redmax;
  int greenmax;
  int bluemax;
  int redshift;
  int greenshift;
  int blueshift;
  int compresslevel;
  int quality;
} clientsettings_t;

//__dfb_vnc_options
typedef struct
{
  // char *servername;
  // int port;
  char *password;
  // char *passwordfile;
  // char *encodings;
  // char *modmapfile;
  serversettings_t server;
  clientsettings_t client;
  int shared;
  int stretch;
  int localcursor;
  // int poll_freq;
  /* not really options, but hey ;) */
  double h_ratio;
  double v_ratio;
  int h_offset;
  int v_offset;
} dfb_vnc_options;

typedef struct
{
  int x;
  int y;
  unsigned int buttonmask;
} mousestate_t;

#include "rfbproto.h"

class VNCdisplay
{
protected:
  VNCdisplay() {}

public:
  virtual ~VNCdisplay() {}

  virtual uint32_t getHeight(void) = 0;
  virtual uint32_t getWidth(void) = 0;

  virtual bool hasCopyRect(void) = 0;
  virtual void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h){};

  virtual void draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) = 0;

  virtual void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color) = 0;

  virtual void flush() {};

  virtual void vnc_options_override(dfb_vnc_options *opt){};
};

class arduinoVNC
{
public:
  arduinoVNC(VNCdisplay *display);
  ~arduinoVNC(void);

  void begin(char *host, uint16_t port = 5900, bool onlyFullUpdate = false);
  void begin(const char *host, uint16_t port = 5900, bool onlyFullUpdate = false);
  void begin(String host, uint16_t port = 5900, bool onlyFullUpdate = false);

  void setPassword(char *pass);
  void setPassword(const char *pass);
  void setPassword(String pass);

  bool connected(void);
  void reconnect(void);

  void loop(void);

  int forceFullUpdate(void);

  void mouseEvent(uint16_t x, uint16_t y, uint8_t buttonMask);
  void keyEvent(int key, int down_flag);

private:
  bool onlyFullUpdate;
  int port;
  String host;
  String password;

  VNCdisplay *display;

  dfb_vnc_options opt;

  uint8_t protocolMinorVersion;

  int sock;
  mousestate_t mousestate;

  /// TCP handling
  void disconnect(void);
  bool read_from_rfb_server(int sock, char *out, size_t n);

#ifdef VNC_ZRLE
  bool read_from_z(uint8_t *out, size_t n);
#endif // #ifdef VNC_ZRLE

  bool write_exact(int sock, char *buf, size_t n);
  bool set_non_blocking(int sock);

  /// Connect to Server
  bool rfb_connect_to_server(const char *server, int display);
  bool rfb_initialise_connection();

  bool _read_conn_failed_reason(void);
  bool _read_authentication_result(void);

  bool _rfb_negotiate_protocol(void);
  bool _rfb_authenticate(void);
  bool _rfb_initialise_client(void);
  bool _rfb_initialise_server(void);

  bool rfb_set_format_and_encodings();
  bool rfb_set_desktop_size();
  bool rfb_send_update_request(int incremental);
  bool rfb_set_continuous_updates(bool enable);
  bool rfb_handle_server_message();
  bool rfb_update_mouse();
  bool rfb_send_key_event(int key, int down_flag);

  // void rfb_get_rgb_from_data(int *r, int *g, int *b, char *data);

  /// Encode handling
  bool _handle_server_cut_text_message(rfbServerToClientMsg *msg);

  bool _handle_raw_encoded_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  bool _handle_raw_encoded_message_core(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  bool _handle_copyrect_encoded_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
#ifdef VNC_RRE
  bool _handle_rre_encoded_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
#endif
#ifdef VNC_CORRE
  bool _handle_corre_encoded_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
#endif
#ifdef VNC_HEXTILE
  bool _handle_hextile_encoded_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
#endif
#ifdef VNC_ZRLE
  bool _handle_zrle_encoded_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
#endif
  bool _handle_cursor_pos_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

  bool _handle_server_continuous_updates_message(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

  /// Encryption
  void vncRandomBytes(unsigned char *bytes);
  void vncEncryptBytes(unsigned char *bytes, char *passwd);

#ifdef USE_ARDUINO_TCP

#if defined(ESP32)
  WiFiClient TCPclient;
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  WiFiClient TCPclient;
#elif defined(ESP8266)
  WiFiClient TCPclient;
#elif defined(RTL8722DM)
  WiFiClient TCPclient;
#else // default platform

#ifdef UIPETHERNET_H
  UIPClient TCPclient;
#else
  EthernetClient TCPclient;
#endif

#endif // default platform

#endif // #ifdef USE_ARDUINO_TCP

#ifdef TCP_BUFFER_SIZE
  uint8_t tcp_buffer[TCP_BUFFER_SIZE];
  size_t buf_idx = 0;
  size_t buf_remain = 0;
#endif

  uint32_t maxSize = VNC_RAW_BUFFER;
  char raw_buffer[VNC_RAW_BUFFER];
  uint16_t framebuffer[FB_SIZE];

#ifdef VNC_ZRLE
  tinfl_decompressor inflator;
  bool header_inited = false;

  uint8_t *zin_buffer;
  size_t allocated_zin = 0, zin_buf_size = 0, zin_buf_ofs = 0;

  uint8_t *zdict;
  size_t zdict_ofs = 0;

  uint8_t *zout_buffer;
  size_t zout_buf_idx = 0, zout_buf_remain = 0;

  uint16_t palette[127];
#endif
};

#endif /* MARKUS_VNC_H_ */
