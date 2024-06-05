VNC Client for Arduino [![CI](https://github.com/Links2004/arduinoVNC/actions/workflows/main.yml/badge.svg)](https://github.com/Links2004/arduinoVNC/actions/workflows/main.yml)
===========================================

a VNC Client for Arduino based on rfbproto.

Video:

[![VNC Client on ESP8266 + ILI9341](https://img.youtube.com/vi/MA47npOtApc/0.jpg)](https://www.youtube.com/watch?v=MA47npOtApc)
[![VNC Client on ESP8266 + ILI9341 + Touch](https://img.youtube.com/vi/8Ix-HomwQvw/0.jpg)](https://www.youtube.com/watch?v=8Ix-HomwQvw)

##### Supported features #####
 - Bell
 - CutText (clipboard)
 
##### Supported encodings #####
 - RAW
 - RRE
 - CORRE
 - HEXTILE
 - COPYRECT (if display support it)
 - ZLIB
 - ZRLE
  
##### Not supported encodings #####
 - TIGHT
    
##### Supported Hardware #####
 - ESP8266 [Arduino for ESP8266](https://github.com/esp8266/Arduino)
 - ESP32 [Arduino for ESP32](https://github.com/espressif/arduino-esp32)
 may run on Arduino DUE too.

##### Supported Displays #####
 - ILI9341 [library](https://github.com/Links2004/Adafruit_ILI9341)
 - ST7789 [library](https://github.com/Bodmer/TFT_eSPI)
 - ST7796 [library](https://github.com/lovyan03/LovyanGFX)
 
more possible using ```VNCdisplay``` Interface
 
### Issues ###
Submit issues to: https://github.com/Links2004/arduinoVNC/issues

### License and credits ###

The library is licensed under [GPLv2](https://github.com/Links2004/arduinoVNC/blob/master/LICENSE)

D3DES by Richard Outerbridge (public domain)

VNC code base (GPLv2)
Thanks to all that worked on the original VNC implementation
```
Copyright (C) 2009-2010, 2012 D. R. Commander. All Rights Reserved.
Copyright (C) 2004-2008 Sun Microsystems, Inc. All Rights Reserved.
Copyright (C) 2004 Landmark Graphics Corporation. All Rights Reserved.
Copyright (C) 2000-2006 Constantin Kaplinsky. All Rights Reserved.
Copyright (C) 2000 Tridia Corporation. All Rights Reserved.
Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
```


