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
#include "animations.h"


/********************
 * global variables *
 ********************/

uint8_t scroll_speed = 8;			// scrolling speed (0 = fastest)
volatile uint8_t button = PB_ACK;	// button event
uint8_t* msg_ptr = messages;		// pointer to next message in EEPROM
uint8_t* ee_write_ptr = messages;


/*************
 * constants *
 *************/
// serial input states
#define IDLE			0
#define AUTH			1		// first authentication byte received
#define NORMAL			2
#define SPECIAL_CHAR	3
#define HEX_CODE		4
#define CRLF			5
#define RESET			6

#define AUTH1_CHAR		'H'
#define AUTH2_CHAR		'L'


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
	
	// serial interface
	// Note: Speed of the serial interface must not be higher than 2400 Baud.
	// This leaves enough time for the EEPROM write operation to complete before 
	// the next byte arrives.
	UBRRL = 103;						// 2400 baud
	UBRRH = 0;
	UCSRB = (1<<RXCIE)|(1<<RXEN);		// enable receiver, enable RX interrupt
	UCSRC = (3<<UCSZ0);					// async USART, 8 data bits, no parity, 1 stop bit
}


/*======================================================================
	Function:		SetMode
	Input:			pointer to message data in EEPROM memory
	Output:			none
	Description:	Read mode byte and set display parameters accordingly.
					The mode byte is interpreted as follows:
					Bit 7:		reverse scrolling direction (0 = no, always scroll forward, 1 = yes, bidirectional scrolling)
					Bit 6..4:	delay between scrolling repetitions (0 = shortest, 7 = longest)
					Bit 3:		scrolling increment (cleared = +1 (for texts), set = +5 (for animations))
					Bit 2..0:	scrolling speed (1 = slowest, 7 = fastest)
======================================================================*/
void SetMode(uint8_t* ee_adr)
{
	uint8_t mode;
	uint8_t inc, dir, dly, spd;

	mode = msg_read_byte(ee_adr);
	if (mode & 0x08)	{ inc = 5; }
		else			{ inc = 1; }
	if (mode & 0x80)	{ dir = BIDIRECTIONAL; }
		else			{ dir = FORWARD; }
	spd = mode & 0x07;
	dly = swap(mode) & 0x07;
	dmSetScrolling(inc, dir, pgm_read_byte(&dly_conv[dly]));
	scroll_speed = pgm_read_byte(&spd_conv[spd]);
}		


/*======================================================================
	Function:		DisplayMessage
	Input:			pointer to zero terminated message data in EEPROM memory
	Output:			pointer to next message
	Description:	Show a message (i. e. text or animation) on the display.

					Escape characters:

					Character '^' is used to access special characters by 
					shifting the character code of the next character by 96 
					so that e. g. '^A' becomes char(161).
					To enter a '^' character simply double it: '^^'

					Character '~' followed by an upper case letter is used
					to insert (animation) data from flash.
					
					The character 0xFF is used to enter direct mode in which 
					the following bytes are directly written to the display 
					memory without being decoded using the character font.
					Direct mode is ended by 0xFF.
======================================================================*/
uint8_t* DisplayMessage(uint8_t* ee_adr)
{
	uint8_t ch;

	SetMode(ee_adr);
	ee_adr++;
	dmClearDisplay();

	ch = msg_read_byte(ee_adr++);
	while (ch) {
		if (ch == '~') {					// animation
			ch = msg_read_byte(ee_adr++);
			if (ch != '~') {
				ch -= 'A';
				if (ch < ANIMATION_COUNT) {
					dmDisplayImage((const uint8_t*)pgm_read_word(&animation[ch]));
				}				
			}
		}
		else if (ch == 0xFF) {				// direct mode
			ch = msg_read_byte(ee_adr++);
			while (ch != 0xFF) {
				dmPrintByte(ch);
				ch = msg_read_byte(ee_adr++);
			}
		}
		else {								// character
			if (ch == '^') {				// special character
				ch = msg_read_byte(ee_adr++);
				if (ch != '^') {
					ch += 63;
				}
			}
			dmPrintChar(ch);
		}
		ch = msg_read_byte(ee_adr++);
		if (ch) { dmPrintByte(0); }			// print a narrow space except for the last character					
	}
	ch = msg_read_byte(ee_adr);				// read mode byte of next message
	if (ch)		{ return(ee_adr); }
		else	{ return(messages); }		// restart all-over if mode byte is 0
}		


/*======================================================================
	Function:		SerialInput
	Input:			none
	Output:			none
	Description:	Read input from serial interface.

					Escape characters:

					Character '^' is used to access special characters by 
					shifting the character code of the next character by 96 
					so that e. g. '^A' becomes char(161).
					To enter a '^' character simply double it: '^^'

					Character '~' followed by an upper case letter is used
					to insert (animation) data from flash.
					
					The <TAB> character is used to enter direct mode in which 
					the following bytes are directly written to the display 
					memory without being decoded using the character font.
					Direct mode is ended by 0xFF.
======================================================================*/
void SerialInput(void)
{
	uint8_t state = IDLE;
	uint8_t ch;
	
	
}


/********
 * main *
 ********/

int main(void)
{
	uint8_t a = 0;					// index of current animation
	
	InitHardware();
	dmInit();
	sei();							// enable interrupts

	dmPrintChar(129);				// show logo
	_delay_ms(1000);
	msg_ptr = DisplayMessage(msg_ptr);

	while(1)
	{
		if (button == PB_RELEASE) {			// short button press
			msg_ptr = DisplayMessage(msg_ptr);
			button |= PB_ACK;
		}
		
		if (button == PB_LONGPRESS) {		// button pressed for some seconds
			dmClearDisplay();
			dmPrintChar(130);				// sad smiley
			_delay_ms(500);
			dmClearDisplay();
			_delay_ms(1000);
			GIFR = (1<<PCIF2);				// clear interrupt flag
			PCMSK2 = (1<<PCINT17);			// enable pin change interrupt
			GIMSK = (1<<PCIE2);				// enable pin change interrupt
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_mode();
			GIMSK = 0;						// disable all external interrupts (including pin change)
			dmPrintChar(131);				// happy smiley
			_delay_ms(500);
			msg_ptr = DisplayMessage(messages);
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
		dmScroll();							// do a scrolling step
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



ISR(USART0_RX_vect)
{
	static uint8_t state = IDLE;
	static uint8_t val;
	uint8_t ch;

	ch = UDR;							// read received character
	if (ch == 27) { state = RESET; }	// <ESC> resets the state machine
	#ifdef ECHO_RXD			// only for debugging purposes
		dmClearDisplay();
		dmPrintChar(ch);
	#endif

	switch (state) {
		case IDLE:
			if (ch == AUTH1_CHAR) { state = AUTH; }
			break;
		case AUTH:
			if (ch == AUTH2_CHAR) { state = NORMAL; }
			break;
		case CRLF:
			if ((ch == 13) || (ch == 10)) { break; }	// chr(13) = <CR>, chr(10) = <LF>
			state = NORMAL;
			// no break-> fall through to case NORMAL
		case NORMAL:
			if (ch == '^') { state = SPECIAL_CHAR; }
			else if (ch == '$') { val = 0;  state = HEX_CODE; }
			else if ((ch == 13) || (ch == 10)) {		// chr(13) = <CR>, chr(10) = <LF>
				eeprom_write_byte(ee_write_ptr++, 0);
				state = CRLF;
			}
			else { eeprom_write_byte(ee_write_ptr++, ch); }
			break;
		case SPECIAL_CHAR:
			eeprom_write_byte(ee_write_ptr++, ch+63);
			#ifdef ECHO_RXD			// only for debugging purposes
				dmClearDisplay();
				dmPrintChar(ch+63);
			#endif
			state = NORMAL;
			break;
		case HEX_CODE:
			ch -= '0';
			if (ch >= 17) { ch -= 7; }
			if (ch > 15) {				// any character below '0' or above 'F' terminates hex input
				eeprom_write_byte(ee_write_ptr++, val);
				#ifdef ECHO_RXD				// only for debugging purposes
					dmClearDisplay();
					dmPrintByte(val>>4);	// print high nibble
					dmPrintByte(val & 0x0F);// print low  nibble
				#endif
				state = NORMAL;
			}
			else { val <<= 4;  val += ch; }
			break;
		case RESET:
			msg_ptr = messages;
			ee_write_ptr = messages;
			dmClearDisplay();
			dmPrintChar(129);				// show logo
			state = IDLE;
		break;
	}
}


/*
ISR(TIMER1_COMPA_vect)
{
}


ISR(TIMER1_COMPB_vect)
{
}
*/