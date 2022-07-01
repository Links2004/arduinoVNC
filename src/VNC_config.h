/*
 * @file VNC_config.h
 * @date 07.01.2016
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
 *
 */

#ifndef VNC_CONFIG_H_
#define VNC_CONFIG_H_

/// Display
#define VNC_ILI9341
#define VNC_ST7789

/// TCP layer
#define USE_ARDUINO_TCP
#define VNC_TCP_TIMEOUT 5000
// comment below for disable TCP buffer
#define TCP_BUFFER_SIZE 32

/// VNC Encodes
#define VNC_RRE
#define VNC_CORRE
#define VNC_HEXTILE

// not implemented
//#define VNC_TIGHT
//#define VNC_ZLIB
//#define VNC_RICH_CURSOR
//#define VNC_SEC_TYPE_TIGHT

/// Testing
#define FPS_BENCHMARK
//#define FPS_BENCHMARK_FULL

#define MAXFPS 25

//#define SLOW_LOOP 250

/// Memory Options
#define VNC_RAW_BUFFER 512
#define FB_SIZE (16 * 16)

/// debugging
#if defined(ESP32)
#define DEBUG_VNC(...) Serial.printf( __VA_ARGS__ )
#elif defined(RTL8722DM)
#define DEBUG_VNC(...)
#else

#ifdef DEBUG_ESP_PORT
#define DEBUG_VNC(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#elif defined(ESP8266)
#define DEBUG_VNC(...) Serial.printf( __VA_ARGS__ )
#else
#define DEBUG_VNC(...) os_printf( __VA_ARGS__ )
#endif

#endif

#define DEBUG_VNC_RAW(...)
#define DEBUG_VNC_HEXTILE(...)
#define DEBUG_VNC_RICH_CURSOR(...)

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

#endif /* VNC_CONFIG_H_ */
