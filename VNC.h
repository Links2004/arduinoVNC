/*
 * VNC.h
 *
 *  Created on: 11.05.2015
 *      Author: links
 */

#ifndef VNC_H_
#define VNC_H_

#define USE_ARDUINO_TCP

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


#define DEBUG_VNC(...) os_printf( __VA_ARGS__ )
//#define DEBUG_VNC(...)

#define MAXPWLEN 8
#define CHALLENGESIZE 16

#define MAX_ENCODINGS 10

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

class VNCdisplay {
    protected:
        VNCdisplay() {}

    public:
        virtual ~VNCdisplay() {}
        virtual bool hasCopyRect(void) = 0;

        virtual uint32_t getHeight(void)= 0;
        virtual uint32_t getWidth(void)= 0;

        virtual int draw_area(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t *data) = 0;
        virtual int draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b) = 0;
        virtual int copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dest_x, uint32_t dest_y, uint32_t w, uint32_t h) = 0;

};

class arduinoVNC {
    public:
        arduinoVNC(VNCdisplay * display);

        void begin(char *host, uint16_t port = 5900);
        void begin(const char *host, uint16_t port = 5900);
        void begin(String host, uint16_t port = 5900);

        void loop(void);

    private:
        int port;
        String host;
        VNCdisplay * display;

        dfb_vnc_options opt;

        int sock;
        mousestate_t mousestate;

        /// TCP handling
        int read_from_rfb_server(int sock, char *out, size_t n);
        int write_exact(int sock, char *buf, size_t n);
        int set_non_blocking(int sock);


        /// Connect to Server
        int rfb_connect_to_server(const char *server, int display);
        int rfb_initialise_connection();

        int _rfb_negotiate_protocol(void);
        int _rfb_authenticate(void);
        int _rfb_initialise_client(void);
        int _rfb_initialise_server(void);


        int rfb_set_format_and_encodings();
        int rfb_send_update_request(int incremental);
        int rfb_handle_server_message();
        int rfb_update_mouse();
        int rfb_send_key_event(int key, int down_flag);

        void rfb_get_rgb_from_data(int *r, int *g, int *b, char *data);



        /// Encode handling
        int _handle_raw_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
        int _handle_copyrect_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
#ifdef VNC_CORRE
        int _handle_rre_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
        int _handle_corre_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
#endif
#ifdef VNC_HEXTILE
        int _handle_hextile_encoded_message(rfbFramebufferUpdateRectHeader rectheader);
#endif
        int _handle_richcursor_message(rfbFramebufferUpdateRectHeader rectheader);

        /// Encryption
        void vncRandomBytes(unsigned char *bytes);
        void vncEncryptBytes(unsigned char *bytes, char *passwd);

        /// Cursor
        int HandleRichCursor(int x, int y, int w, int h);
        void SoftCursorMove(int x, int y);


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

int vnc_connect(char *host, int port);
void vnc_loop(void);

#endif /* MARKUS_VNC_H_ */
