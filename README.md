VNC Client for Arduino
===========================================

a VNC Client for Arduino based on rfbproto.

Example:
<iframe width="854" height="480" src="https://www.youtube.com/embed/MA47npOtApc" frameborder="0" allowfullscreen></iframe>

##### Supported features #####
 - Bell
 - CutText (clipboard)
 
##### Supported encodings #####
 - RAW
 - RRE
 - CORRE
 - HEXTILE
 - COPYRECT (if display support it)
  
##### Not supported encodings #####
 - TIGHT
 - ZLIB
    
##### Supported Hardware #####
 - ESP8266 [Arduino for ESP8266](https://github.com/Links2004/Arduino)
 
 may run on Arduino DUE too.

##### Supported Displays #####
 - ILI9341
 
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


