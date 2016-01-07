/*
 * VNC.h
 *
 *  Created on: 11.05.2015
 *      Author: links
 */

#ifndef VNC_H_
#define VNC_H_

/// TCP layer
#define USE_ARDUINO_TCP
#define VNC_TCP_TIMEOUT 800

/// VNC Encodes
#define VNC_RRE
#define VNC_CORRE
#define VNC_HEXTILE

// not implementet yet
//#define VNC_TIGHT
//#define VNC_ZLIB
//#define VNC_RICH_CURSOR
//#define VNC_SEC_TYPE_TIGHT

/// Buffers
#define VNC_FRAMEBUFFER

/// Testing
//#define FPS_BENCHMARK
//#define FPS_BENCHMARK_FULL

//#define SLOW_LOOP 250

#define DEBUG_VNC(...) os_printf( __VA_ARGS__ )

#define DEBUG_VNC_RAW(...)
#define DEBUG_VNC_HEXTILE(...)
//#define DEBUG_VNC_RICH_CURSOR(...)

#ifndef DEBUG_VNC
#define DEBUG_VNC(...)
#endif

#ifndef DEBUG_VNC_RAW
#define DEBUG_VNC_RAW(...) DEBUG_VNC( __VA_ARGS__ )
#endif

#ifndef DEBUG_VNC_HEXTILE
#define DEBUG_VNC_HEXTILE(...) DEBUG_VNC( __VA_ARGS__ )
#endif

#ifndef DEBUG_VNC_RICH_CURSOR
#define DEBUG_VNC_RICH_CURSOR(...) DEBUG_VNC( __VA_ARGS__ )
#endif

#define freeSec(ptr) free(ptr); ptr = 0



#include "Arduino.h"

#ifdef USE_ARDUINO_TCP
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <UIPEthernet.h>
#ifndef UIPETHERNET_H
#include <Ethernet.h>
#include <SPI.h>
#endif
#endif
#endif


#define MAXPWLEN 8
#define CHALLENGESIZE 16

#define MAX_ENCODINGS 20

#ifdef WORDS_BIGENDIAN
#define Swap16IfLE(s) (s)
#define Swap32IfLE(l) (l)
#else
#define Swap16IfLE(s) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff))
#define Swap32IfLE(l) ((((l) & 0xff000000) >> 24) | \
               (((l) & 0x00ff0000) >> 8)  | \
               (((l) & 0x0000ff00) << 8)  | \
               (((l) & 0x000000ff) << 24))
#endif /* WORDS_BIGENDIAN */

typedef uint8_t     CARD8;
typedef int8_t      INT8;
typedef uint16_t    CARD16;
typedef int16_t     INT16;
typedef uint32_t    CARD32;
typedef int32_t     INT32;

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
   //char *servername;
   //int port;
   char *password;
   //char *passwordfile;
   //char *encodings;
   //char *modmapfile;
   serversettings_t server;
   clientsettings_t client;
   int shared;
   int stretch;
   int localcursor;
   //int poll_freq;
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

#ifdef VNC_FRAMEBUFFER
#include "frameBuffer.h"
#endif

class VNCdisplay {
    protected:
        VNCdisplay() {}

    public:
        virtual ~VNCdisplay() {}
        virtual bool hasCopyRect(void) = 0;

        virtual uint32_t getHeight(void) = 0;
        virtual uint32_t getWidth(void) = 0;

        virtual void draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) = 0;

        virtual void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color) = 0;
        virtual void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) = 0;

        virtual void area_update_start(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;
        virtual void area_update_data(char *data, uint32_t pixel) = 0;
        virtual void area_update_end(void) = 0;

};

class arduinoVNC {
    public:
        arduinoVNC(VNCdisplay * display);
        ~arduinoVNC(void);

        void begin(char *host, uint16_t port = 5900, bool onlyFullUpdate = false);
        void begin(const char *host, uint16_t port = 5900, bool onlyFullUpdate = false);
        void begin(String host, uint16_t port = 5900, bool onlyFullUpdate= false);

        void setPassword(char * pass);
        void setPassword(const char * pass);
        void setPassword(String pass);

        bool connected(void);
        void reconnect(void);

        void loop(void);


        int forceFullUpdate(void);

    private:
        bool onlyFullUpdate;
        int port;
        String host;
        String password;


        VNCdisplay * display;

        dfb_vnc_options opt;

        uint8_t protocolMinorVersion;

        int sock;
        mousestate_t mousestate;

#ifdef VNC_FRAMEBUFFER
        FrameBuffer fb;
#endif
        /// TCP handling
        void disconnect(void);
        bool read_from_rfb_server(int sock, char *out, size_t n);
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
        bool rfb_send_update_request(int incremental);
        bool rfb_handle_server_message();
        bool rfb_update_mouse();
        bool rfb_send_key_event(int key, int down_flag);

        //void rfb_get_rgb_from_data(int *r, int *g, int *b, char *data);

        /// Encode handling
        bool _handle_server_cut_text_message(rfbServerToClientMsg * msg);

        bool _handle_raw_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
        bool _handle_copyrect_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
#ifdef VNC_CORRE
        bool _handle_rre_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
        bool _handle_corre_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
#endif
#ifdef VNC_HEXTILE
        bool _handle_hextile_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
#endif
        bool _handle_cursor_pos_message(rfbFramebufferUpdateRectHeader rectheader);
#ifdef VNC_RICH_CURSOR
        bool _handle_richcursor_message(rfbFramebufferUpdateRectHeader rectheader);
#endif

        bool _handle_server_continuous_updates_message(rfbFramebufferUpdateRectHeader rectheader);

        /// Encryption
        void vncRandomBytes(unsigned char *bytes);
        void vncEncryptBytes(unsigned char *bytes, char *passwd);

#ifdef VNC_RICH_CURSOR
        /// Cursor
        uint8_t * richCursorData;
        uint8_t * richCursorMask;
        void SoftCursorMove(int x, int y);
#endif

#ifdef USE_ARDUINO_TCP
#ifdef ESP8266
        WiFiClient TCPclient;
#else
#ifdef UIPETHERNET_H
        UIPClient TCPclient;
#else
        EthernetClient TCPclient;
#endif
#endif
#endif

};


#endif /* MARKUS_VNC_H_ */