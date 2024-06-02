/*
 * @file VNC.cpp
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

#include <Arduino.h>

#include <math.h>

#include "VNC.h"

#ifdef VNC_FRAMEBUFFER
#include "frameBuffer.h"
#endif

#ifdef VNC_TIGHT
#include "tight.h"
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
    sock = 0;
    protocolMinorVersion = 3;
    onlyFullUpdate = false;
#ifdef VNC_RICH_CURSOR
    richCursorData = NULL;
    richCursorMask = NULL;
#endif
}

arduinoVNC::~arduinoVNC(void) {
    TCPclient.stop();
#ifdef VNC_RICH_CURSOR
    if(richCursorData) {
        freeSec(richCursorData);
    }
    if(richCursorMask) {
        freeSec(richCursorMask);
    }
#endif
}

void arduinoVNC::begin(char *_host, uint16_t _port, bool _onlyFullUpdate) {
    host = _host;
    port = _port;

#ifdef FPS_BENCHMARK
#ifdef FPS_BENCHMARK_FULL
    onlyFullUpdate = true;
#else
    onlyFullUpdate = false;
#endif
#else
    onlyFullUpdate = _onlyFullUpdate;
#endif

    opt.client.width = display->getWidth();
    opt.client.height = display->getHeight();

    opt.client.bpp = 16;
    opt.client.depth = 16;

    opt.client.bigendian = 1;
    opt.client.truecolour = 1;

    opt.client.redmax = 31;
    opt.client.greenmax = 63;
    opt.client.bluemax = 31;

    opt.client.redshift = 11;
    opt.client.greenshift = 5;
    opt.client.blueshift = 0;

#ifdef VNC_COMPRESS_LEVEL
    opt.client.compresslevel = VNC_COMPRESS_LEVEL;
#else
    opt.client.compresslevel = 99;
#endif
    opt.client.quality = 99;

    opt.shared = 1;
    opt.localcursor = 1;

    opt.h_ratio = 1;
    opt.v_ratio = 1;
    opt.h_offset = 0;
    opt.v_offset = 0;

    display->vnc_options_override(&opt);

    setMaxFPS(100);
}

void arduinoVNC::begin(const char *_host, uint16_t _port, bool _onlyFullUpdate) {
    begin((char *) _host, _port, _onlyFullUpdate);
}

void arduinoVNC::begin(String _host, uint16_t _port, bool _onlyFullUpdate) {
    begin(_host.c_str(), _port, _onlyFullUpdate);
}

void arduinoVNC::setPassword(char * pass) {
    password = pass;
    opt.password = (char *) password.c_str();
}

void arduinoVNC::setPassword(const char * pass) {
    password = pass;
    opt.password = (char *) password.c_str();
}

void arduinoVNC::setPassword(String pass) {
    password = pass;
    opt.password = (char *) password.c_str();
}

void arduinoVNC::loop(void) {

    static uint16_t fails = 0;
    static unsigned long lastUpdate = 0;

#if defined(ESP8266) || defined(ESP32)
    if(WiFi.status() != WL_CONNECTED) {
        if(connected()) {
            disconnect();
        }
        return;
    }
#endif

    if(!connected()) {
        DEBUG_VNC("!connected\n");
        if(!rfb_connect_to_server(host.c_str(), port)) {
            DEBUG_VNC("Couldnt establish connection with the VNC server. Exiting\n");
            delay(500);
            return;
        }

        /* initialize the connection */
        if(!rfb_initialise_connection()) {
            DEBUG_VNC("Connection with VNC server couldnt be initialized. Exiting\n");
            disconnect();
            delay(500);
            return;
        }

        /* Tell the VNC server which pixel format and encodings we want to use */
        if(!rfb_set_format_and_encodings()) {
            DEBUG_VNC("Error negotiating format and encodings. Exiting.\n");
            disconnect();
            delay(500);
            return;
        }

#ifdef SET_DESKTOP_SIZE
        /* set display resolution */
        if (!rfb_set_desktop_size()) {
            DEBUG_VNC("Error set desktop size. Exiting.\n");
            disconnect();
            delay(500);
            return;
        }
#endif

        /* calculate horizontal and vertical offset */
        if(opt.client.width > opt.server.width) {
            opt.h_offset = rint((opt.client.width - opt.server.width) / 2);
        }
        if(opt.client.height > opt.server.height) {
            opt.v_offset = rint((opt.client.height - opt.server.height) / 2);
        }

        mousestate.x = opt.client.width / 2;
        mousestate.y = opt.client.height / 2;

        // no scale support for embedded systems!
        opt.h_ratio = 1; //(double) opt.client.width / (double) opt.server.width;
        opt.v_ratio = 1; //(double) opt.client.height / (double) opt.server.height;


        rfb_send_update_request(0);
        //rfb_set_continuous_updates(1);

        DEBUG_VNC("vnc_connect Done.\n");

#if defined(VNC_ZLIB) || defined(VNC_ZRLE)
        if (!zin) {
            zin = (uint8_t *)malloc(ZRLE_INPUT_BUFFER);
        }
        if (!zin) {
            DEBUG_VNC("zin_buffer malloc failed!\n");
        }

        if (!zout) {
            zout = (uint8_t *)malloc(ZRLE_OUTPUT_BUFFER);
        }
        if (!zout) {
            DEBUG_VNC("zout malloc failed!\n");
        }

        tinfl_init(&inflator);
        // reset dict
        memset(zout, 0, ZRLE_OUTPUT_BUFFER);
        zout_next = zout;

#ifdef VNC_ZRLE
        zout_read = zout;
#endif // #ifdef VNC_ZRLE

#endif

    } else {
        if(!rfb_handle_server_message()) {
            //DEBUG_VNC("rfb_handle_server_message failed.\n");
            return;
        }

        if((millis() - lastUpdate) > updateDelay) {
            if(rfb_send_update_request(onlyFullUpdate ? 0 : 1)) {
                lastUpdate = millis();
                fails = 0;
            } else {
                fails++;
                if(fails > 20) {
                    disconnect();
                }
            }
        }
    }
#ifdef SLOW_LOOP
    delay(SLOW_LOOP);
#endif
}

int arduinoVNC::forceFullUpdate(void) {
    return rfb_send_update_request(0);
}

void arduinoVNC::setOffset(uint16_t x, uint16_t y) {
    opt.h_offset = x;
    opt.v_offset = y;
}

void arduinoVNC::setMaxFPS(uint16_t fps) {
    updateDelay = (1000/fps);
}


void arduinoVNC::mouseEvent(uint16_t x, uint16_t y, uint8_t buttonMask) {
    mousestate.x = x;
    mousestate.y = y;
    mousestate.buttonmask = buttonMask;
    rfb_update_mouse();
}

void arduinoVNC::keyEvent(int key, int keyMask)
{
  rfb_send_key_event(key, keyMask);
}

void arduinoVNC::reconnect(void) {
    // auto reconnect on next loop
    disconnect();
}

bool arduinoVNC::connected(void) {
#ifdef ESP8266
    return (TCPclient.status() == ESTABLISHED);
#else
    return TCPclient.connected();
#endif
}

//#############################################################################################
//                                       TCP handling
//#############################################################################################

#ifdef USE_ARDUINO_TCP

bool arduinoVNC::read_from_rfb_server(int sock, char *out, size_t n) {
    unsigned long t = millis();
    size_t len;
    /*
     DEBUG_VNC("read_from_rfb_server %d...\n", n);

     if(n > 2 * 1024 * 1042) {
     DEBUG_VNC("read_from_rfb_server N: %d 0x%08X make no sens!\n", n, n);
     return 0;
     }

     if(out == NULL) {
     DEBUG_VNC("read_from_rfb_server out == NULL!\n");
     return 0;
     }
     */
    while(n > 0) {
        if(!connected()) {
            DEBUG_VNC("[read_from_rfb_server] not connected!\n");
            return false;
        }

        if((millis() - t) > VNC_TCP_TIMEOUT) {
            DEBUG_VNC("[read_from_rfb_server] receive TIMEOUT!\n");
            return false;
        }

        if(!TCPclient.available()) {
            delay(0);
            continue;
        }

        len = TCPclient.read((uint8_t*) out, n);
        if(len) {
            t = millis();
            out += len;
            n -= len;
            //DEBUG_VNC("Receive %d left %d!\n", len, n);
        } else {
            //DEBUG_VNC("Receive %d left %d!\n", len, n);
        }
        delay(0);
    }
    return true;
}

#ifdef VNC_ZRLE
bool arduinoVNC::read_from_z(uint8_t *out, size_t n) {
    // Make our life a bit easier
    if(n > ZRLE_OUTPUT_BUFFER) {
        DEBUG_VNC("[read_from_z] Cannot read more than %d bytes (%d)\n", ZRLE_OUTPUT_BUFFER, n);
        return false;
    }

    bool decompress_more = true;

    // Check if we already decompressed enough data in the run before
    if(zout_read != zout_next) {
        size_t buf_size = 0;
        if(zout_read < zout_next) {
            buf_size = zout_next - zout_read;
        } else {
            buf_size = ZRLE_OUTPUT_BUFFER - (zout_read - zout_next);
        }
        if(buf_size < n) {
            DEBUG_VNC_ZRLE("[read_from_z] Partially in buffer: %d of %d\n", buf_size, n);
            // Data is only partially available
            memcpy(out, zout_read, buf_size);
            out += buf_size;
            n -= buf_size;
            zout_read = zout_next;
        }  else {
            decompress_more = false;
        }
    }

    while(decompress_more) {
        DEBUG_VNC_ZRLE("[read_from_z] dict: %zu next: %zu avail: %zu\n", zout, zout_next, ZRLE_OUTPUT_BUFFER - (zout_next-zout));
        if(!bytes_available) {
            if(!msg_bytes_remain) {
                DEBUG_VNC("[read_from_z] Empty buffer, but %d missing\n", n);
                return false;
            }
            bytes_available = min(msg_bytes_remain, (size_t)ZRLE_INPUT_BUFFER);
            DEBUG_VNC_ZRLE("[read_from_z] Reading %d\n", bytes_available);
            if (!read_from_rfb_server(sock, (char *)zin, bytes_available)) {
                DEBUG_VNC("[read_from_z] Failed reading from socket %d!\n", bytes_available);
                return false;
            }
            msg_bytes_remain -= bytes_available;
            zin_next = zin;
        }

        size_t bytes_decompressed = zout + ZRLE_OUTPUT_BUFFER - zout_next;
        size_t bytes_consumed = bytes_available;
        // We cannot decompress into "out" directly, because it would have to have at least ZRLE_OUTPUT_BUFFER capacity
        tinfl_status last_status = tinfl_decompress(&inflator, zin_next, &bytes_consumed, zout, zout_next, &bytes_decompressed, TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_PARSE_ZLIB_HEADER);
        bytes_available -= bytes_consumed;
        zin_next += bytes_consumed;

        zout_next += bytes_decompressed;
        if (zout_next >= zout + ZRLE_OUTPUT_BUFFER) {
            zout_next = zout;
        }

        if(bytes_decompressed < n) {
            memcpy(out, zout_read, bytes_decompressed);
            out += bytes_decompressed;
            n -= bytes_decompressed;
            zout_read = zout_next;
        } else {
            decompress_more = false;
        }

        DEBUG_VNC_ZRLE("[read_from_z] Available: %zu Consumed: %zu Decompressed: %zu Missing: %zu AvailOut: %zu Status: %d\n", bytes_available, bytes_consumed, bytes_decompressed, bytes_missing, ZRLE_OUTPUT_BUFFER - (zout_next-zout), last_status);
        if(last_status < TINFL_STATUS_NEEDS_MORE_INPUT) {
            DEBUG_VNC("[read_from_z] Error during decompression: %d\n", last_status);
            return false;
        }
    }

    // Copy data from decompression buffer into "out"
    memcpy(out, zout_read, n);
    zout_read += n;

    if (zout_read >= zout + ZRLE_OUTPUT_BUFFER) {
        zout_read = zout;
    }

    return true;
}
#endif // #ifdef VNC_ZRLE

bool arduinoVNC::write_exact(int sock, char *buf, size_t n) {
    if(!connected()) {
        DEBUG_VNC("[write_exact] not connected!\n");
        return false;
    }
    return (TCPclient.write((uint8_t*) buf, n) == n);
}

bool arduinoVNC::set_non_blocking(int sock) {
#if defined(ESP8266) || defined(ESP32)
    TCPclient.setNoDelay(true);
#endif
    return true;
}

void arduinoVNC::disconnect(void) {
    DEBUG_VNC("[arduinoVNC] disconnect...\n");
    TCPclient.stop();
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
bool arduinoVNC::rfb_connect_to_server(const char *host, int port) {
#ifdef USE_ARDUINO_TCP
    if(!TCPclient.connect(host, port)) {
        DEBUG_VNC("[rfb_connect_to_server] Connect error\n");
        return false;
    }

    DEBUG_VNC("[rfb_connect_to_server] Connected.\n");
    set_non_blocking(sock);
    return true;
#else
    struct hostent *he=NULL;
    int one=1;
    struct sockaddr_in s;

    if ( (sock = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        DEBUG_VNC("Error creating communication socket: %d\n", errno);
        //exit(2)
        return false;
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
            return false;
        }
    }

    s.sin_port = htons(port);
    s.sin_family = AF_INET;

    if (connect(sock,(struct sockaddr*) &s, sizeof(s)) < 0)
    {
        DEBUG_VNC("Connect error\n");
        close(sock);
        return false;
    }
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)) < 0)
    {
        DEBUG_VNC("Error setting socket options\n");
        close(sock);
        return false;
    }

    if (!set_non_blocking(sock)) return -1;

    return (sock);
#endif
}

bool arduinoVNC::rfb_initialise_connection() {
    if(!_rfb_negotiate_protocol()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_negotiate_protocol()  Failed!\n");
        return false;
    }

    if(!_rfb_authenticate()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_authenticate()  Failed!\n");
        return false;
    }

    if(!_rfb_initialise_client()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_initialise_client()  Failed!\n");
        return false;
    }

    if(!_rfb_initialise_server()) {
        DEBUG_VNC("[rfb_initialise_connection] _rfb_initialise_server() Failed!\n");
        return false;
    }

    return true;
}

bool arduinoVNC::_rfb_negotiate_protocol() {
    uint16_t server_major, server_minor;

    rfbProtocolVersionMsg msg;

    /* read the protocol version the server uses */
    if(!read_from_rfb_server(sock, (char*) &msg, sz_rfbProtocolVersionMsg))
        return false;

    //RFB xxx.yyy
    if(msg[0] != 'R' || msg[1] != 'F' || msg[2] != 'B' || msg[3] != ' ' || msg[7] != '.') {
        DEBUG_VNC("[_rfb_negotiate_protocol] Not a valid VNC server\n");
        return false;
    }
    msg[11] = 0x00;

    DEBUG_VNC("[_rfb_negotiate_protocol] Server protocol: %s\n", msg);

    char majorStr[4] { msg[4], msg[5], msg[6], 0x00 };
    char minorStr[4] { msg[8], msg[9], msg[10], 0x00 };

    server_major = atol((const char *) &majorStr);
    server_minor = atol((const char *) &minorStr);

    if(server_major == 3 && server_minor >= 8) {
        /* the server supports protocol 3.8 or higher version */
        protocolMinorVersion = 8;
    } else if(server_major == 3 && server_minor == 7) {
        /* the server supports protocol 3.7 */
        protocolMinorVersion = 7;
    } else {
        /* any other server version, request the standard 3.3 */
        protocolMinorVersion = 3;
    }

    /* send the protocol version we want to use */
    sprintf(msg, rfbProtocolVersionFormat, rfbProtocolMajorVersion, protocolMinorVersion);

    DEBUG_VNC("[_rfb_negotiate_protocol] used protocol: %s\n", msg);

    if(!write_exact(sock, msg, sz_rfbProtocolVersionMsg)) {
        return false;
    }

    return true;
}

bool arduinoVNC::_read_conn_failed_reason(void) {
    CARD32 reason_length;
    CARD8 *reason_string;
    DEBUG_VNC("[_read_conn_failed_reason] Connection to VNC server failed\n");

    if(!read_from_rfb_server(sock, (char *) &reason_length, sizeof(CARD32))) {
        return false;
    }

    reason_length = Swap32IfLE(reason_length);
    reason_string = (CARD8 *) malloc(sizeof(CARD8) * reason_length);

    if(!reason_string) {
        return false;
    }

    if(!read_from_rfb_server(sock, (char *) reason_string, reason_length)) {
        freeSec(reason_string);
        return false;
    }

    DEBUG_VNC("[_read_conn_failed_reason] Errormessage: %s\n", reason_string);
    freeSec(reason_string);
    return true;
}

bool arduinoVNC::_read_authentication_result(void) {
    CARD32 auth_result;
    if(!read_from_rfb_server(sock, (char*) &auth_result, 4)) {
        return false;
    }

    auth_result = Swap32IfLE(auth_result);
    switch(auth_result) {
        case rfbAuthFailed:
            DEBUG_VNC("Authentication Failed\n");
            return false;
        case rfbAuthTooMany:
            DEBUG_VNC("Too many connections\n");
            return false;
        case rfbAuthOK:
            DEBUG_VNC("Authentication OK\n");
            return true;
        default:
            DEBUG_VNC("Unknown result of authentication: 0x%08X (%d)\n", auth_result, auth_result);
            return false;
    }
}

bool arduinoVNC::_rfb_authenticate() {

    CARD32 authscheme;
    CARD8 challenge_and_response[CHALLENGESIZE];

    if(protocolMinorVersion >= 7) {
        CARD8 secType = rfbSecTypeInvalid;

        CARD8 nSecTypes;
        CARD8 *secTypes;

        CARD8 knownSecTypes[] = { rfbSecTypeNone, rfbSecTypeVncAuth };
        uint8_t nKnownSecTypes = sizeof(knownSecTypes);

        if(!read_from_rfb_server(sock, (char *) &nSecTypes, sizeof(nSecTypes))) {
            return false;
        }

        if(nSecTypes == 0) {
            if(!_read_conn_failed_reason()) {
                return false;
            }
        }

        secTypes = (CARD8 *) malloc(nSecTypes);

        if(!secTypes) {
            return false;
        }

        if(!read_from_rfb_server(sock, (char *) secTypes, nSecTypes)) {
            freeSec(secTypes);
            return false;
        }
#ifdef VNC_SEC_TYPE_TIGHT
        // Prefere rfbSecTypeTight
        for(uint8_t i = 0; i < nSecTypes; i++) {
            if(secTypes[i] == rfbSecTypeTight) {
                secType = rfbSecTypeTight;
                break;
            }
        }
#endif
        if(secType == rfbSecTypeInvalid) {
            // use first supported security type
            for(uint8_t x = 0; x < nSecTypes; x++) {
                for(uint8_t i = 0; i < nKnownSecTypes; i++) {
                    if(secTypes[x] == knownSecTypes[i]) {
                        secType = secTypes[x];
                        break;
                    }
                }
                if(secType != rfbSecTypeInvalid) {
                    break;
                }
            }
        }

        freeSec(secTypes);

        if(!write_exact(sock, (char *) &secType, sizeof(secType))) {
            return false;
        }

        if(secType == rfbSecTypeInvalid) {
            DEBUG_VNC("Server did not offer supported security type\n");
        }

        authscheme = secType;
    } else {
        // protocol Minor < 7
        if(!read_from_rfb_server(sock, (char *) &authscheme, 4)) {
            return false;
        }
        authscheme = Swap32IfLE(authscheme);
        if(authscheme == rfbSecTypeInvalid) {
            if(!_read_conn_failed_reason()) {
                return false;
            }
        }
    }

    switch(authscheme) {
        case rfbSecTypeInvalid:
            return false;
            break;
        case rfbSecTypeNone:
            if(protocolMinorVersion >= 8) {
                return _read_authentication_result();
            }
            return true;
            break;
        case rfbSecTypeTight:
#ifdef VNC_SEC_TYPE_TIGHT
            //todo implement rfbSecTypeTight
#endif
            return false;
            break;
        case rfbSecTypeVncAuth:

            if(!opt.password || *(opt.password) == 0x00) {
                DEBUG_VNC("Server ask for password? but no Password is set.\n");
                return false;
            }

            if(!read_from_rfb_server(sock, (char *) challenge_and_response, CHALLENGESIZE)) {
                return false;
            }

            vncEncryptBytes(challenge_and_response, opt.password);
            if(!write_exact(sock, (char *) challenge_and_response, CHALLENGESIZE)) {
                return false;
            }
            return _read_authentication_result();
            break;
    }

    return false;
}

bool arduinoVNC::_rfb_initialise_client() {
    rfbClientInitMsg cl;
    cl.shared = opt.shared;
    if(!write_exact(sock, (char *) &cl, sz_rfbClientInitMsg)) {
        return false;
    }

    return true;
}

bool arduinoVNC::_rfb_initialise_server() {
    int len;
    rfbServerInitMsg si;

    if(!read_from_rfb_server(sock, (char *) &si, sz_rfbServerInitMsg)) {
        return false;
    }

    opt.server.width = Swap16IfLE(si.framebufferWidth);
    opt.server.height = Swap16IfLE(si.framebufferHeight);

    // never be bigger then the client!
    opt.server.width = min(opt.client.width, opt.server.width);
    opt.server.height = min(opt.client.height, opt.server.height);

    opt.server.bpp = si.format.bitsPerPixel;
    opt.server.depth = si.format.depth;
    opt.server.bigendian = si.format.bigEndian;
    opt.server.truecolour = si.format.trueColour;

    opt.server.redmax = Swap16IfLE(si.format.redMax);
    opt.server.greenmax = Swap16IfLE(si.format.greenMax);
    opt.server.bluemax = Swap16IfLE(si.format.blueMax);
    opt.server.redshift = si.format.redShift;
    opt.server.greenshift = si.format.greenShift;
    opt.server.blueshift = si.format.blueShift;

    len = Swap32IfLE(si.nameLength);
    opt.server.name = (char *) malloc(sizeof(char) * len + 1);

    if(!read_from_rfb_server(sock, opt.server.name, len)) {
        return false;
    }
    opt.server.name[len] = 0x00;

    DEBUG_VNC("[VNC-SERVER] VNC Server config - Name: %s\n", opt.server.name);
    DEBUG_VNC(" - width:%d      height:%d\n", Swap16IfLE(si.framebufferWidth), Swap16IfLE(si.framebufferHeight));
    DEBUG_VNC(" - bpp:%d        depth:%d       bigendian:%d     truecolor:%d\n", opt.server.bpp, opt.server.depth, opt.server.bigendian, opt.server.truecolour);
    DEBUG_VNC(" - redmax:%d     greenmax:%d    bluemax:%d\n", opt.server.redmax, opt.server.greenmax, opt.server.bluemax);
    DEBUG_VNC(" - redshift:%d   greenshift:%d  blueshift:%d\n", opt.server.redshift, opt.server.greenshift, opt.server.blueshift);
    return true;
}

//#############################################################################################
//                                      Connection handling
//#############################################################################################

bool arduinoVNC::rfb_set_format_and_encodings() {
    uint8_t num_enc = 0;
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

    if(!write_exact(sock, (char*) &pf, sz_rfbSetPixelFormatMsg)) {
        return false;
    }

    em.type = rfbSetEncodings;

    DEBUG_VNC("[VNC-CLIENT] Supported Encodings:\n");
#ifdef VNC_ZRLE
    enc[num_enc++] = Swap32IfLE(rfbEncodingZRLE);
    DEBUG_VNC(" - ZRLE\n");
#endif
#ifdef VNC_TIGHT
    enc[num_enc++] = Swap32IfLE(rfbEncodingTight);
    DEBUG_VNC(" - Tight\n");
#endif
#ifdef VNC_HEXTILE
    enc[num_enc++] = Swap32IfLE(rfbEncodingHextile);
    DEBUG_VNC(" - Hextile\n");
#endif
#ifdef VNC_ZLIB
    enc[num_enc++] = Swap32IfLE(rfbEncodingZlib);
    DEBUG_VNC(" - Zlib\n");
#endif

    if(display->hasCopyRect()) {
        enc[num_enc++] = Swap32IfLE(rfbEncodingCopyRect);
        DEBUG_VNC(" - CopyRect\n");
    }

#ifdef VNC_RRE
    enc[num_enc++] = Swap32IfLE(rfbEncodingRRE);
    DEBUG_VNC(" - RRE\n");
#endif
#ifdef VNC_CORRE
    enc[num_enc++] = Swap32IfLE(rfbEncodingCoRRE);
    DEBUG_VNC(" - CoRRE\n");
#endif

    enc[num_enc++] = Swap32IfLE(rfbEncodingRaw);
    DEBUG_VNC(" - Raw\n");

    DEBUG_VNC("[VNC-CLIENT] Supported Special Encodings:\n");

#ifdef SET_DESKTOP_SIZE
    enc[num_enc++] = Swap32IfLE(rfbEncodingNewFBSize);
    DEBUG_VNC(" - SetDesktopSize\n");
#endif

#ifdef VNC_RICH_CURSOR
    enc[num_enc++] = Swap32IfLE(rfbEncodingRichCursor);
    DEBUG_VNC(" - RichCursor\n");

    enc[num_enc++] = Swap32IfLE(rfbEncodingXCursor);
    DEBUG_VNC(" - XCursor\n");
#endif

    enc[num_enc++] = Swap32IfLE(rfbEncodingPointerPos);
    DEBUG_VNC(" - CursorPos\n");

    //enc[num_enc++] = Swap32IfLE(rfbEncodingLastRect);
    //DEBUG_VNC(" - LastRect\n");

    enc[num_enc++] = Swap32IfLE(rfbEncodingContinuousUpdates);
    DEBUG_VNC(" - ContinuousUpdates\n");

    if (opt.client.compresslevel <= 9) {
        enc[num_enc++] = Swap32IfLE(rfbEncodingCompressLevel0 + opt.client.compresslevel);
        DEBUG_VNC(" - compresslevel: %d\n", opt.client.compresslevel);
    }
    if (opt.client.quality <= 9) {
        enc[num_enc++] = Swap32IfLE(rfbEncodingQualityLevel0 + opt.client.quality);
        DEBUG_VNC(" - quality: %d\n", opt.client.quality);
    }

    em.nEncodings = Swap16IfLE(num_enc);

    if(!write_exact(sock, (char*) &em, sz_rfbSetEncodingsMsg)) {
        return false;
    }

    if(!write_exact(sock, (char*) &enc, num_enc * 4)) {
        return false;
    }

    DEBUG_VNC("[VNC-CLIENT] Client pixel format:\n");
    DEBUG_VNC(" - width:%d      height:%d\n", opt.client.width, opt.client.height);
    DEBUG_VNC(" - bpp:%d        depth:%d        bigEndian:%d    trueColor:%d\n", opt.client.bpp, opt.client.depth, opt.client.bigendian, opt.client.truecolour);
    DEBUG_VNC(" - red-max:%d    green-max:%d    blue-max:%d\n", opt.client.redmax, opt.client.greenmax, opt.client.bluemax);
    DEBUG_VNC(" - red-shift:%d  green-shift:%d  blue-shift:%d\n", opt.client.redshift, opt.client.greenshift, opt.client.blueshift);

    return true;
}


#ifdef SET_DESKTOP_SIZE
bool arduinoVNC::rfb_set_desktop_size() {
    uint16_t w = opt.client.width;
    uint16_t h = opt.client.height;

    DEBUG_VNC("[rfb_set_desktop_size] setting desktop size to %dx%d\n", w, h);

    // override server resolution
    opt.server.width = w;
    opt.server.height = h;

    w = Swap16IfLE(w);
    h = Swap16IfLE(h);
    rfbSetDesktopSizeMsg ds;
    ds.type = rfbSetDesktopSize;
    ds.pad1 = 0;
    ds.width = w;
    ds.height = h;
    ds.numScreens = 1;
    ds.pad2 = 0;
    ds.layoutId = 1;
    ds.layoutX = 0;
    ds.layoutY = 0;
    ds.layoutWidth = w;
    ds.layoutHeight = h;
    ds.layoutFlag = 0;

    if (!write_exact(sock, (char *)&ds, sz_rfbSetDesktopSizeMsg)) {
        return false;
    }

    return true;
}
#endif

bool arduinoVNC::rfb_send_update_request(int incremental) {
    rfbFramebufferUpdateRequestMsg urq = { 0 };

    urq.type = rfbFramebufferUpdateRequest;
    urq.incremental = incremental;
    urq.x = opt.v_offset;
    urq.y = opt.h_offset;
    urq.w = opt.server.width;
    urq.h = opt.server.height;

    urq.x = Swap16IfLE(urq.x);
    urq.y = Swap16IfLE(urq.y);
    urq.w = Swap16IfLE(urq.w);
    urq.h = Swap16IfLE(urq.h);

    if(!write_exact(sock, (char*) &urq, sz_rfbFramebufferUpdateRequestMsg)) {
        DEBUG_VNC("[rfb_send_update_request] write_exact failed!\n");
        return false;
    }

    return true;
}

bool arduinoVNC::rfb_set_continuous_updates(bool enable) {
    rfbEnableContinuousUpdatesMsg urq = { 0 };

    urq.type = rfbEnableContinuousUpdates;
    urq.enable = enable;
    urq.x = opt.v_offset;
    urq.y = opt.h_offset;
    urq.w = opt.server.width;
    urq.h = opt.server.height;

    urq.x = Swap16IfLE(urq.x);
    urq.y = Swap16IfLE(urq.y);
    urq.w = Swap16IfLE(urq.w);
    urq.h = Swap16IfLE(urq.h);

    if(!write_exact(sock, (char*) &urq, sz_rfbEnableContinuousUpdatesMsg)) {
        DEBUG_VNC("[rfb_set_continuous_updates] write_exact failed!\n");
        return false;
    }

    return true;
}

bool arduinoVNC::rfb_handle_server_message() {

    rfbServerToClientMsg msg = { 0 };
    rfbFramebufferUpdateRectHeader rectheader = { 0 };

    if(TCPclient.available()) {
        if(!read_from_rfb_server(sock, (char*) &msg, 1)) {
            return false;
        }
        switch(msg.type) {
            case rfbFramebufferUpdate:
                read_from_rfb_server(sock, ((char*) &msg.fu) + 1, sz_rfbFramebufferUpdateMsg - 1);
                msg.fu.nRects = Swap16IfLE(msg.fu.nRects);
                for(uint16_t i = 0; i < msg.fu.nRects; i++) {
                    read_from_rfb_server(sock, (char*) &rectheader,
                    sz_rfbFramebufferUpdateRectHeader);
                    rectheader.r.x = Swap16IfLE(rectheader.r.x);
                    rectheader.r.y = Swap16IfLE(rectheader.r.y);
                    rectheader.r.w = Swap16IfLE(rectheader.r.w);
                    rectheader.r.h = Swap16IfLE(rectheader.r.h);
                    rectheader.encoding = Swap32IfLE(rectheader.encoding);
                    //SoftCursorLockArea(rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);

#ifdef FPS_BENCHMARK
                    unsigned long encodingStart = micros();
#endif
                    bool encodingResult = false;
                    switch(rectheader.encoding) {
                        case rfbEncodingRaw:
                            encodingResult = _handle_raw_encoded_message(rectheader);
                            break;
                        case rfbEncodingCopyRect:
                            encodingResult = _handle_copyrect_encoded_message(rectheader);
                            break;
#ifdef VNC_RRE
                        case rfbEncodingRRE:
                            encodingResult = _handle_rre_encoded_message(rectheader);
                            break;
#endif
#ifdef VNC_CORRE
                        case rfbEncodingCoRRE:
                            encodingResult = _handle_corre_encoded_message(rectheader);
                            break;
#endif
#ifdef VNC_HEXTILE
                        case rfbEncodingHextile:
                            encodingResult = _handle_hextile_encoded_message(rectheader);
                            break;
#endif
#ifdef VNC_ZRLE
                        case rfbEncodingZRLE:
                            encodingResult = _handle_zrle_encoded_message(rectheader);
                            break;
#endif
#ifdef VNC_TIGHT
                            case rfbEncodingTight:
                            encodingResult =_handle_tight_encoded_message(rectheader);
                            break;
#endif
#ifdef VNC_ZLIB
                            case rfbEncodingZlib:
                            encodingResult =_handle_zlib_encoded_message(rectheader);
                            break;
#endif
#ifdef VNC_RICH_CURSOR
                            case rfbEncodingXCursor:
                            case rfbEncodingRichCursor:
                            encodingResult = _handle_richcursor_message(rectheader);
                            break;
#endif
                        case rfbEncodingPointerPos:
                            encodingResult = _handle_cursor_pos_message(rectheader);
                            break;
                        case rfbEncodingContinuousUpdates:
                            encodingResult = _handle_server_continuous_updates_message(rectheader);
                            break;
                        case rfbEncodingLastRect:
                            DEBUG_VNC("[rfbEncodingLastRect] LAST\n");
                            encodingResult = true;
                            break;
#ifdef SET_DESKTOP_SIZE
                        case rfbEncodingNewFBSize:
                            DEBUG_VNC("[rfbEncodingNewFBSize]\n");
                            encodingResult = true;
                            break;
#endif
                        default:
                            DEBUG_VNC("Unknown encoding 0x%08X %d\n", rectheader.encoding, rectheader.encoding);
                            break;
                    }

#ifdef FPS_BENCHMARK
                    unsigned long encodingTime = micros() - encodingStart;
                    double fps = ((double) (1 * 1000 * 1000) / (double) encodingTime);
                    DEBUG_VNC("[Benchmark][0x%08X][%d]\t us: %d \tfps: %s \tHeap: %d\n", rectheader.encoding, rectheader.encoding, encodingTime, String(fps, 2).c_str(), ESP.getFreeHeap());
#endif
                    //wdt_enable(0);
                    if(!encodingResult) {
                        DEBUG_VNC("[0x%08X][%d] encoding Failed!\n", rectheader.encoding, rectheader.encoding);
                        disconnect();
                        return false;
                    } else {
                        //DEBUG_VNC("[0x%08X] encoding ok!\n", rectheader.encoding);
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
                if(!_handle_server_cut_text_message(&msg)) {
                    disconnect();
                    return false;
                }
                break;
            default:
                DEBUG_VNC("Unknown server message. Type: %d\n", msg.type);
                disconnect();
                return false;
                break;
        }
    }
    return true;
}


bool arduinoVNC::rfb_update_mouse() {
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
    msg.x = mousestate.x; //rint(mousestate.x * opt.h_ratio);
    msg.y = mousestate.y; //rint(mousestate.y * opt.v_ratio);

#ifdef VNC_RICH_CURSOR
    SoftCursorMove(msg.x, msg.y);
#endif

    msg.x = Swap16IfLE(msg.x);
    msg.y = Swap16IfLE(msg.y);

    return (write_exact(sock, (char*) &msg, sz_rfbPointerEventMsg));
}

bool arduinoVNC::rfb_send_key_event(int key, int down_flag) {
    rfbKeyEventMsg ke;

    ke.type = rfbKeyEvent;
    ke.down = down_flag;
    ke.key = Swap32IfLE(key);

    return (write_exact(sock, (char*) &ke, sz_rfbKeyEventMsg));
}

//#############################################################################################
//                                      Encode handling
//#############################################################################################

bool arduinoVNC::_handle_server_cut_text_message(rfbServerToClientMsg * msg) {

    DEBUG_VNC("[_handle_server_cut_text_message] work...\n");

    CARD32 size;
    char *buf;

    if(!read_from_rfb_server(sock, ((char*) &msg->sct) + 1, sz_rfbServerCutTextMsg - 1)) {
        return false;
    }
    size = Swap32IfLE(msg->sct.length);

    DEBUG_VNC("[_handle_server_cut_text_message] size: %d\n", size);

    buf = (char *) malloc((sizeof(char) * size) + 1);
    if(!buf) {
        DEBUG_VNC("[_handle_server_cut_text_message] no memory!\n");
        return false;
    }

    if(!read_from_rfb_server(sock, buf, size)) {
        freeSec(buf);
        return false;
    }

    buf[size] = 0;

    DEBUG_VNC("[_handle_server_cut_text_message] msg: %s\n", buf);

    freeSec(buf);
    return true;
}

bool arduinoVNC::_handle_raw_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {

    uint32_t msgPixelTotal = (rectheader.r.w * rectheader.r.h);
    uint32_t msgPixel = msgPixelTotal;
    uint32_t msgSize = (msgPixel * (opt.client.bpp / 8));
#ifdef VNC_SAVE_MEMORY
    uint32_t maxSize = (ESP.getFreeHeap() / 4); // max use 20% of the free HEAP
    char *buf = NULL;
#else
    static uint32_t maxSize = VNC_RAW_BUFFER;
    static char *buf = (char *) malloc(maxSize);
#endif

    DEBUG_VNC_RAW("[_handle_raw_encoded_message] x: %d y: %d w: %d h: %d bytes: %d!\n", rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h, msgSize);

    if(msgSize > maxSize) {
        msgPixel = (maxSize / (opt.client.bpp / 8));
        msgSize = (msgPixel * (opt.client.bpp / 8));
        DEBUG_VNC_RAW("[_handle_raw_encoded_message] update to big for ram split %d! Free: %d\n", msgSize, ESP.getFreeHeap());
    }

    DEBUG_VNC_RAW("[_handle_raw_encoded_message] msgPixel: %d msgSize: %d\n", msgPixel, msgSize);

    display->area_update_start(rectheader.r.x - opt.v_offset, rectheader.r.y - opt.h_offset, rectheader.r.w, rectheader.r.h);
#ifdef VNC_SAVE_MEMORY
    buf = (char *) malloc(msgSize);
#endif
    if(!buf) {
        DEBUG_VNC("[_handle_raw_encoded_message] TO LESS MEMORY TO HANDLE DATA!");
        return false;
    }

    while(msgPixelTotal) {
        DEBUG_VNC_RAW("[_handle_raw_encoded_message] Pixel left: %d\n", msgPixelTotal);

        if(msgPixelTotal < msgPixel) {
            msgPixel = msgPixelTotal;
            msgSize = (msgPixel * (opt.client.bpp / 8));
        }

        if(!read_from_rfb_server(sock, buf, msgSize)) {
#ifdef VNC_SAVE_MEMORY
            freeSec(buf);
#endif
            return false;
        }

        display->area_update_data(buf, msgPixel);

        msgPixelTotal -= msgPixel;
        delay(0);
    }

    display->area_update_end();

#ifdef VNC_SAVE_MEMORY
    freeSec(buf);
#endif

    DEBUG_VNC_RAW("[_handle_raw_encoded_message] ------------------------ Fin ------------------------\n");
    return true;
}

bool arduinoVNC::_handle_copyrect_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    int src_x, src_y;

    if(!read_from_rfb_server(sock, (char*) &src_x, 2)) {
        return false;
    }

    if(!read_from_rfb_server(sock, (char*) &src_y, 2)) {
        return false;
    }

    /* If RichCursor encoding is used, we should extend our
     "cursor lock area" (previously set to destination
     rectangle) to the source rectangle as well. */
    //SoftCursorLockArea(src_x, src_y, rectheader.r.w, rectheader.r.h);
    display->copy_rect(Swap16IfLE(src_x), Swap16IfLE(src_y), rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);
    return true;
}

#ifdef VNC_RRE
bool arduinoVNC::_handle_rre_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    rfbRREHeader header;
    uint16_t colour;
    CARD16 rect[4];

    if(!read_from_rfb_server(sock, (char *) &header, sz_rfbRREHeader)) {
        return false;
    }
    header.nSubrects = Swap32IfLE(header.nSubrects);

    /* draw background rect */
    if(!read_from_rfb_server(sock, (char *) &colour, sizeof(colour))) {
        return false;
    }

    display->draw_rect(rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h, Swap16IfLE(colour));

    /* subrect pixel values */
    for(uint32_t i = 0; i < header.nSubrects; i++) {
        if(!read_from_rfb_server(sock, (char *) &colour, sizeof(colour))) {
            return false;
        }
        if(!read_from_rfb_server(sock, (char *) &rect, sizeof(rect)))
            return false;
        display->draw_rect(
        Swap16IfLE(rect[0]) + rectheader.r.x,
        Swap16IfLE(rect[1]) + rectheader.r.y, Swap16IfLE(rect[2]), Swap16IfLE(rect[3]), Swap16IfLE(colour));
    }

    return true;
}
#endif

#ifdef VNC_CORRE
bool arduinoVNC::_handle_corre_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    rfbRREHeader header;
    uint16_t colour;
    CARD8 rect[4];

    if(!read_from_rfb_server(sock, (char *) &header, sz_rfbRREHeader)) {
        return false;
    }
    header.nSubrects = Swap32IfLE(header.nSubrects);

    /* draw background rect */
    if(!read_from_rfb_server(sock, (char *) &colour, sizeof(colour))) {
        return false;
    }
    display->draw_rect(rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h, Swap16IfLE(colour));

    /* subrect pixel values */
    for(uint32_t i = 0; i < header.nSubrects; i++) {
        if(!read_from_rfb_server(sock, (char *) &colour, sizeof(colour))) {
            return false;
        }
        if(!read_from_rfb_server(sock, (char *) &rect, sizeof(rect))) {
            return false;
        }
        display->draw_rect(rect[0] + rectheader.r.x, rect[1] + rectheader.r.y, rect[2], rect[3], Swap16IfLE(colour));
    }
    return true;
}
#endif

#ifdef VNC_HEXTILE
bool arduinoVNC::_handle_hextile_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    uint32_t rect_x, rect_y, rect_w, rect_h, i = 0, j = 0;
    uint32_t rect_xW, rect_yW;

    uint32_t tile_w = 16, tile_h = 16;
    int32_t remaining_w, remaining_h;

    CARD8 subrect_encoding;

    uint16_t fgColor;
    uint16_t bgColor;

    DEBUG_VNC_HEXTILE("[_handle_hextile_encoded_message] x: %d y: %d w: %d h: %d!\n", rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);

    //alloc max nedded size
#ifdef VNC_SAVE_MEMORY
    char * buf = (char *) malloc(255 * sizeof(HextileSubrectsColoured_t));
#else
    static char * buf = (char *) malloc(255 * sizeof(HextileSubrectsColoured_t));
#endif
    if(!buf) {
        DEBUG_VNC("[_handle_hextile_encoded_message] too less memory!\n");
        return false;
    }

    rect_w = remaining_w = rectheader.r.w;
    rect_h = remaining_h = rectheader.r.h;
    rect_x = rectheader.r.x;
    rect_y = rectheader.r.y;

    /* the rect is divided into tiles of width and height 16. Iterate over
     * those */
    while((i * 16) < rect_h) {
        /* the last tile in a column could be smaller than 16 */
        if((remaining_h -= 16) <= 0)
            tile_h = remaining_h + 16;

        j = 0;
        while((j * 16) < rect_w) {
            /* the last tile in a row could also be smaller */
            if((remaining_w -= 16) <= 0)
                tile_w = remaining_w + 16;

            if(!read_from_rfb_server(sock, (char*) &subrect_encoding, 1)) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                return false;
            }

            rect_xW = rect_x + (j * 16);
            rect_yW = rect_y + (i * 16);

            /* first, check if the raw bit is set */
            if(subrect_encoding & rfbHextileRaw) {
                rfbFramebufferUpdateRectHeader rawUpdate;

                rawUpdate.encoding = rfbEncodingRaw;

                rawUpdate.r.w = tile_w;
                rawUpdate.r.h = tile_h;

                rawUpdate.r.x = rect_xW;
                rawUpdate.r.y = rect_yW;

                if(!_handle_raw_encoded_message(rawUpdate)) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                    return false;
                }

            } else { /* subrect encoding is not raw */

                /* check whether theres a new bg or fg colour specified */
                if(subrect_encoding & rfbHextileBackgroundSpecified) {
                    if(!read_from_rfb_server(sock, (char *) &bgColor, sizeof(bgColor))) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                        return false;
                    }
                }

                if(subrect_encoding & rfbHextileForegroundSpecified) {
                    if(!read_from_rfb_server(sock, (char *) &fgColor, sizeof(fgColor))) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                        return false;
                    }
                }

                //DEBUG_VNC_HEXTILE("[_handle_hextile_encoded_message] subrect: x: %d y: %d w: %d h: %d\n", rect_xW, rect_yW, tile_w, tile_h);

#ifdef VNC_FRAMEBUFFER
                if(!fb.begin(tile_w, tile_h)) {
                    DEBUG_VNC("[_handle_hextile_encoded_message] too less memory!\n");
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                    return false;
                }

                /* fill the background */
                fb.draw_rect(0, 0, tile_w, tile_h, bgColor);
#else
                /* fill the background */
                display->draw_rect(rect_xW, rect_yW, tile_w, tile_h, Swap16IfLE(bgColor));
#endif

                if(subrect_encoding & rfbHextileAnySubrects) {
                    uint8_t nr_subr = 0;
                    if(!read_from_rfb_server(sock, (char*) &nr_subr, 1)) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                        return false;
                    }
                    //DEBUG_VNC_HEXTILE("[_handle_hextile_encoded_message] nr_subr: %d\n", nr_subr);
                    if(nr_subr) {
                        if(subrect_encoding & rfbHextileSubrectsColoured) {
                            if(!read_from_rfb_server(sock, buf, nr_subr * 4)) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                                return false;
                            }

                            HextileSubrectsColoured_t * bufPC = (HextileSubrectsColoured_t *) buf;
                            for(uint8_t n = 0; n < nr_subr; n++) {
                                //  DEBUG_VNC_HEXTILE("[_handle_hextile_encoded_message] Coloured nr_subr: %d bufPC: 0x%08X\n", n, bufPC);
#ifdef VNC_FRAMEBUFFER
                                fb.draw_rect(bufPC->x, bufPC->y, bufPC->w + 1, bufPC->h + 1, bufPC->color);
#else
                                display->draw_rect(rect_xW + bufPC->x, rect_yW + bufPC->y, bufPC->w+1, bufPC->h+1, Swap16IfLE(bufPC->color));
#endif
                                bufPC++;
                            }
                        } else {

                            if(!read_from_rfb_server(sock, buf, nr_subr * 2)) {
#ifdef VNC_SAVE_MEMORY
                freeSec(buf);
#endif
                                return false;
                            }

                            HextileSubrects_t * bufP = (HextileSubrects_t *) buf;
                            for(uint8_t n = 0; n < nr_subr; n++) {

                                // DEBUG_VNC_HEXTILE("[_handle_hextile_encoded_message] nr_subr: %d bufP: 0x%08X\n", n, bufP);
#ifdef VNC_FRAMEBUFFER
                                fb.draw_rect(bufP->x, bufP->y, bufP->w + 1, bufP->h + 1, fgColor);
#else
                                display->draw_rect(rect_xW + bufP->x, rect_yW + bufP->y, bufP->w+1, bufP->h+1, Swap16IfLE(fgColor));
#endif
                                bufP++;
                            }
                        }
                    }
                }
#ifdef VNC_FRAMEBUFFER
                display->draw_area(rect_xW - opt.v_offset, rect_yW - opt.h_offset, tile_w, tile_h, fb.getPtr());
#endif
            }
            j++;
            delay(0);
        }
        remaining_w = rectheader.r.w;
        tile_w = 16; /* reset for next row */
        i++;
    }

#ifdef VNC_SAVE_MEMORY
#ifdef VNC_FRAMEBUFFER
    fb.freeBuffer();
#endif
    freeSec(buf);
#endif

    DEBUG_VNC_HEXTILE("[_handle_hextile_encoded_message] ------------------------ Fin ------------------------\n");
    return true;
}
#endif

#ifdef VNC_ZLIB
bool arduinoVNC::_handle_zlib_encoded_message(rfbFramebufferUpdateRectHeader rectheader)
{
    rfbZlibHeader hdr;

    DEBUG_VNC_ZLIB("[_handle_zlib_encoded_message] New message with size %zux%zu\n", rectheader.r.w, rectheader.r.h);

    if (!read_from_rfb_server(sock, (char *)&hdr, sz_rfbZlibHeader)) {
        return false;
    }

    size_t remaining = Swap32IfLE(hdr.nBytes);

    DEBUG_VNC_ZLIB("[_handle_zlib_encoded_message] Byte size %zu\n", remaining);

    size_t processed = 0;

    zin_next = zin;
    mz_uint32 flags = TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_PARSE_ZLIB_HEADER;

    uint16_t w = rectheader.r.w;
    uint16_t h = rectheader.r.h;

    size_t bytes_available = 0;

    bool leftOver = false;

    int32_t xOffset = rectheader.r.x - opt.v_offset;
    int32_t yOffset = rectheader.r.y - opt.h_offset;

    int32_t xAvail = ((int32_t)display->getWidth())-xOffset;
    int32_t yAvail = ((int32_t)display->getHeight())-yOffset;

    bool allVisible =
        xOffset >= 0 &&
        yOffset >= 0 &&
        w <= xAvail &&
        h <= yAvail;

    bool allHidden =
        xOffset + w < 0 ||
        yOffset + h < 0 ||
        xOffset >= display->getWidth() ||
        yOffset >= display->getHeight();

    display->area_update_start(
        max((int32_t)0, xOffset),
        max((int32_t)0, yOffset),
        min(xAvail, (int32_t)w),
        min(yAvail, (int32_t)h)
        );

    DEBUG_VNC_ZLIB("[_handle_zlib_encoded_message] visi: %d hidden: %d\n", allVisible, allHidden);

    while (remaining) {
        size_t toRead = min(remaining, (size_t)ZRLE_INPUT_BUFFER - ((zin_next - zin) + bytes_available));

        /* Fill the buffer, obtaining data from the server. */
        if (!read_from_rfb_server(sock, (char*)zin_next, toRead)) {
            return false;
        }
        if(!bytes_available) {
            zin_next = zin;
        }
        remaining -= toRead;
        bytes_available += toRead;

        while(bytes_available) {
            size_t bytes_decompressed = zout + ZRLE_OUTPUT_BUFFER - zout_next;
            size_t bytes_consumed = bytes_available;

            tinfl_status last_status = tinfl_decompress(&inflator, zin_next, &bytes_consumed, zout, zout_next, &bytes_decompressed, flags);
            if(last_status < TINFL_STATUS_DONE) {
                DEBUG_VNC_ZLIB("[_handle_zlib_encoded_message] decoding failed: %d\n", last_status);
                return false;
            }
            bytes_available -= bytes_consumed;
            zin_next += bytes_consumed;
            // Consume byte left from last run
            if(leftOver) {
                bytes_decompressed++;
                zout_next--;
            }
            if(allVisible) {
                display->area_update_data((char *)zout_next, bytes_decompressed / 2);
                processed += bytes_decompressed / 2;
            } else if (allHidden){
                processed += bytes_decompressed / 2;
            } else {
                uint32_t n = 0;
                while(n+1 < bytes_decompressed) {
                    int32_t cX = (processed % w) + xOffset;
                    int32_t cY = (processed / w) + yOffset;
                    if(cX >= 0 && cY >= 0 && cX < display->getWidth() && cY < display->getHeight()) {
                        // This could be further optimized to consider line wrapping, but doesn't seem worth the effort
                        uint32_t printable = min(display->getWidth() - cX, (bytes_decompressed-n) / 2);
                        display->area_update_data((char *)zout_next+n, printable);
                        processed += printable;
                        n += printable*2;
                    } else {
                        processed++;
                        n += 2;
                    }
                }
            }
            zout_next += bytes_decompressed;
            // Check if we have a left over byte for next run
            leftOver = bytes_decompressed % 2;
            if(zout_next >= zout + ZRLE_OUTPUT_BUFFER) {
                zout_next = zout;
            }
            DEBUG_VNC_ZLIB("[_handle_zlib_encoded_message] Avail: %zu Consumed: %zu Decomp: %zu AvailOut: %zu Status: %d\n", bytes_available, bytes_consumed, bytes_decompressed, zout + ZRLE_OUTPUT_BUFFER - zout_next, last_status);
        }
    }

    display->area_update_end();

    DEBUG_VNC_ZLIB("[_handle_zlib_encoded_message] done (%zu of %zu)\n", processed, w*h);

    return true;
}
#endif

#ifdef VNC_ZRLE
bool arduinoVNC::_handle_zrle_encoded_message(rfbFramebufferUpdateRectHeader rectheader) {
    uint16_t x = rectheader.r.x;
    uint16_t y = rectheader.r.y;
    uint16_t w = rectheader.r.w;
    uint16_t h = rectheader.r.h;

    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] x: %d y: %d w: %d h: %d\n", x, y, w, h);

    rfbZlibHeader zlh;
    if (!read_from_rfb_server(sock, (char *)&zlh, sz_rfbZlibHeader)) {
        return false;
    }
    uint32_t len = Swap32IfLE(zlh.nBytes);

    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] len: %zu\n", len);

    msg_bytes_remain = len;
    zin_next = zin;
    bytes_available = 0;
    zout_read = zout_next;

    uint16_t rect_x, rect_y, rect_w, rect_h, i = 0, j = 0;
    uint16_t rect_xW, rect_yW;

    uint16_t tile_w = 64, tile_h = 64;
    uint16_t remaining_w, remaining_h;
    size_t tile_size;
    uint16_t *p;

    CARD8 subrect_encoding;

    uint16_t color;
    size_t idx;
    size_t paletteSize = 0;

    rect_w = remaining_w = w;
    rect_h = remaining_h = h;
    rect_x = x;
    rect_y = y;

    size_t runLengthCount = 0;
    uint8_t runLenMinus1;
    uint16_t runLength;

    while (i < rect_h) {
        /* the rect is divided into tiles of width and height 64. Iterate over
        * those */
        /* the last tile in a column could be smaller than 16 */
        if (remaining_h < 64) {
            tile_h = remaining_h;
        }

        /* the last tile in a row could also be smaller */
        if (remaining_w < 64) {
            tile_w = remaining_w;
        } else {
            remaining_w -= 64;
        }

        tile_size = tile_w * tile_h;
        rect_xW = rect_x + j;
        rect_yW = rect_y + i;

        read_from_z(&subrect_encoding, 1);

        if (subrect_encoding == rfbTrleRaw) {
            DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d RAW x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);
            read_from_z((uint8_t *)framebuffer, tile_size * 2);
            display->draw_area(rect_xW, rect_yW, tile_w, tile_h, (uint8_t *)framebuffer);
        } else {
            paletteSize = subrect_encoding & 127;

            read_from_z((uint8_t *)&palette, paletteSize * 2);
 
            if (subrect_encoding == rfbTrleSolid) {
                DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d SOLID x: %d y: %d w: %d h: %d c: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h, palette[0]);
                display->draw_rect(rect_xW, rect_yW, tile_w, tile_h, Swap16IfLE(palette[0]));
            } else if (subrect_encoding <= rfbTrleReusePackedPalette) {
                p = framebuffer;
                uint8_t data = 0;
                if (paletteSize == 2) { // 1-bit
                    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d 1-bit, x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);

                    for (int hidx = 0; hidx < tile_h; ++hidx) {
                        for (idx = 0; idx < tile_w; ++idx) {
                            if ((idx & 0b111) == 0) { // new byte
                                read_from_z(&data, 1);
                            } else {
                                data <<= 1;
                            }
                            *p++ = palette[(data >> 7) & 127];
                        }
                    }
                } else if (paletteSize <= 4) { // 3-4 palettes, 2-bit
                    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d 2-bit, x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);

                    for (int hidx = 0; hidx < tile_h; ++hidx) {
                        for (idx = 0; idx < tile_w; ++idx) {
                            if ((idx & 0b11) == 0) { // new byte
                                read_from_z(&data, 1);
                            } else {
                                data <<= 2;
                            }
                            *p++ = palette[(data >> 6) & 127];
                        }
                    }
                } else if (paletteSize <= 16) { // 5-16 palettes, 4-bit
                    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d 4-bit, x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);

                    for (int hidx = 0; hidx < tile_h; ++hidx) {
                        for (idx = 0; idx < tile_w; ++idx) {
                            if ((idx & 1) == 0) { // new byte
                                read_from_z(&data, 1);
                            } else {
                                data <<= 4;
                            }
                            *p++ = palette[(data >> 4) & 127];
                        }
                    }
                } else { // > 16 palettes, 8-bit
                    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d 8-bit, x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);

                    for (idx = 0; idx < tile_size; ++idx) {
                        read_from_z(&data, 1);
                        *p++ = palette[data & 127];
                    }
                }

                display->draw_area(rect_xW, rect_yW, tile_w, tile_h, (uint8_t *)framebuffer);
            } else if (subrect_encoding == rfbTrlePlainRLE) {
                DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d Plain RLE x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);
                p = framebuffer;
                runLengthCount = 0;
                while (runLengthCount < tile_size) {
                    read_from_z((uint8_t *)&color, 2);

                    runLength = 1;
                    do {
                        read_from_z(&runLenMinus1, 1);

                        runLength += runLenMinus1;
                    } while (runLenMinus1 == 255);

                    runLengthCount += runLength;
                    if (runLengthCount > tile_size) {
                        DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d Plain RLE runLengthCount(%d) > tile_size(%d)\n", subrect_encoding, runLengthCount, tile_size);
                    } else {
                        while (runLength--) {
                            *p++ = color;
                        }
                    }
                }

                display->draw_area(rect_xW, rect_yW, tile_w, tile_h, (uint8_t *)framebuffer);
            } else { // Palette RLE
                DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d Palette RLE x: %d y: %d w: %d h: %d\n", subrect_encoding, rect_xW, rect_yW, tile_w, tile_h);
                p = framebuffer;
                runLengthCount = 0;
                while (runLengthCount < tile_size) {
                    read_from_z((uint8_t *)&idx, 1);

                    runLength = 1;
                    if ((idx & 128) != 0) {
                        do {
                            read_from_z(&runLenMinus1, 1);

                            runLength += runLenMinus1;
                        } while (runLenMinus1 == 255);
                    }

                    color = palette[idx & 127];

                    runLengthCount += runLength;
                    // DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] Palette RLE idx: %d, runLength: %d, runLengthCount: %d.\n", idx, runLength, runLengthCount);
                    if (runLengthCount > tile_size) {
                        DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] %d Palette RLE runLengthCount(%d) > tile_size(%d)\n", subrect_encoding, runLengthCount, tile_size);
                    } else {
                        while (runLength--) {
                            *p++ = color;
                        }
                    }
                }

                display->draw_area(rect_xW, rect_yW, tile_w, tile_h, (uint8_t *)framebuffer);
            }
        }

        // next tile
        j += 64;
        if (j >= rect_w) {
            j = 0;
            remaining_w = w;
            tile_w = 64; /* reset for next row */
            i += 64;

            if (remaining_h >= 64) {
                remaining_h -= 64;
            } else {
                break;
            }
        }

        // DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] loop i: %d, j: %d\n", i, j);
    }

    // We need to consume the remaining data to make sure the TCP buffer is 
    // at correct position and tinfl_decompress is in the right state
    while(msg_bytes_remain > 0) {
        DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] reading left-over bytes from message: %d\n", msg_bytes_remain);
        uint8_t skipped;
        read_from_z(&skipped, 1);
    }
    DEBUG_VNC_ZRLE("[_handle_zrle_encoded_message] ------------------------ Fin ------------------------\n");
    return true;
}
#endif // #ifdef VNC_ZRLE

bool arduinoVNC::_handle_cursor_pos_message(rfbFramebufferUpdateRectHeader rectheader) {
    DEBUG_VNC_RICH_CURSOR("[HandleCursorPos] x: %d y: %d w: %d h: %d\n", rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);
    return true;
}

#ifdef VNC_RICH_CURSOR
bool arduinoVNC::_handle_richcursor_message(rfbFramebufferUpdateRectHeader rectheader) {
//todo handle Cursor
    DEBUG_VNC_RICH_CURSOR("[HandleRichCursor] x: %d y: %d w: %d h: %d\n", rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);

    CARD16 width = rectheader.r.w;
    CARD16 height = rectheader.r.h;

    size_t bytesPerRow, bytesMaskData;

    bytesPerRow = (width + 7) / 8;
    bytesMaskData = bytesPerRow * height;

//FreeCursors(True);

    if(width * height == 0) {
        return true;
    }

    /* Read cursor pixel data. */
    if(richCursorData) {
        freeSec(richCursorData);
    }
    richCursorData = (uint8_t *) malloc(width * height * (opt.client.bpp / 8));
    if(richCursorData == NULL)
    return false;

    if(!read_from_rfb_server(sock, (char *) richCursorData, width * height * (opt.client.bpp / 8))) {
        freeSec(richCursorData);
        return false;
    }

    /* Read and decode mask data. */
    uint8_t * buf = (uint8_t *) malloc(bytesMaskData);
    if(buf == NULL) {
        freeSec(richCursorData);
        return false;
    }

    if(!read_from_rfb_server(sock, (char *)buf, bytesMaskData)) {
        freeSec(richCursorData);
        freeSec(buf);
        return false;
    }

    if(richCursorMask) {
        freeSec(richCursorMask);
    }
    richCursorMask = (uint8_t *) malloc(width * height);
    if(richCursorMask == NULL) {
        freeSec(richCursorData);
        freeSec(buf);
        return false;
    }
    int32_t x, y, b;
    uint8_t * ptr = richCursorMask;
    for(y = 0; y < height; y++) {
        for(x = 0; x < width / 8; x++) {
            for(b = 7; b >= 0; b--) {
                *ptr++ = buf[y * bytesPerRow + x] >> b & 1;
            }
        }
        for(b = 7; b > 7 - width % 8; b--) {
            *ptr++ = buf[y * bytesPerRow + x] >> b & 1;
        }
    }

    free(buf);

//todo Render Cursor

    return true;
}
#endif

bool arduinoVNC::_handle_server_continuous_updates_message(rfbFramebufferUpdateRectHeader rectheader) {
    DEBUG_VNC("[rfbEncodingContinuousUpdates] x: %d y: %d w: %d h: %d\n", rectheader.r.x, rectheader.r.y, rectheader.r.w, rectheader.r.h);
    return true;
}

//#############################################################################################
//                                      Encryption
//#############################################################################################

void arduinoVNC::vncRandomBytes(unsigned char *bytes) {
#ifdef ESP8266
    srand(RANDOM_REG32);
#else
    // todo find better seed
    srand(millis());
#endif
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

#ifdef VNC_RICH_CURSOR
void arduinoVNC::SoftCursorMove(int x, int y) {
//todo handle Cursor
}
#endif
