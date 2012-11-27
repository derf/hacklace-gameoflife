/*
 * animations.h
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


#ifndef ANIMATIONS_H_
#define ANIMATIONS_H_


typedef uint8_t const* animation_t;


/******************
 * animation data *
 ******************/

#define END_OF_DATA			0xFF

#include "arrow.h"
#include "fire.h"
#include "bounce.h"
#include "mountainrange.h"
#include "snow.h"
#include "tunnel.h"
#include "wink.h"
#include "ecg.h"
#include "checkers.h"
#include "tetris.h"
#include "glider.h"
#include "hop.h"
#include "pong.h"
#include "house.h"
#include "rocket.h"
#include "rainy.h"
#include "explode.h"
#include "droplet.h"
#include "psycho.h"
#include "tv_off.h"
#include "clock.h"
//#include "lady.h"
//#include "boobs.h"
//#include "thighs.h"


// list of all animations
const animation_t animation[] PROGMEM = {	arrow,
											fire,
											bounce,
											mountainrange,
											snow,
											tunnel,
											wink,
											ecg,
											checkers,
											tetris,
											glider,
											hop,
											pong,
											house,
											rocket,
											rainy,
											explode,
											droplet,
											psycho,
											tv_off,
											clock
											//lady,
											//boobs,
											//thighs
										};

#define ANIMATION_COUNT	(sizeof(animation)/sizeof(animation[0]))



#endif /* ANIMATIONS_H_ */