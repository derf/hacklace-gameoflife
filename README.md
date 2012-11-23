Hacklace Firmware
=================

Visit http://www.hacklace.org for more information and build instructions.

Building
--------
To build the project you can use AVR Studio 6 or the integrated Makefile.

**Important:** This code comes with a fixed iotn4313 header file.
If you get build errors, use this one.
For example copy it to /usr/avr/include/avr/iotn4313.h

Flash
-----
You can flash the complete firmware to your hacklace (with eeprom) using the target flashall.
