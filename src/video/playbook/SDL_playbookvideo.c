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

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "PLAYBOOK" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_playbookvideo.h"
#include "SDL_playbookevents_c.h"
#include "SDL_playbookmouse_c.h"

#include <string.h>
#include <errno.h>

#define PLAYBOOKVID_DRIVER_NAME "playbook"

// FIXME: Wrap these up in a much cleaner way.
screen_context_t m_screenContext;
screen_event_t m_screenEvent;
screen_window_t m_screenWindow;
screen_buffer_t m_frontBuffer;
SDL_Rect *SDL_modelist[SDL_NUMMODES+1];
void* m_pixels;
int m_pitch;
void* m_buffer;
SDL_Surface *m_surface;

/* Initialization/Query functions */
static int PLAYBOOK_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **PLAYBOOK_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *PLAYBOOK_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int PLAYBOOK_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void PLAYBOOK_VideoQuit(_THIS);

/* Hardware surface functions */
static int PLAYBOOK_AllocHWSurface(_THIS, SDL_Surface *surface);
static int PLAYBOOK_LockHWSurface(_THIS, SDL_Surface *surface);
static void PLAYBOOK_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void PLAYBOOK_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void PLAYBOOK_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* PLAYBOOK driver bootstrap functions */

static int PLAYBOOK_Available(void)
{
	return 1;
}

static void PLAYBOOK_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *PLAYBOOK_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = PLAYBOOK_VideoInit;
	device->ListModes = PLAYBOOK_ListModes;
	device->SetVideoMode = PLAYBOOK_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = PLAYBOOK_SetColors;
	device->UpdateRects = PLAYBOOK_UpdateRects;
	device->VideoQuit = PLAYBOOK_VideoQuit;
	device->AllocHWSurface = PLAYBOOK_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = PLAYBOOK_LockHWSurface;
	device->UnlockHWSurface = PLAYBOOK_UnlockHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = PLAYBOOK_FreeHWSurface;
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = PLAYBOOK_InitOSKeymap;
	device->PumpEvents = PLAYBOOK_PumpEvents;

	device->free = PLAYBOOK_DeleteDevice;

	return device;
}

VideoBootStrap PLAYBOOK_bootstrap = {
	PLAYBOOKVID_DRIVER_NAME, "SDL dummy video driver",
	PLAYBOOK_Available, PLAYBOOK_CreateDevice
};


int PLAYBOOK_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i;
	int rc = screen_create_context(&m_screenContext, 0);
	if (rc) {
		SDL_SetError("Cannot create screen context: %s", strerror(errno));
		return -1;
	}

	rc = screen_create_event(&m_screenEvent);
	if (rc) {
		SDL_SetError("Cannot create event object: %s", strerror(errno));
		screen_destroy_context(m_screenContext);
		return -1;
	}

	m_screenWindow = 0;
	m_surface = 0;

	for ( i=0; i<SDL_NUMMODES; ++i ) {
		SDL_modelist[i] = SDL_malloc(sizeof(SDL_Rect));
		SDL_modelist[i]->x = SDL_modelist[i]->y = 0;
	}

	/* Modes sorted largest to smallest */
	SDL_modelist[0]->w = 1024; SDL_modelist[0]->h = 600;
	SDL_modelist[1]->w = 800; SDL_modelist[1]->h = 576;
	SDL_modelist[2]->w = 640; SDL_modelist[2]->h = 480;
	SDL_modelist[3] = NULL;

	/* Determine the screen depth (use default 32-bit depth) */
	vformat->BitsPerPixel = 32;
	vformat->BytesPerPixel = 4;
	this->info.hw_available = 0;
	// TODO: Enable window manager?
	// this->info.wm_available = 1;
	this->info.current_w = 1024;
	this->info.current_h = 600;

	/* We're done! */
	return(0);
}

SDL_Rect **PLAYBOOK_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	if (flags & SDL_FULLSCREEN ) {
		return SDL_modelist;
	} else {
		return (SDL_Rect**)-1; // HACK FOR DOSBOX makes it -1 SDL_modelist; // Consider changing this!
	}
}

SDL_Surface *PLAYBOOK_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	if (m_buffer) {
		SDL_free(m_buffer);
	}

	// FIXME: Use a screen pixmap
	m_buffer = SDL_malloc(width * height * (bpp / 8));
	if (!m_buffer) {
		SDL_SetError("Failed to allocate buffer");
		return NULL;
	}

	SDL_memset(m_buffer, 0, width * height * (bpp / 8));

	if ( !SDL_ReallocFormat(current, bpp, 0, 0, 0, 0)) {
		SDL_free(m_buffer);
		m_buffer = 0;
		SDL_SetError("Couldn't realloc pixel format");
		return NULL;
	}

	{
		screen_window_t screenWindow;
		int rc = 0;
		if (!m_screenWindow) {
			rc = screen_create_window(&screenWindow, m_screenContext);
			if (rc) {
				SDL_SetError("Cannot create window: %s", strerror(errno));
				return NULL;
			}
		} else {
			screen_destroy_window_buffers(m_screenWindow);
			screenWindow = m_screenWindow;
		}

			/*
			 * FIXME: More properties needed
			SCREEN_PROPERTY_USAGE
			SCREEN_PROPERTY_SWAP_INTERVAL
			 */
			int position[2] = {0, 0};
			rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_POSITION, position);
			if (rc) {
				SDL_SetError("Cannot position window: %s", strerror(errno));
				screen_destroy_window(screenWindow);
				return NULL;
			}

			int size[2] = {width, height};
			rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_SIZE, size);
			if (rc) {
				SDL_SetError("Cannot resize window: %s", strerror(errno));
				screen_destroy_window(screenWindow);
				return NULL;
			}

			rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_BUFFER_SIZE, size);
			if (rc) {
				SDL_SetError("Cannot resize window buffer: %s", strerror(errno));
				screen_destroy_window(screenWindow);
				return NULL;
			}

			int format = SCREEN_FORMAT_RGBA8888; // FIXME: Allow configurable format
			rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_FORMAT, &format);
			if (rc) {
				SDL_SetError("Cannot set window format: %s", strerror(errno));
				screen_destroy_window(screenWindow);
				return NULL;
			}

			int usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_WRITE; // FIXME: GL needs other usage
			rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_USAGE, &usage);
			if (rc) {
				SDL_SetError("Cannot set window usage: %s", strerror(errno));
				screen_destroy_window(screenWindow);
				return NULL;
			}

			int bufferCount = 1; // FIXME: Permit double-buffered windows
			rc = screen_create_window_buffers(screenWindow, bufferCount);
			if (rc) {
				SDL_SetError("Cannot create window buffer: %s", strerror(errno));
				return NULL;
			}

			screen_buffer_t windowBuffer[bufferCount];
			rc = screen_get_window_property_pv(screenWindow,
					SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&windowBuffer);
			if (rc) {
				SDL_SetError("Cannot get window render buffers: %s", strerror(errno));
				return NULL;
			}

			rc = screen_get_buffer_property_pv(windowBuffer[0], SCREEN_PROPERTY_POINTER, &m_pixels);
			if (rc) {
				SDL_SetError("Cannot get buffer pointer: %s", strerror(errno));
				return NULL;
			}

			rc = screen_get_buffer_property_iv(windowBuffer[0], SCREEN_PROPERTY_STRIDE, &m_pitch);
			if (rc) {
				SDL_SetError("Cannot get stride: %s", strerror(errno));
				return NULL;
			} else {
				fprintf(stderr, "Pitch is %d\n", m_pitch);
			}

			m_frontBuffer = windowBuffer[0];
			m_screenWindow = screenWindow;
	}

	current->flags &= ~SDL_RESIZABLE; /* no resize for Direct Context */
	current->flags |= SDL_FULLSCREEN;
	current->w = width;
	current->h = height;
	current->pitch = current->w * (bpp / 8);
	current->pixels = m_buffer;
	m_surface = current;

	return current;
}

/* FIXME: We don't actually allow hardware surfaces other than the main one */
static int PLAYBOOK_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void PLAYBOOK_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int PLAYBOOK_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void PLAYBOOK_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void PLAYBOOK_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	static int dirtyRects[256*4];
	int index = 0, i = 0;
	int y = 0, x = 0, ptr = 0, srcPtr = 0;
	unsigned char* pixels = (unsigned char*)m_pixels;
	SDL_Surface* src = m_surface;
	if (!m_surface) {
		if (m_screenWindow && m_frontBuffer) {
			memset(pixels, 0, m_pitch * 600);
			screen_post_window(m_screenWindow, m_frontBuffer, numrects, dirtyRects, 0);
		}
		return;
	}
	unsigned char* srcPixels = (unsigned char*)src->pixels;
	if (!srcPixels) {
		fprintf(stderr, "Can't handle palette yet\n");
		return; // FIXME: Handle palette
	}

	int rc = screen_get_buffer_property_pv(m_frontBuffer, SCREEN_PROPERTY_POINTER, &m_pixels);
	if (rc) {
		fprintf(stderr, "Cannot get buffer pointer: %s\n", strerror(errno));
		return;
	}

	rc = screen_get_buffer_property_iv(m_frontBuffer, SCREEN_PROPERTY_STRIDE, &m_pitch);
	if (rc) {
		fprintf(stderr, "Cannot get stride: %s\n", strerror(errno));
		return;
	}

	// FIXME: Bounds, sanity checking, resizing
	// TODO: Use screen_blit?
	for (i=0; i<numrects; i++) {
		dirtyRects[index] = rects[i].x;
		dirtyRects[index+1] = rects[i].y;
		dirtyRects[index+2] = rects[i].x + rects[i].w;
		dirtyRects[index+3] = rects[i].y + rects[i].h;

		for (y = dirtyRects[index+1]; y<dirtyRects[index+3]; y++) {
			for (x = dirtyRects[index]; x < dirtyRects[index+2]; x++) {
				ptr = y * m_pitch + x * 4;
				srcPtr = y * src->pitch + x * src->format->BytesPerPixel;
				pixels[ptr] = srcPixels[srcPtr];
				pixels[ptr+1] = srcPixels[srcPtr+1];
				pixels[ptr+2] = srcPixels[srcPtr+2];
				pixels[ptr+3] = 0xff;
			}
		}
		index += 4;
	}

	if (m_screenWindow && m_frontBuffer)
		screen_post_window(m_screenWindow, m_frontBuffer, numrects, dirtyRects, 0);
}

int PLAYBOOK_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	/* do nothing of note. */
	// FIXME: Do we need to handle palette?
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
// FIXME: Fix up cleanup process
void PLAYBOOK_VideoQuit(_THIS)
{
//	if (m_buffer) {
//		SDL_free(m_buffer);
//		m_buffer = 0;
//	}
	if (m_screenWindow) {
		screen_destroy_window_buffers(m_screenWindow);
		screen_destroy_window(m_screenWindow);
	}
	screen_destroy_event(m_screenEvent);
	screen_destroy_context(m_screenContext);
}
