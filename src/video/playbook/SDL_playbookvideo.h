/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_playbookvideo_h
#define _SDL_playbookvideo_h

#include "../SDL_sysvideo.h"
#include <screen/screen.h>

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this
#define _priv   this->hidden

#define SDL_NUMMODES 3

/* Private display data */

struct SDL_PrivateVideoData {
    int w, h;
    void *buffer;
    void *emu_context;
    char *tcoControlsDir;
    screen_context_t screenContext;
    screen_event_t screenEvent;
    screen_window_t screenWindow;
    screen_window_t mainWindow;
    screen_buffer_t frontBuffer;
    SDL_Surface *surface;
    void* pixels;
    int pitch;
    int eventYOffset;

    SDL_Rect *SDL_modelist[SDL_NUMMODES+1];

    // For 8bit video driver and OpenGL windows
    struct {
    	void *eglDisplay;
    	void *eglContext;
    	void *eglSurface;
    } eglInfo;

    struct {
    	unsigned int shader;
    	int positionAttrib;
    	int texcoordAttrib;
    	unsigned int palette;
    	unsigned int screen[2];
    	int writableScreen;
    } glInfo;
};

#endif /* _SDL_playbookvideo_h */
