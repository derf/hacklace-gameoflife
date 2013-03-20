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

#endif /* CONFIG_H_ */
