/*
 * SDL_playbookvideo_c.h
 *
 *  Created on: Nov 23, 2011
 *      Author: jnicholl
 */

#ifndef SDL_PLAYBOOKVIDEO_C_H_
#define SDL_PLAYBOOKVIDEO_C_H_

#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_playbookvideo.h"
#include <screen/screen.h>

/*
 * The following will make major differences in how the content is laid out on the screen.
 * Most of these have only really been tested with the 1024x600 PlayBook screen.
 *
 * __STRETCHED__ will stretch arbitrary resolutions onto the full screen, distorting if needed.
 * __STRETCH_PRESERVE_ASPECT__ will stretch uniformly until one dimension is the screen dimension,
 *    and letterbox with black in the other dimension.
 * __NOSCALE__ will center a window of the correct resolution in the middle of the full screen.
 *
 * __STRETCH_PRESERVE_ASPECT__ is the default.
 * You should leave only one of these options uncommented.
 */

#define __STRETCH_PRESERVE_ASPECT__ 1
//#define __STRETCHED__ 1
//#define __NOSCALE__ 1

/* Function to handle the stretch options above */
extern int PLAYBOOK_SetupStretch(_THIS, screen_window_t screenWindow, int width, int height);

/* Initialization/Query functions */
extern int PLAYBOOK_VideoInit(_THIS, SDL_PixelFormat *vformat);
extern SDL_Rect **PLAYBOOK_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
extern screen_window_t PLAYBOOK_CreateWindow();
extern SDL_Surface *PLAYBOOK_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
extern int PLAYBOOK_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
extern void PLAYBOOK_VideoQuit(_THIS);

/* etc. */
extern void PLAYBOOK_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

#endif /* SDL_PLAYBOOKVIDEO_C_H_ */
