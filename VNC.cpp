/*
 * VNC.cpp
 *
 *  Created on: 11.05.2015
 *      Author: Markus Sattler
 */

#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <math.h>

#include "VNC.h"

#ifdef VNC_TIGHT
#include "tight.h"
#endif

#ifdef VNC_ZLIB
#include <zlib.h>
#endif

extern "C" {
#include "d3des.h"
}

//#############################################################################################

arduinoVNC::arduinoVNC(VNCdisplay * _display) {
    host = "";
    port = 5900;
    display = _display;
    opt = {0};
}

void arduinoVNC::begin(char *_host, uint16_t _port) {
    host = _host;
    port = _port;

    opt.client.width = display->getWidth();
    opt.client.height = display->getHeight();

    opt.client.bpp = 16;
    opt.client.depth = 16;
    opt.client.bigendian = 0;
    opt.client.truecolour = 1;
    opt.client.redmax = 31;
    opt.client.greenmax = 63;
    opt.client.bluemax = 31;
    opt.client.redshift = 11;
    opt.client.greenshift = 5;
    opt.client.blueshift = 0;
    opt.client.compresslevel = 99;
    opt.client.quality = 99;

    opt.shared = 1;
    opt.localcursor = 1;

    opt.h_ratio = 1;
    opt.v_ratio = 1;
    opt.h_offset = 0;
    opt.v_offset = 0;
}

void arduinoVNC::begin(const char *_host, uint16_t _port) {
    begin((char *) _host, _port);
}

void arduinoVNC::begin(String _host, uint16_t _port) {
    begin(_host.c_str(), _port);
}

void arduinoVNC::loop(void) {

    if(!TCPclient.connected()) {

        if(!rfb_connect_to_server(host.c_str(), port)) {
            DEBUG_VNC("Couldnt establish connection with the VNC server. Exiting\n");
            return;
        }

        /* initialize the connection */
        if(!rfb_initialise_connection()) {
            DEBUG_VNC("Connection with VNC server couldnt be initialized. Exiting\n");
            return;
        }

        /* Tell the VNC server which pixel format and encodings we want to use */
        if(!rfb_set_format_and_encodings()) {
            DEBUG_VNC("Error negotiating format and encodings. Exiting.\n");
            return;
        }

        /* calculate horizontal and vertical offset */
        if(opt.client.width > opt.server.width)
            opt.h_offset = rint((opt.client.width - opt.server.width) / 2);
        if(opt.client.height > opt.server.height)
            opt.v_offset = rint((opt.client.height - opt.server.height) / 2);

        /*
         mousestate.x = opt.client.width / 2;
         mousestate.y = opt.client.height / 2;
         */
        /*  FIXME disabled for now
         *    opt.h_ratio = (double) opt.client.width / (double) opt.server.width;
         *    opt.v_ratio = (double) opt.client.height / (double) opt.server.height;
         */

        /* Now enter the main loop, processing VNC messages.  mouse and keyboard
         * events will automatically be processed whenever the VNC connection is
         * idle. */
        rfb_send_update_request(0);

        DEBUG_VNC("vnc_connect Done.\n");

    } else {
        if(!rfb_handle_server_message()) {
            DEBUG_VNC("rfb_handle_server_message faild.\n");
            return;
        }
        rfb_send_update_request(1);
    }
}

//#############################################################################################
//                                       TCP handling
//#############################################################################################

#ifdef USE_ARDUINO_TCP

int arduinoVNC::read_from_rfb_server(int sock, char *out, size_t n) {
    unsigned long t = millis();
    size_t len;
    //DEBUG_VNC("read_from_rfb_server %d...\n", n);

    if(n > 2 * 1024 * 1042) {
        DEBUG_VNC("read_from_rfb_server N: %d 0x%08X make no sens!\n", n, n);
        return 0;
    }

    if(out == NULL) {
        DEBUG_VNC("read_from_rfb_server out == NULL!\n");
        return 0;
    }

    while(n > 0) {
        if(!TCPclient.connected()) {
            DEBUG_VNC("Receive not connected!\n");
            return 0;
        }
        while(!TCPclient.available()) {
            if((millis() - t) > 5000) {
                DEBUG_VNC("Receive TIMEOUT!\n");
                return 0;
            }
            delay(0);
        }

        len = TCPclient.read((uint8_t*) out, n);
        if(len) {
            t = millis();
            if(len > n) {
                DEBUG_VNC("Receive %d left need only %d?!\n", len, n);
            } else {
                n -= len;
                //   DEBUG_VNC("Receive %d left %d!\n", len, n);
            }
        } else {
            // DEBUG_VNC("Receive %d left %d!\n", len, n);
        }
        delay(0);
    }
    return 1;
}

int arduinoVNC::write_exact(int sock, char *buf, size_t n) {
    if(!TCPclient.connected()) {
        DEBUG_VNC("Receive not connected!\n");
        return 0;
    }
    return TCPclient.write((uint8_t*) buf, n);
}

int arduinoVNC::set_non_blocking(int sock) {
    return 1;
}
#else
#error implement TCP handling
#endif

//#############################################################################################
//                                       Connect to Server
//#############################################################################################
/*
 * ConnectToRFBServer.
 */
int arduinoVNC::rfb_connect_to_server(const char *host, int port) {
#ifdef USE_ARDUINO_TCP
    if(!TCPclient.connect(host, port)) {
        DEBUG_VNC("[rfb_connect_to_server] Connect error\n");
        return 0;
    }
    DEBUG_VNC("[rfb_connect_to_server] Connected.\n");
    return 1;
#else
    struct hostent *he=NULL;
    int one=1;
    struct sockaddr_in s;

    if ( (sock = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        DEBUG_VNC("Error creating communication socket: %d\n", errno);
        //exit(2)
        return (0);
    }

    /* if the server wasnt specified as an ip address, look it up */
    if (!inet_aton(host, &s.sin_addr))
    {
        if ( (he = gethostbyname(host)) )
        memcpy (&s.sin_addr.s_addr, he->h_addr, he->h_length);
        else
        {
            DEBUG_VNC("Couldnt resolve host!\n");
            close(sock);
            return (0);
        }
    }

    s.sin_port = htons(port);
    s.sin_family = AF_INET;

    if (connect(sock,(struct sockaddr*) &s, sizeof(s)) < 0)
    {
        DEBUG_VNC("Connect error\n");
        close(sock);
        return (0);
    }
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)) < 0)
    {
        DEBUG_VNC("Error setting socket options\n");
        close(sock);
        return (0);
    }

    if (!set_non_blocking(sock)) return -1;

    return (sock);
#endif
}

int arduinoVNC::rfb_initialise_connection() {
    if(!_rfb_negotiate_protocol()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_negotiate_protocol()  Faild!\n");
        return 0;
    }

    if(!_rfb_authenticate()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_authenticate()  Faild!\n");
        return 0;
    }

    if(!_rfb_initialise_client()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_initialise_client()  Faild!\n");
        return 0;
    }

    if(!_rfb_initialise_server()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_initialise_server() Faild!\n");
        return 0;
    }
    return (1);
}

int arduinoVNC::_rfb_negotiate_protocol() {
    rfbProtocolVersionMsg msg;

    /* read the protocol version the server uses */
    if(!read_from_rfb_server(sock, (char*) &msg, sz_rfbProtocolVersionMsg))
        return 0;
    /* FIXME actually do something with that information ;) */

    /* send the protocol version we want to use */
    sprintf(msg, rfbProtocolVersionFormat,
    rfbProtocolMajorVersion,
    rfbProtocolMinorVersion);
    if(!write_exact(sock, msg, sz_rfbProtocolVersionMsg))
        return 0;

    return 1;
}

int arduinoVNC::_rfb_authenticate() {
    CARD32 authscheme;

    read_from_rfb_server(sock, (char *) &authscheme, 4);
    authscheme = Swap32IfLE(authscheme);
    switch(authscheme) {
        CARD32 reason_length;
        CARD8 *reason_string;
        CARD8 challenge_and_response[CHALLENGESIZE];
        CARD32 auth_result;

    case rfbConnFailed:
        DEBUG_VNC("DIRECTVNC: Connection to VNC server failed\n");
        read_from_rfb_server(sock, (char *) &reason_length, 4);
        reason_length = Swap32IfLE(reason_length);
        reason_string = (CARD8 *) malloc(sizeof(CARD8) * reason_length);
        read_from_rfb_server(sock, (char *) reason_string, reason_length);
        DEBUG_VNC("Errormessage: %s\n", reason_string);
        return (0);
    case rfbVncAuth:

#if 0
        /* we didnt get a password on the command line, so go get one */
        if (!opt.password)
        {
            if( opt.passwordfile )
            {
                opt.password = vncDecryptPasswdFromFile( opt.passwordfile );
            }
            else
            {
                opt.password = getpass("Password: ");
            }
        }
#else
        DEBUG_VNC("Server ask for password?");
#endif
        if(!read_from_rfb_server(sock, (char *) challenge_and_response, CHALLENGESIZE))
            return 0;
        vncEncryptBytes(challenge_and_response, opt.password);
        if(!write_exact(sock, (char *) challenge_and_response, CHALLENGESIZE))
            return 0;
        if(!read_from_rfb_server(sock, (char*) &auth_result, 4))
            return 0;
        auth_result = Swap32IfLE(auth_result);
        switch(auth_result) {
            case rfbVncAuthFailed:
                DEBUG_VNC("Authentication Failed\n");
                return (0);
            case rfbVncAuthTooMany:
                DEBUG_VNC("Too many connections\n");
                return (0);
            case rfbVncAuthOK:
                DEBUG_VNC("Authentication OK\n");
                break;
        }
        break;
    case rfbNoAuth:
        break;
    }
    return 1;
}

int arduinoVNC::_rfb_initialise_client() {
    rfbClientInitMsg cl;
    cl.shared = opt.shared;
    if(!write_exact(sock, (char *) &cl, sz_rfbClientInitMsg))
        return 0;

    return 1;
}

int arduinoVNC::_rfb_initialise_server() {
    int len;
    rfbServerInitMsg si;

    if(!read_from_rfb_server(sock, (char *) &si, sz_rfbServerInitMsg))
        return 0;

    opt.server.width = Swap16IfLE(si.framebufferWidth);
    opt.server.height = Swap16IfLE(si.framebufferHeight);

    // never be bigger then the client!
    opt.server.width = min(opt.client.width, opt.server.width);
    opt.server.height = min(opt.client.height, opt.server.height);

    opt.server.bpp = si.format.bitsPerPixel;
    opt.server.depth = si.format.bigEndian;
    opt.server.truecolour = si.format.trueColour;

    opt.server.redmax = Swap16IfLE(si.format.redMax);
    opt.server.greenmax = Swap16IfLE(si.format.greenMax);
    opt.server.bluemax = Swap16IfLE(si.format.blueMax);
    opt.server.redshift = si.format.redShift;
    opt.server.greenshift = si.format.greenShift;
    opt.server.blueshift = si.format.blueShift;

    len = Swap32IfLE(si.nameLength);
    opt.server.name = (char *) malloc(sizeof(char) * len + 1);

    if(!read_from_rfb_server(sock, opt.server.name, len))
        return 0;

    return 1;
}

//#############################################################################################
//                                      Connection handling
//#############################################################################################

int arduinoVNC::rfb_set_format_and_encodings() {
    int num_enc = 0;
    rfbSetPixelFormatMsg pf;
    rfbSetEncodingsMsg em;
    CARD32 enc[MAX_ENCODINGS];

    pf.type = 0;
    pf.format.bitsPerPixel = opt.client.bpp;
    pf.format.depth = opt.client.depth;
    pf.format.bigEndian = opt.client.bigendian;
    pf.format.trueColour = opt.client.truecolour;
    pf.format.redMax = Swap16IfLE(opt.client.redmax);
    pf.format.greenMax = Swap16IfLE(opt.client.greenmax);
    pf.format.blueMax = Swap16IfLE(opt.client.bluemax);
    pf.format.redShift = opt.client.redshift;
    pf.format.greenShift = opt.client.greenshift;
    pf.format.blueShift = opt.client.blueshift;

    if(!write_exact(sock, (char*) &pf, sz_rfbSetPixelFormatMsg))
        return 0;

    em.type = rfbSetEncodings;
    em.nEncodings = Swap16IfLE(0);

#ifdef VNC_TIGHT
    enc[num_enc++] = Swap32IfLE(rfbEncodingTight);
#endif
#ifdef VNC_HEXTILE
    enc[num_enc++] = Swap32IfLE(rfbEncodingHextile);
#endif
#ifdef VNC_ZLIB
    enc[num_enc++] = Swap32IfLE(rfbEncodingZlib);
#endif

    if(display->hasCopyRect()) {
        enc[num_enc++] = Swap32IfLE(rfbEncodingCopyRect);
    }

#ifdef VNC_CORRE
    enc[num_enc++] = Swap32IfLE(rfbEncodingRRE);
    enc[num_enc++] = Swap32IfLE(rfbEncodingCoRRE);
#endif
    enc[num_enc++] = Swap32IfLE(rfbEncodingRaw);

#if 0
    /* Track cursor locally */
    if (opt.localcursor)
    enc[num_enc++] = Swap32IfLE(rfbEncodingRichCursor);

    if (opt.client.compresslevel <= 9)
    enc[num_enc++] = Swap32IfLE(rfbEncodingCompressLevel0 +
            opt.client.compresslevel);
    if (opt.client.quality <= 9)
    enc[num_enc++] = Swap32IfLE(rfbEncodingQualityLevel0 +
            opt.client.quality);
#endif
    em.nEncodings = Swap16IfLE(num_enc);

    if(!write_exact(sock, (char*) &em, sz_rfbSetEncodingsMsg))
        return 0;
    if(!write_exact(sock, (char*) &enc, num_enc * 4))
        return 0;

    return (1);
}

int arduinoVNC::rfb_send_update_request(int incremental) {
    rfbFramebufferUpdateRequestMsg urq = { 0 };

    urq.type = rfbFramebufferUpdateRequest;
    urq.incremental = incremental;
    urq.x = 0;
    urq.y = 0;
    urq.w = opt.server.width;
    urq.h = opt.server.height;

    urq.x = Swap16IfLE(urq.x);
    urq.y = Swap16IfLE(urq.y);
    urq.w = Swap16IfLE(urq.w);
    urq.h = Swap16IfLE(urq.h);

    if(!write_exact(sock, (char*) &urq, sz_rfbFramebufferUpdateRequestMsg))
        return 0;

    return 1;
}

int arduinoVNC::rfb_handle_server_message() {
    int size;
    char *buf;
    rfbServerToClientMsg msg = { 0 };
    rfbFramebufferUpdateRectHeader rectheader = { 0 };

    if(!read_from_rfb_server(sock, (char*) &msg, 1))
        return 0;
    switch(msg.type) {
        int i;
    case rfbFramebufferUpdate:
        read_from_rfb_server(sock, ((char*) &msg.fu) + 1, sz_rfbFramebufferUpdateMsg - 1);
        msg.fu.nRects = Swap16IfLE(msg.fu.nRects);
        for(i = 0; i < msg.fu.nRects; i++) {
            read_from_rfb_server(sock, (char*) &rectheader,
            sz_rfbFramebufferUpdateRectHeader);
            rectheader.r.x = Swap16IfLE(rectheader.r.x);
            rectheader.r.y = Swap16IfLE(rectheader.r.y);
            rectheader.r.w = Swap16IfLE(rectheader.r.w);
            rectheader.r.h = Swap16IfLE(rectheader.r.h);
            rectheader.encoding = Swap32IfLE(rectheader.encoding);
            //SoftCursorLockArea(rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);
            switch(rectheader.encoding) {
                case rfbEncodingRaw:
                    _handle_raw_encoded_message(rectheader);
                    break;
                case rfbEncodingCopyRect:
                    _handle_copyrect_encoded_message(rectheader);
                    break;
#ifdef VNC_CORRE
                    case rfbEncodingRRE:
                    _handle_rre_encoded_message(rectheader);
                    break;
                    case rfbEncodingCoRRE:
                    _handle_corre_encoded_message(rectheader);
                    break;
#endif
#ifdef VNC_HEXTILE
                    case rfbEncodingHextile:
                    _handle_hextile_encoded_message(rectheader);
                    break;
#endif
#ifdef VNC_TIGHT
                    case rfbEncodingTight:
                    _handle_tight_encoded_message(rectheader);
                    break;
#endif
#ifdef VNC_ZLIB
                    case rfbEncodingZlib:
                    _handle_zlib_encoded_message(rectheader);
                    break;
#endif
                case rfbEncodingRichCursor:
                    _handle_richcursor_message(rectheader);
                    break;
                case rfbEncodingLastRect:
                    DEBUG_VNC("LAST\n");
                    break;
                default:
                    DEBUG_VNC("Unknown encoding 0x%08X\n", rectheader.encoding);
                    return 0;
                    break;
            }
            /* Now we may discard "soft cursor locks". */
            //SoftCursorUnlockScreen();
        }
        break;
    case rfbSetColourMapEntries:
        DEBUG_VNC("SetColourMapEntries\n");
        read_from_rfb_server(sock, ((char*) &msg.scme) + 1, sz_rfbSetColourMapEntriesMsg - 1);
        break;
    case rfbBell:
        DEBUG_VNC("Bell message. Unimplemented.\n");
        break;
    case rfbServerCutText:
        DEBUG_VNC("ServerCutText. Unimplemented.\n");
        read_from_rfb_server(sock, ((char*) &msg.sct) + 1,
        sz_rfbServerCutTextMsg - 1);
        size = Swap32IfLE(msg.sct.length);
        buf = (char *) malloc(sizeof(char) * size);
        read_from_rfb_server(sock, buf, size);
        buf[size] = 0;
        DEBUG_VNC("%s\n", buf);
        free(buf);
        break;
    default:
        DEBUG_VNC("Unknown server message. Type: %i\n", msg.type);
        TCPclient.stop();
        return 0;
        break;
    }
    return (1);
}

int arduinoVNC::rfb_update_mouse() {
    rfbPointerEventMsg msg;

    if(mousestate.x < 0)
        mousestate.x = 0;
    if(mousestate.y < 0)
        mousestate.y = 0;

    if(mousestate.x > opt.client.width)
        mousestate.x = opt.client.width;
    if(mousestate.y > opt.client.height)
        mousestate.y = opt.client.height;

    msg.type = rfbPointerEvent;
    msg.buttonMask = mousestate.buttonmask;

    /* scale to server resolution */
    msg.x = rint(mousestate.x * opt.h_ratio);
    msg.y = rint(mousestate.y * opt.v_ratio);

    SoftCursorMove(msg.x, msg.y);

    msg.x = Swap16IfLE(msg.x);
    msg.y = Swap16IfLE(msg.y);

    return (write_exact(sock, (char*) &msg, sz_rfbPointerEventMsg));
}

int arduinoVNC::rfb_send_key_event(int key, int down_flag) {
    rfbKeyEventMsg ke;

    ke.type = rfbKeyEvent;
    ke.down = down_flag;
    ke.key = Swap32IfLE(key);

    return (write_exact(sock, (char*) &ke, sz_rfbKeyEventMsg));
}

void arduinoVNC::rfb_get_rgb_from_data(int *r, int *g, int *b, char *data) {
    CARD16 foo16;

    switch(opt.client.bpp) {
        case 8:
            DEBUG_VNC("FIXME unimplemented\n");
            break;
        case 16:
            memcpy(&foo16, data, 2);
            *r = ((foo16 >> opt.client.redshift) & opt.client.redmax) << 3;
            *g = ((foo16 >> opt.client.greenshift) & opt.client.greenmax) << 2;
            *b = ((foo16 >> opt.client.blueshift) & opt.client.bluemax) << 3;
            break;
        case 24:
        case 32:
            *r = data[2] & 0x00FF;
            *g = data[1] & 0x00FF;
            *b = data[0] & 0x00FF;
            break;
    }
}

//#############################################################################################
//                                      Encode handling
//#############################################################################################

int arduinoVNC::_handle_raw_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    DEBUG_VNC("_handle_raw_encoded_message x: %d y: %d w: %d h: %d!\n", rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);

    uint32_t size = (opt.client.bpp / 8 * rectheader.r.w) * rectheader.r.h;
    char *buf = NULL;

    // max use 33% of the free HEAP
    if(size < (ESP.getFreeHeap() / 3)) {
        buf = (char *) malloc(size);
    } else {
        DEBUG_VNC("_handle_raw_encoded_message update to big for ram split %d! Free: %d\n", size, ESP.getFreeHeap());
        size = (opt.client.bpp / 8);
    }
    if(!buf) {
        //to less memory split in little blocks
        // while(size) {

        for(uint32_t y = rectheader.r.y; y < (rectheader.r.h + rectheader.r.y); y++) {
            // for each column
            for(uint32_t x = rectheader.r.x; x < (rectheader.r.w + rectheader.r.x); x++) {
                buf = (char *) malloc(size);
                if(!buf) {
                    DEBUG_VNC("TO LESS MEMEORE TO HANDLE DATA!");
                    return 0;
                }

                if(!read_from_rfb_server(sock, buf, size))
                    return 0;

                // send data to display
                display->draw_area(x, y, 1, 1, (uint8_t *) buf);

                free(buf);
            }
        }
        //  }
    } else {
        if(!read_from_rfb_server(sock, buf, size)) {
            return 0;
        }

        // send data to display
        display->draw_area(rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h, (uint8_t *) buf);

        free(buf);
        return 1;
    }
}

int arduinoVNC::_handle_copyrect_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    int src_x, src_y;

    if(!read_from_rfb_server(sock, (char*) &src_x, 2))
        return 0;
    if(!read_from_rfb_server(sock, (char*) &src_y, 2))
        return 0;

    /* If RichCursor encoding is used, we should extend our
     "cursor lock area" (previously set to destination
     rectangle) to the source rectangle as well. */
    //SoftCursorLockArea(src_x, src_y, rectheader.r.w, rectheader.r.h);
    display->copy_rect(Swap16IfLE(src_x), Swap16IfLE(src_y), rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);
    return 1;
}

#ifdef VNC_CORRE
int arduinoVNC::_handle_rre_encoded_message(rfbFramebufferUpdateRectHeader rectheader)
{
    rfbRREHeader header;
    char *colour;
    CARD16 rect[4];
    int i;
    int r=0, g=0, b=0;

    colour = (char *) malloc(sizeof(opt.client.bpp/8));
    if (!read_from_rfb_server(sock, (char *)&header, sz_rfbRREHeader)) return 0;
    header.nSubrects = Swap32IfLE(header.nSubrects);

    /* draw background rect */
    if (!read_from_rfb_server(sock, colour, opt.client.bpp/8)) return 0;
    rfb_get_rgb_from_data(&r, &g, &b, colour);
    dfb_draw_rect_with_rgb(
            rectheader.r.x,
            rectheader.r.y,
            rectheader.r.w,
            rectheader.r.h,
            r,g,b
    );

    /* subrect pixel values */
    for (i=0;i<header.nSubrects;i++)
    {
        if (!read_from_rfb_server(sock, colour, opt.client.bpp/8))
        return 0;
        rfb_get_rgb_from_data(&r, &g, &b, colour);
        if (!read_from_rfb_server(sock, (char *)&rect, sizeof(rect)))
        return 0;
        dfb_draw_rect_with_rgb(
                Swap16IfLE(rect[0]) + rectheader.r.x,
                Swap16IfLE(rect[1]) + rectheader.r.y,
                Swap16IfLE(rect[2]),
                Swap16IfLE(rect[3]),
                r,g,b
        );
    }
    free (colour);
    return 1;
}

int arduinoVNC::_handle_corre_encoded_message(rfbFramebufferUpdateRectHeader rectheader)
{
    rfbRREHeader header;
    char *colour;
    CARD8 rect[4];
    int i;
    int r=0, g=0, b=0;

    colour = (char *) malloc(sizeof(opt.client.bpp/8));
    if (!read_from_rfb_server(sock, (char *)&header, sz_rfbRREHeader)) return 0;
    header.nSubrects = Swap32IfLE(header.nSubrects);

    /* draw background rect */
    if (!read_from_rfb_server(sock, colour, opt.client.bpp/8)) return 0;
    rfb_get_rgb_from_data(&r, &g, &b, colour);
    dfb_draw_rect_with_rgb(
            rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h, r,g,b);

    /* subrect pixel values */
    for (i=0;i<header.nSubrects;i++)
    {
        if (!read_from_rfb_server(sock, colour, opt.client.bpp/8))
        return 0;
        rfb_get_rgb_from_data(&r, &g, &b, colour);
        if (!read_from_rfb_server(sock, (char *)&rect, sizeof(rect)))
        return 0;
        dfb_draw_rect_with_rgb( rect[0] + rectheader.r.x,
                rect[1] + rectheader.r.y,
                rect[2], rect[3], r,g,b);
    }
    free (colour);
    return 1;
}
#endif

#ifdef VNC_HEXTILE
int arduinoVNC::_handle_hextile_encoded_message(rfbFramebufferUpdateRectHeader rectheader)
{
    int rect_x, rect_y, rect_w,rect_h, i=0,j=0, n;
    int tile_w = 16, tile_h = 16;
    int remaining_w, remaining_h;
    CARD8 subrect_encoding;
    int bpp = opt.client.bpp / 8;
    int nr_subr = 0;
    int x,y,w,h;
    int r=0, g=0, b=0;
    int fg_r=0, fg_g=0, fg_b=0;
    int bg_r=0, bg_g=0, bg_b=0;
    rect_w = remaining_w = rectheader.r.w;
    rect_h = remaining_h = rectheader.r.h;
    rect_x = rectheader.r.x;
    rect_y = rectheader.r.y;

    /* the rect is divided into tiles of width and height 16. Iterate over
     * those */
    while ((double)i < (double) rect_h/16)
    {
        /* the last tile in a column could be smaller than 16 */
        if ( (remaining_h -=16) <= 0) tile_h = remaining_h +16;

        j=0;
        while ((double)j < (double) rect_w/16)
        {
            /* the last tile in a row could also be smaller */
            if ( (remaining_w -= 16) <= 0 ) tile_w = remaining_w +16;

            if (!read_from_rfb_server(sock, (char*)&subrect_encoding, 1)) return 0;
            /* first, check if the raw bit is set */
            if (subrect_encoding & rfbHextileRaw)
            {
                if (!read_from_rfb_server(sock, buffer, bpp*tile_w*tile_h)) return 0;
                dfb_write_data_to_screen(
                        rect_x+(j*16), rect_y+(i*16), tile_w, tile_h, buffer);
            }
            else /* subrect encoding is not raw */
            {
                /* check whether theres a new bg or fg colour specified */
                if (subrect_encoding & rfbHextileBackgroundSpecified)
                {
                    if (!read_from_rfb_server(sock, buffer, bpp)) return 0;
                    rfb_get_rgb_from_data(&bg_r, &bg_g, &bg_b, buffer);
                }
                if (subrect_encoding & rfbHextileForegroundSpecified)
                {
                    if (!read_from_rfb_server(sock, buffer, bpp)) return 0;
                    rfb_get_rgb_from_data(&fg_r, &fg_g, &fg_b, buffer);
                }
                /* fill the background */
                dfb_draw_rect_with_rgb(
                        rect_x+(j*16), rect_y+(i*16), tile_w, tile_h, bg_r, bg_g, bg_b);

                if (subrect_encoding & rfbHextileAnySubrects)
                {
                    if (!read_from_rfb_server(sock, (char*)&nr_subr, 1)) return 0;
                    for (n=0;n<nr_subr;n++)
                    {
                        if (subrect_encoding & rfbHextileSubrectsColoured)
                        {
                            read_from_rfb_server(sock, buffer, bpp);
                            rfb_get_rgb_from_data(&r, &g, &b, buffer);

                            read_from_rfb_server(sock, buffer, 2);
                            x = rfbHextileExtractX( (CARD8) *buffer);
                            y = rfbHextileExtractY( (CARD8) *buffer);
                            w = rfbHextileExtractW( (CARD8)*(buffer+1));
                            h = rfbHextileExtractH( (CARD8)*(buffer+1));
                            dfb_draw_rect_with_rgb(
                                    x+(rect_x+(j*16)), y+(rect_y+(i*16)), w, h, r,g,b);
                        }
                        else
                        {
                            read_from_rfb_server(sock, buffer, 2);
                            x = rfbHextileExtractX( (CARD8) *buffer);
                            y = rfbHextileExtractY( (CARD8) *buffer);
                            w = rfbHextileExtractW( (CARD8)* (buffer+1));
                            h = rfbHextileExtractH( (CARD8)* (buffer+1));
                            dfb_draw_rect_with_rgb(
                                    x+(rect_x+(j*16)), y+(rect_y+(i*16)), w, h, fg_r,fg_g,fg_b);
                        }
                    }
                }
            }
            j++;
        }
        remaining_w = rectheader.r.w;
        tile_w = 16; /* reset for next row */
        i++;
    }
    return 1;
}
#endif

int arduinoVNC::_handle_richcursor_message(rfbFramebufferUpdateRectHeader rectheader) {
    return HandleRichCursor(rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);
}

//#############################################################################################
//                                      Encryption
//#############################################################################################

void arduinoVNC::vncRandomBytes(unsigned char *bytes) {
    srand(micros());
    for(int i = 0; i < CHALLENGESIZE; i++) {
        bytes[i] = (unsigned char) (rand() & 255);
    }
}

void arduinoVNC::vncEncryptBytes(unsigned char *bytes, char *passwd) {
    unsigned char key[8];
    size_t i;

    /* key is simply password padded with nulls */

    for(i = 0; i < 8; i++) {
        if(i < strlen(passwd)) {
            key[i] = passwd[i];
        } else {
            key[i] = 0;
        }
    }

    deskey(key, EN0);

    for(i = 0; i < CHALLENGESIZE; i += 8) {
        des(bytes + i, bytes + i);
    }
}

//#############################################################################################
//                                      Cursor
//#############################################################################################

int arduinoVNC::HandleRichCursor(int x, int y, int w, int h) {
    //todo handle Cursor
    return 0;
}

void arduinoVNC::SoftCursorMove(int x, int y) {
    //todo handle Cursor
}
