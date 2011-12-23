/*
 * SDL_playbookvideo_8bit_c.h
 *
 *  Created on: Nov 23, 2011
 *      Author: jnicholl
 */

#ifndef SDL_PLAYBOOKVIDEO_8BIT_C_H_
#define SDL_PLAYBOOKVIDEO_8BIT_C_H_

#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_playbookvideo.h"

extern int PLAYBOOK_8Bit_VideoInit(_THIS, SDL_PixelFormat *vformat);
extern SDL_Rect **PLAYBOOK_8Bit_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
extern SDL_Surface *PLAYBOOK_8Bit_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
extern void PLAYBOOK_8Bit_VideoQuit(_THIS);
extern int PLAYBOOK_8Bit_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
extern void PLAYBOOK_8Bit_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

#endif /* SDL_PLAYBOOKVIDEO_8BIT_C_H_ */
