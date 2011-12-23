/*
 * SDL_playbookhw_c.h
 *
 *  Created on: Nov 23, 2011
 *      Author: jnicholl
 */

#ifndef SDL_PLAYBOOKHW_C_H_
#define SDL_PLAYBOOKHW_C_H_

#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_playbookvideo.h"

struct private_hwdata {
	screen_pixmap_t pixmap;
	screen_window_t window;
	screen_buffer_t front;
	screen_buffer_t back;
};

int PLAYBOOK_AllocHWSurface(_THIS, SDL_Surface *surface);
void PLAYBOOK_FreeHWSurface(_THIS, SDL_Surface *surface);
int PLAYBOOK_LockHWSurface(_THIS, SDL_Surface *surface);
void PLAYBOOK_UnlockHWSurface(_THIS, SDL_Surface *surface);
int PLAYBOOK_FlipHWSurface(_THIS, SDL_Surface *surface);
int PLAYBOOK_FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);

#endif /* SDL_PLAYBOOKHW_C_H_ */
