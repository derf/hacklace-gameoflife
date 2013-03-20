# Conway's Game of Life for the Hacklace

This is a modified hacklace firmware playing conway's game of life.
When turnd on, it will populate the display with random values and then start
applying the rules, displaying the result after each step.

If the world becomes a still life, it will be replaced by a random new one
after 32 cycles.
Additionally, any world will be replaced by a new one after 2048 cycles. This
makes sure that no oscillators can persist.

Note that the game is constrained to the 5x7 field of the display. Bot the left
and right edges and the top and bottom edges are connected in the game field.

The following instructions are part of the original readme:

Visit http://www.hacklace.org for more information and build instructions.

## Building
To build the project you can use AVR Studio 6 or the integrated Makefile.

**Important:** This code comes with a fixed iotn4313 header file.
If you get build errors, use this one.
For example copy it to /usr/avr/include/avr/iotn4313.h

Flash
-----
You can flash the complete firmware to your hacklace using the target flashall.
