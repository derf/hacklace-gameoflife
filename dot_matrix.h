/*
 * dot_matrix.h
 *
 */ 

/**********************************************************************************

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


#ifndef DOT_MATRIX_H_
#define DOT_MATRIX_H_


/*************
 * constants *
 *************/

// dot matrix display
#define DISP_COLUMNS		5			// number of columns (range 1..8)
#define DISP_ROWS			7			// number of rows (range 1..8)
#define DISP_TYPE			0			// 1 = common column anode (TA), 0 = common column cathode (TC)
//#define DISP_UPDOWN						// if defined -> display is upside down
#define DOT_MATRIX_TYPE		Tx07-11		// choose Tx07-11 (Kingbright) or HDSP5403 (Hewlett Packard)
//#define DOT_MATRIX_TYPE		HDSP5403

// display memory
#define DISP_MAX			5			// size of display memory in bytes (1 byte = 1 column, range 5..240)

// scrolling directions
#define FORWARD				0			// text moves from right to left
#define BACKWARD			1
#define BIDIRECTIONAL		2			// text reverses direction

// font
#define CHAR_WIDTH			5			// maximum width of a character
#define SPC					127			// narrow space used as spacing between characters

// connection map for the rows and columns of the dot matrix display
// Note: Row 1 is the top row and column 1 is the leftmost column.
#define DISP_MASK_A			0b00000011	// set every bit that is connected to the dot matrix
#define DISP_MASK_B			0b01111110	// set every bit that is connected to the dot matrix
#define DISP_MASK_D			0b00011110	// set every bit that is connected to the dot matrix

#define A					0			// do not change
#define B					1			// do not change
#define D					2			// do not change

#if DOT_MATRIX_TYPE == Tx07-11
	// columns
	#define C1_PORT			D			// column 1 is connected to PD4 etc.
	#define C1				4
	#define C2_PORT			D
	#define C2				2
	#define C3_PORT			B
	#define C3				3
	#define C4_PORT			B
	#define C4				6
	#define C5_PORT			B
	#define C5				5
	// rows
	#define R1_PORT			B
	#define R1				1
	#define R2_PORT			B
	#define R2				2
	#define R3_PORT			D
	#define R3				3
	#define R4_PORT			B
	#define R4				4
	#define R5_PORT			A
	#define R5				0
	#define R6_PORT			A
	#define R6				1
	#define R7_PORT			D
	#define R7				1

#elif DOT_MATRIX_TYPE == HDSP5403
	// columns
	#define C1_PORT			B			// column 1 is connected to PB3 etc.
	#define C1				3
	#define C2_PORT			B
	#define C2				4
	#define C3_PORT			D
	#define C3				4
	#define C4_PORT			D
	#define C4				2
	#define C5_PORT			D
	#define C5				1
	// rows
	#define R1_PORT			B
	#define R1				6
	#define R2_PORT			B
	#define R2				5
	#define R3_PORT			A
	#define R3				1
	#define R4_PORT			A
	#define R4				0
	#define R5_PORT			D
	#define R5				3
	#define R6_PORT			B
	#define R6				2
	#define R7_PORT			B
	#define R7				1

#else
	#error "Unknown dot matrix type"
#endif


/**************
 * prototypes *
 **************/
void dmInit(void);
void dmDisplay(void);
uint8_t dmScroll(void);
void dmSetScrolling(uint8_t inc, uint8_t dir, uint8_t delay);
void dmClearDisplay(void);
void dmDisplayImage(const uint8_t* image);
void dmWakeUp();
void dmPrintChar(uint8_t ch);

// The following function was commented out to save flash memory.
// Uncomment it if you want to use it.
//void dmPrintString(const char* st);



#endif /* DOT_MATRIX_H_ */
