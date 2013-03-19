/*
 * dot_matrix.c
 *
 */ 

/**********************************************************************************

Description:		Functions to access a dot matrix display.
Author:				Frank Andre
License:			This software is distributed under the creative commons license
					CC-BY-NC-SA.
Disclaimer:			This software is provided by the copyright holder "as is" and any 
					express or implied warranties, including, but not limited to, the 
					implied warranties of merchantability and fitness for a particular 
					purpose are disclaimed. In no event shall the copyright owner or 
					contributors be liable for any direct, indirect, incidental, 
					special, exemplary, or consequential damages (including, but not 
					limited to, procurement of substitute goods or services; loss of 
					use, data, or profits; or business interruption) however caused 
					and on any theory of liability, whether in contract, strict 
					liability, or tort (including negligence or otherwise) arising 
					in any way out of the use of this software, even if advised of 
					the possibility of such damage.
					
**********************************************************************************/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include "dot_matrix.h"


/********************
 * global variables *
 ********************/

#ifdef DISP_UPDOWN
	const uint8_t col_port[] PROGMEM = {C5_PORT, C4_PORT, C3_PORT, C2_PORT, C1_PORT};
	const uint8_t col_bit[]  PROGMEM = {(1<<C5), (1<<C4), (1<<C3), (1<<C2), (1<<C1)};
	const uint8_t row_port[] PROGMEM = {R7_PORT, R6_PORT, R5_PORT, R4_PORT, R3_PORT, R2_PORT, R1_PORT};
	const uint8_t row_bit[]  PROGMEM = {(1<<R7), (1<<R6), (1<<R5), (1<<R4), (1<<R3), (1<<R2), (1<<R1)};
#else
	const uint8_t col_port[] PROGMEM = {C1_PORT, C2_PORT, C3_PORT, C4_PORT, C5_PORT};
	const uint8_t col_bit[]  PROGMEM = {(1<<C1), (1<<C2), (1<<C3), (1<<C4), (1<<C5)};
	const uint8_t row_port[] PROGMEM = {R1_PORT, R2_PORT, R3_PORT, R4_PORT, R5_PORT, R6_PORT, R7_PORT};
	const uint8_t row_bit[]  PROGMEM = {(1<<R1), (1<<R2), (1<<R3), (1<<R4), (1<<R5), (1<<R6), (1<<R7)};
#endif

// The display memory contains all the data to be displayed. Of the display memory
// only a small window, whose size matches the dot matrix display, is actually displayed.
typedef struct {
	uint8_t memory[DISP_MAX];	// display memory (every byte encodes a column)
	uint8_t base;				// index of column 1 of currently displayed window
	uint8_t curr_col;			// index of currently displayed column within window
	uint8_t scroll_mode;		// lower nibble = increment of display base for each scrolling step (0 = off)
	// bit 4 = direction (0 = forward, 1 = backward)
	// bit 5 = bidirectional (0 = off, 1 = on)
	uint8_t cursor;				// index of first free byte after current display content (0 = empty display)
	uint8_t scroll_delay;		// delay (number of scrolling steps) before scrolling cycle restarts
	uint8_t delay_counter;		// counter for scroll delays (counting down to zero)
} display_t;

display_t display;

/**********
 * makros *
 **********/

#define BIT_IS_ON	(pattern & 1)
#define NEXT_BIT	pattern >>= 1
#define COL			col

// Usage: swap(b)
#define swap(x) 											\
	({														\
		asm volatile ("swap %0" : "=r" (x) : "0" (x));		\
	})


/*************
 * functions *
 *************/

/*======================================================================
	Function:		dmInit
	Input:			none
	Output:			none
	Description:	Initialize the hardware.
======================================================================*/
void dmInit(void)
{
	dmClearDisplay();
	display.scroll_mode = 0;
	display.scroll_delay = 0;
}


/*======================================================================
	Function:		dmSetOutputs
	Input:			column number
					bit pattern
	Output:			none
	Description:	Set row and column outputs so that the leds of the specified 
					column represent the bit pattern (1 = led on).
======================================================================*/
inline void dmSetOutputs(uint8_t col, uint8_t pattern)
{
	uint8_t i;
	uint8_t p[3];

	p[0] = 0;  p[1] = 0;  p[2] = 0;
	for (i = 0; i < DISP_ROWS; i++) {
		if (BIT_IS_ON) {
			p[ pgm_read_byte(&row_port[i]) ] |= pgm_read_byte(&row_bit[i]);		// set bit
		}
		NEXT_BIT;
	}		
	for (i = 0; i < DISP_COLUMNS; i++) {
		if (i != COL) {					// note: COL is a macro
			p[ pgm_read_byte(&col_port[i]) ] |= pgm_read_byte(&col_bit[i]);		// set bit
		}
	}
	#if DISP_TYPE == 1					// if we use a display with common column anode
		p[0] ^= DISP_MASK_A;				// -> invert outputs
		p[1] ^= DISP_MASK_B;
		p[2] ^= DISP_MASK_D;
	#endif

	// set outputs
	i = PORTA & ~DISP_MASK_A;
	PORTA = i | p[0];
	i = PORTB & ~DISP_MASK_B;
	PORTB = i | p[1];
	i = PORTD & ~DISP_MASK_D;
	PORTD = i | p[2];
}


/*======================================================================
	Function:		dmDisplay
	Input:			none
	Output:			none
	Description:	Switch to the next display column and display it on the led matrix.
					Call this function periodically, e. g. within an interrupt routine.
======================================================================*/
void dmDisplay(void)
{
	if (display.curr_col >= DISP_COLUMNS) {
		display.curr_col = 0;
	}
	else {
		display.curr_col++;
	}
	dmSetOutputs(display.curr_col, display.memory[display.base + display.curr_col]);
}


/*======================================================================
	Function:		dmScroll
	Input:			none
	Output:			status
	Description:	Scroll display by one step. Returns 1 if end of scrolling range has been reached.
					Call this function periodically, e. g. within an interrupt routine.
======================================================================*/
uint8_t dmScroll(void)
{
	uint8_t newmem[DISP_COLUMNS];
	uint8_t x, y;
	uint8_t l, r, t, b;
	uint8_t live_neighbours;

	static uint8_t samecnt = 0;
	uint8_t equal_cols = 0;

	for (x = 0; x < DISP_COLUMNS; x++) {

		l = (x == 0) ? (DISP_COLUMNS - 1) : (x - 1);
		r = (x == (DISP_COLUMNS - 1)) ? 0 : (x + 1);

		newmem[x] = 0;

		for (y = 0; y < DISP_ROWS; y++) {

			t = (y == 0) ? (DISP_ROWS - 1) : (y - 1);
			b = (y == (DISP_ROWS - 1)) ? 0 : (y + 1);

			live_neighbours =
				((display.memory[l] & _BV(t)) > 0) +
				((display.memory[l] & _BV(y)) > 0) +
				((display.memory[l] & _BV(b)) > 0) +
				((display.memory[x] & _BV(t)) > 0) +
				((display.memory[x] & _BV(b)) > 0) +
				((display.memory[r] & _BV(t)) > 0) +
				((display.memory[r] & _BV(y)) > 0) +
				((display.memory[r] & _BV(b)) > 0);

			if (((live_neighbours == 2) && (display.memory[x] & _BV(y)))
					|| (live_neighbours == 3))
				newmem[x] |= _BV(y);

		}
	}

	for (x = 0; x < DISP_COLUMNS; x++) {
		if (display.memory[x] == newmem[x])
			equal_cols++;
		display.memory[x] = newmem[x];
	}
	if (equal_cols == DISP_COLUMNS) {
		if (++samecnt == 12) {
			samecnt = 0;
			dmWakeUp();
		}
	}
	return 0;
}

/*======================================================================
	Function:		dmClearDisplay
	Input:			none
	Output:			none
	Description:	Set display cursor to begin of display memory and 
					clear visible part of display memory.
======================================================================*/
void dmClearDisplay(void)
{
	uint8_t i;

	display.base  = 0;
	display.cursor = 0;
	for (i = 0; i < DISP_COLUMNS; i++) {
		display.memory[i] = 0;
	}
}


/*======================================================================
	Function:		dmPrintByte
	Input:			byte
	Output:			none
	Description:	Write byte directly to the display memory.
======================================================================*/
void dmPrintByte(uint8_t byt)
{
	uint8_t pos;

	pos = display.cursor;
	if (pos < DISP_MAX) { 
		display.memory[pos] = byt;
		pos++;
		display.cursor = pos;
	}
}

void dmWakeUp()
{
	uint8_t i;

	display.base  = 0;
	display.cursor = 0;

	for (i = 0; i < DISP_COLUMNS; i++) {
		display.memory[i] = rand() % 0x80;
	}
}
