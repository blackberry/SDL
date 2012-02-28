/*
 * SDL_playbooktouch_c.h
 *
 *  Created on: Nov 23, 2011
 *      Author: jnicholl
 */

#ifndef SDL_PLAYBOOKTOUCH_C_H_
#define SDL_PLAYBOOKTOUCH_C_H_

#include "SDL_config.h"

#include "SDL_playbookvideo.h"
#include <bps/screen.h>

extern int handleKey(int sym, int mod, int scancode, uint16_t unicode, int event);
extern int handleDPad(int angle, int event);
extern int handleTouch(int dx, int dy);
extern int handleMouseButton(int button, int mask, int event);
extern int handleTap();
extern int handleTouchScreen(int x, int y, int tap, int hold);
extern void locateTCOControlFile(_THIS);
extern void initializeOverlay(_THIS, screen_window_t screenWindow);

#endif /* SDL_PLAYBOOKTOUCH_C_H_ */
