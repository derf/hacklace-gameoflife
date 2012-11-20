/*
 * config.h
 *
 */ 

/**********************************************************************************

Description:		Configuration file for Hacklace
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


#ifndef CONFIG_H_
#define CONFIG_H_


/*************
 * constants *
 *************/

// processor clock frequency
#ifndef F_CPU
#define F_CPU		4000000
#endif

// timing
#define COLUMN_FREQ			1000		// display column frequency [Hz]
#define SYS_TIMER_FREQ		100			// system timer frequency [Hz]
#define OCR0A_CYCLE_TIME	(uint8_t)(F_CPU / 1024.0 / COLUMN_FREQ + 0.5);
#define OCR0B_CYCLE_TIME	(uint8_t)(F_CPU / 1024.0 / SYS_TIMER_FREQ + 0.5);

// push button
#define PB_PORT				PORTD
#define PB_PIN				PIND
#define PB_BIT				6			// bit number of the pin where the push button is connected
#define PB_LONGPRESS_DELAY	100			// number of system timer cycles after which a longpress event is issued

// bit masks for push button events (do not change)
#define PB_PRESS			(1<<0)					// 1 = pressed, 0 = released
#define PB_RELEASE			(0<<0)
#define PB_LONG				(1<<1)					// 1 = long press, 0 = short press
#define PB_ACK				(1<<7)					// 1 = key event has been processed
#define PB_LONGPRESS		(PB_PRESS|PB_LONG)
#define PB_MASK				(1<<PB_BIT)				// mask to extract button state

// messages in EEPROM
#define MSG_SIZE	256			// number of EEPROM bytes reserved for messages

// default message data
// A message is either a text or an animation to be displayed on the dot matrix.
// Note: Between each two characters of a string there will be a space of 1 column.
//		0x20 = normal space (3+1 columns)
//		0x7F = short space (0+1 column)
//		0x9D = long space (5+1 columns), may be used as the last frame of an animation
const uint8_t messages[MSG_SIZE] EEMEM = {
	0x54, 'H', 'a', 'c', 'k', 'l', 'a', 'c', 'e', ' ', '^', 'P', 0x00, 
	0x44, ' ', 'n', 'u', 'r', ' ', '1', '0', '^', 'A', 0x9D, 0x00,
	0x64, ' ', 'K', 'a', 'u', 'f', ' ', 'm', 'i', 'c', 'h', 0x7F, '!', '!', '!', 0x9D, 0x00,
	0x65, ' ', 'I', ' ', '^', 'R', ' ', 'R', 'a', 'u', 'm', 'Z', 'e', 'i', 't', 'L', 'a', 'b', 'o', 'r', 0x9D, 0x00,
	0xC4, 0x8B, ' ', 0x8C, ' ', 0x8E, ' ', 0x8D, 0x00,							// Monster
	0x44, ' ', '^', 'm', ' ', '+', ' ', '^', 'n', ' ', '=', ' ', '^', 'R', 0x00,
	0x0B, 0xA3, ' ', 0xA5, ' ', 0xA6, ' ', 0xA0, ' ', 0x00,						// break-dance
	0x04, ' ', '^', 'S', '^', 'S', '^', 'S', 0x9D, 0x00,						// turn left
	0x04, ' ', 0x94, 0x95, 0x95, ' ',  0x94, ' ', 0x95, 0x7F, 0x94, 0x9D, 0x00,	// music
	0x95, ' ', '|', ' ', 0x00,			// scan
	0x6C, '~', 'A', 0x00,				// arrow
	0x0D, '~', 'B', 0x00,				// fire
	0x4B, '~', 'C', 0x9D, 0x00,			// bounce
	0x44, 0x9D, 'B', 'e', 'r', 'g', 'e', ' ', '~', 'D', 0x00,
	0x4A, '~', 'E', 0x00,				// snow
	0x3D, '~', 'F', 0x9D, 0x00,			// tunnel
	0x5A, '~', 'G', 0x00,				// wink
	0x34, '~', 'H', 0x00,				// ecg
	0x0E, '~', 'I', 0x00,				// crazy checkers
	0x4A, 0x91, ' ', 0x91, 0x9D, 0x00,	// heartbeat
	0x49, '~', 'J', 0x00,				// tetris
	0x5B, '~', 'K', 0x00,				// glider
	0x8B, '~', 'L', 0x9D, 0x00,			// hop
	0x6B, '~', 'M', 0x00,				// pong
	0x38, ' ', 0x9D, '~', 'N', 0x00,	// house
	0x6B, '~', 'O', 0x9D, 0x00,			// rocket
	0x0B, '~', 'P', 0x00,				// rainy
	0x5B, '3', '3', 0x7F, ' ', '2', '2', 0x7F, ' ', 0x7F, '1', 0x7F, '1', '~', 'Q', 0x9D, 0x00,	// explode
	0x6C, '~', 'R', 0x00,				// droplet
	0x0E, '~', 'S', 0x00,				// psycho
	0x7D, '~', 'T', 0x9D, 0x00,			// TV off
//	0x94, '~', 'U', 0x00,				// lady
//	0x94, '~', 'V', 0x00,				// boobs
//	0x94, '~', 'W', 0x00,				// thighs
	0x00
};

// speed and delay conversion
// Convert speed / delay parameters from mode byte (range 0..7) to actual speed / delay values.
const uint8_t dly_conv[] PROGMEM = {0, 1, 2, 3, 5, 8, 13, 21};
const uint8_t spd_conv[] PROGMEM = {50, 30, 18, 11, 7, 5, 3, 2};


// debug helpers

// select if the messages are to be read from flash (pgm_read_byte) or from EEPROM (eeprom_read_byte)
#define msg_read_byte	eeprom_read_byte

// serial input
//#define ECHO_RXD	// if defined, each received character will be displayed (using the font)


#endif /* CONFIG_H_ */