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
#include "dot_matrix.h"
#include "Font_5x7_extended.h"


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
	uint8_t temp, mode;

	mode = display.scroll_mode;
	temp = mode & 0x0F;										// extract increment
	if (mode & 0x10)	{ temp = display.base - temp; }		// scrolling backward
															// We use a dirty trick here:
															// Temp may underflow at left end of display memory.
	else				{ temp = display.base + temp; }		// scrolling forward

	if ((temp + DISP_COLUMNS) > display.cursor ) {			// end of scrolling range reached?
															// Note: As temp is allowed to underflow, this is 
															// true at both ends of the display memory.
		if (display.delay_counter) {
			display.delay_counter--;
		}
		else {
			display.delay_counter = display.scroll_delay;					// reload delay counter
			if (mode & 0x20)		{ display.scroll_mode = mode ^ 0x10; }	// reverse direction
			else if (mode &0x10)	{ display.base = display.cursor - DISP_COLUMNS; }	// restart from right end
			else					{ display.base = 0; }					// restart from left end
		}
		return (1);
	}
	else {
		display.base = temp;
		return (0);
	}
}


/*======================================================================
	Function:		dmSetScrolling
	Input:			increment (range 0..15)
					direction (0 = forward, 1 = backward, 2 = bidirectional)
					delay     (range 0..255)
	Output:			none
	Description:	Set scrolling increment and direction.
					Delay sets number of scrolling steps to wait at end of scrolling range.
======================================================================*/
void dmSetScrolling(uint8_t inc, uint8_t dir, uint8_t delay)
{
	inc &= 0x0F;		// limit range
	dir &= 0x03;		// limit range
	if (dir != BACKWARD)	{ display.delay_counter = delay; }
	else					{ display.delay_counter = 0; }
	swap(dir);
	display.scroll_mode   = inc | dir;
	display.scroll_delay  = delay;
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
	Function:		dmDisplayImage
	Input:			pointer to graphics data in flash memory
	Output:			none
	Description:	Copy flash contents to display memory at current cursor position 
					until the end-of-data marker (0xFF) is reached.
======================================================================*/
void dmDisplayImage(const uint8_t* image)
{
	uint8_t img_data, pos;

	pos = display.cursor;
	while(pos < DISP_MAX) {
		img_data = pgm_read_byte(image++);	// read byte from flash
		if (img_data == 0xFF) { break; }	// stop if end-of-data has been reached
		display.memory[pos] = img_data;
		pos++;
	}
	display.cursor = pos;
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


/*======================================================================
	Function:		dmPrintChar
	Input:			character code
	Output:			none
	Description:	Print character to display memory at current display cursor.
======================================================================*/
void dmPrintChar(uint8_t ch)
{
	uint8_t  i, pos, char_data;
	uint16_t fnt;			// pointer into character font

	// mapping of german special characters
	if (ch == 223) { ch = 138; }		// 'ß'
	if (ch == 196) { ch = 133; }		// 'Ä'
	if (ch == 214) { ch = 135; }		// 'Ö'
	if (ch == 220) { ch = 137; }		// 'Ü'
	if (ch == 228) { ch = 132; }		// 'ä'
	if (ch == 246) { ch = 134; }		// 'ö'
	if (ch == 252) { ch = 136; }		// 'ü'
	ch -= 32;
	if (ch > (sizeof(font)/CHAR_WIDTH)) { return; }
		
	fnt = ch;
	fnt = (fnt << 2) + fnt + (uint16_t) &font[0];		// fnt = &font + 5 * ch
	
	pos = display.cursor;
	for (i = 0; i < CHAR_WIDTH; i++) {
		char_data = pgm_read_byte(fnt++);	// read byte from font
		if (char_data & 0x80) { break; }	// stop if MSB is set (proportional character width)
		if (pos < DISP_MAX) {
			display.memory[pos] = char_data;
			pos++;
		}		
	}
	display.cursor = pos;
}


/*======================================================================
	Function:		dmPrintString
	Input:			pointer to zero terminated string in flash memory
	Output:			none
	Description:	Print string to display memory using the character font.
					The character '^' is used to shift the character code of
					the next character by 63 so that e. g. '^A' becomes char(128).
					To enter a '^' character simply double it: '^^'
======================================================================*/

// This function was commented out to save flash memory.
// Uncomment it if you want to use it.

/*
void dmPrintString(const char* st)
{
	uint8_t ch;

	ch = pgm_read_byte(st++);
	while (ch) {
		if (ch == '^') {
			ch = pgm_read_byte(st++);
			if (ch != '^') {
				ch += 63;
			}
		}
		dmPrintChar(ch);
		ch = pgm_read_byte(st++);
		if (ch) { dmPrintByte(0); }
	}
}		
*/


