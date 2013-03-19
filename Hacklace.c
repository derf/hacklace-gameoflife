/*
 * Hacklace.c
 *
 */ 

/**********************************************************************************

Title:				Hacklace - A necklace for hackers

Hardware:			Hacklace-Board with ATtiny4313 running at 4 MHz and a
					5 x 7 dot matrix display.
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
#include <avr/sleep.h>
#include <util/delay.h>
#include "config.h"
#include "dot_matrix.h"


/*********
* fuses *
*********/

FUSES =
{
	.low = 0xE2,
	.high = 0xDF,
	.extended = 0xFF,
};


/********************
 * global variables *
 ********************/

uint8_t scroll_speed = 14;					// scrolling speed (0 = fastest)
volatile uint8_t button = PB_ACK;			// button event
volatile uint8_t scroll_enabled = 0;
//uint8_t* msg_ptr = (uint8_t*) messages;		// pointer to next message in EEPROM
uint8_t* msg_ptr;							// pointer to next message in EEPROM
uint8_t* ee_write_ptr = (uint8_t*) messages;


/*************
 * constants *
 *************/
// serial input states
#define IDLE			0
#define AUTH			1		// first authentication byte received
#define RESET			2
#define DISP_SET_MODE	3
#define DISP_CHAR		4
#define EE_NORMAL		5
#define EE_SPECIAL_CHAR	6
#define EE_HEX_CODE		7

#define AUTH1_CHAR		'H'
#define EE_AUTH2_CHAR	'L'		// authentication for entering EEPROM mode
#define DISP_AUTH2_CHAR	'D'		// authentication for entering DISPLAY mode


/**********
 * macros *
 **********/

// Usage: b=swap(a) or b=swap(b)
#define swap(x)													\
	({															\
		unsigned char __x__ = (unsigned char) x;				\
		asm volatile ("swap %0" : "=r" (__x__) : "0" (__x__));	\
		__x__;													\
	})


/*************
 * functions *
 *************/

/*======================================================================
	Function:		InitHardware
	Input:			none
	Output:			none
	Description:	.
======================================================================*/
void InitHardware(void)
{
	// switch all pins that are connected to the dot matrix to output
	DDRA = DISP_MASK_A;
	DDRB = DISP_MASK_B;
	DDRD = DISP_MASK_D;
	
	// enable pull-ups on all input pins to avoid floating inputs
	PORTA |= ~DISP_MASK_A;
	PORTB |= ~DISP_MASK_B;
	PORTD |= ~DISP_MASK_D;
	
	// timer 0
	TCCR0A = (0<<WGM00);				// timer mode = normal
	TCCR0B = (5<<CS00);					// prescaler = 1:1024
	OCR0A = OCR0A_CYCLE_TIME;
	OCR0B = OCR0B_CYCLE_TIME;
	TIMSK |= (1<<OCIE0B)|(1<<OCIE0A);
	
}


/*======================================================================
	Function:		GoToSleep
	Input:			none
	Output:			none
	Description:	Put the controller into sleep mode and prepare for
					wake-up by a pin change interrupt.
======================================================================*/
void GoToSleep(void)
{
	scroll_enabled = 0;
	dmClearDisplay();
	_delay_ms(1000);
	GIFR = (1<<PCIF2);				// clear interrupt flag
	PCMSK2 = (1<<PCINT17);			// enable pin change interrupt
	GIMSK = (1<<PCIE2);				// enable pin change interrupt
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
	GIMSK = 0;						// disable all external interrupts (including pin change)
	dmWakeUp();
	_delay_ms(500);
	scroll_enabled = 1;
}


/********
 * main *
 ********/

int main(void)
{
	InitHardware();
	dmInit();
	dmWakeUp();
	sei();									// enable interrupts

	GoToSleep();
	button |= PB_ACK;

	while(1)
	{
		if (button == PB_RELEASE) {			// short button press
			button |= PB_ACK;
		}
		
		if (button == PB_LONGPRESS) {		// button pressed for some seconds
			dmClearDisplay();
			GoToSleep();
			button |= PB_ACK;
		}
		
	} // of while(1)
}


/******************************
 * interrupt service routines *
 ******************************/

ISR(TIMER0_COMPA_vect)
// display interrupt
{
	OCR0A += OCR0A_CYCLE_TIME;				// setup next cycle

	dmDisplay();							// show next column on dot matrix display
}


ISR(TIMER0_COMPB_vect)
// system timer interrupt
{
	static uint8_t scroll_timer = 1;
	static uint8_t pb_timer = 0;			// push button timer
	uint8_t temp;
		
	OCR0B += OCR0B_CYCLE_TIME;				// setup next cycle

	if (scroll_timer) {
		scroll_timer--;
	}
	else {
		scroll_timer = scroll_speed;		// restart timer
		if (scroll_enabled) {
			TIMSK &= ~_BV(OCIE0B);
			sei();
			dmScroll();						// do a scrolling step
			cli();
			TIMSK |= _BV(OCIE0B);
		}
	}
	
	// push button sampling
	temp = ~PB_PIN;							// sample push button
	temp &= PB_MASK;						// extract push button state
	if (temp == 0) {						// --- button not pressed ---
		if (button & PB_PRESS) {			// former state = pressed?
			button &= ~(PB_PRESS | PB_ACK);	// -> issue release event
		}
	}
	else {									// --- button pressed ---
		if ((button & PB_PRESS) == 0) {		// former state = button released?
			button = PB_PRESS;				// issue new press event
			pb_timer = PB_LONGPRESS_DELAY;	// start push button timer
		}
		else {
			if (button == PB_PRESS) {		// holding key pressed
				if (pb_timer == 0) {		// if push button timer has elapsed
					button = PB_LONGPRESS;	// issue long event
				}
				else {
					pb_timer--;
				}
			}			
		}		
	}
	
}


ISR(PCINT_D_vect)
// pin change interrupt (for wake-up)
{
}
