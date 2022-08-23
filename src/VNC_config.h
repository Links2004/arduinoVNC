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

/// TCP layer
#define USE_ARDUINO_TCP
#define VNC_TCP_TIMEOUT 5000
// comment below for disable TCP buffer
#if defined(ESP32)
#define TCP_BUFFER_SIZE 1600
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#define TCP_BUFFER_SIZE 1600
#elif defined(RTL8722DM)
#define TCP_BUFFER_SIZE 1600
#else
#define TCP_BUFFER_SIZE 32
#endif

/// VNC Encodes
#define VNC_RRE // RFC6143
#define VNC_CORRE
#define VNC_HEXTILE // RFC6143
// #define VNC_ZLIB
// #define VNC_ZLIBHEX
// #define VNC_TRLE // RFC6143
// #define VNC_ZRLE // RFC6143
/// not implemented
// #define VNC_TIGHT

// zlib related
#define VNC_COMPRESS_LEVEL 9

/// VNC Pseudo-encodes
#define SET_DESKTOP_SIZE // Set resolution according to display resolution
/// not implemented
// #define VNC_RICH_CURSOR

/// authenticate method
/// not implemented
// #define VNC_SEC_TYPE_TIGHT

/// Testing
#define FPS_BENCHMARK
// #define FPS_BENCHMARK_FULL

#define MAXFPS 25

// #define SLOW_LOOP 250

/// Memory Options
#define VNC_RAW_BUFFER (320 * 2) // RAW screen width
#if defined(VNC_ZRLE)
#define FB_SIZE (64 * 64)
#else
#define FB_SIZE (16 * 16)
#endif

/// debugging
#if defined(ESP32)
#define DEBUG_VNC(...) Serial.printf(__VA_ARGS__)
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#define DEBUG_VNC(...) Serial.printf(__VA_ARGS__)
#elif defined(RTL8722DM)
#define DEBUG_VNC(...) printf(__VA_ARGS__)
#else

#ifdef DEBUG_ESP_PORT
#define DEBUG_VNC(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#elif defined(ESP8266)
#define DEBUG_VNC(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_VNC(...) os_printf(__VA_ARGS__)
#endif

#endif

#define DEBUG_VNC_HANDLE(...)
#define DEBUG_VNC_RAW(...)
#define DEBUG_VNC_HEXTILE(...)
#define DEBUG_VNC_ZLIB(...)
#define DEBUG_VNC_TRLE(...)
#define DEBUG_VNC_ZRLE(...)
#define DEBUG_VNC_RICH_CURSOR(...)

#ifndef DEBUG_VNC
#define DEBUG_VNC(...)
#endif

#ifndef DEBUG_VNC_HANDLE
#define DEBUG_VNC_HANDLE(...) DEBUG_VNC(__VA_ARGS__)
#endif

#ifndef DEBUG_VNC_RAW
#define DEBUG_VNC_RAW(...) DEBUG_VNC(__VA_ARGS__)
#endif

#ifndef DEBUG_VNC_HEXTILE
#define DEBUG_VNC_HEXTILE(...) DEBUG_VNC(__VA_ARGS__)
#endif

#ifndef DEBUG_VNC_ZLIB
#define DEBUG_VNC_ZLIB(...) DEBUG_VNC(__VA_ARGS__)
#endif

#ifndef DEBUG_VNC_TRLE
#define DEBUG_VNC_TRLE(...) DEBUG_VNC(__VA_ARGS__)
#endif

#ifndef DEBUG_VNC_ZRLE
#define DEBUG_VNC_ZRLE(...) DEBUG_VNC(__VA_ARGS__)
#endif

#ifndef DEBUG_VNC_RICH_CURSOR
#define DEBUG_VNC_RICH_CURSOR(...) DEBUG_VNC(__VA_ARGS__)
#endif

#endif /* VNC_CONFIG_H_ */
