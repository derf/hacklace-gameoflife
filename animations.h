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

#include "animations/arrow.h"
#include "animations/fire.h"
#include "animations/bounce.h"
#include "animations/creeper.h"
#include "animations/snow.h"
#include "animations/tunnel.h"
#include "animations/wink.h"
#include "animations/ecg.h"
#include "animations/checkers.h"
#include "animations/tetris.h"
#include "animations/glider.h"
#include "animations/hop.h"
#include "animations/pong.h"
#include "animations/house.h"
#include "animations/rocket.h"
#include "animations/train.h"
#include "animations/explode.h"
#include "animations/droplet.h"
#include "animations/psycho.h"
#include "animations/batt.h"
//#include "animations/tv_off.h"
//#include "animations/clock.h"
//#include "animations/lady.h"
//#include "animations/boobs.h"
//#include "animations/thighs.h"


// list of all animations
const animation_t animation[] PROGMEM = {	arrow,
											fire,
											bounce,
											creeper,
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
											train,
											explode,
											droplet,
											psycho,
											batt
											//tv_off,
											//clock,
											//lady,
											//boobs,
											//thighs
										};

#define ANIMATION_COUNT	(sizeof(animation)/sizeof(animation[0]))



#endif /* ANIMATIONS_H_ */
