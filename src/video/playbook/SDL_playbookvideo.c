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

#include <bps/bps.h>
#include <bps/screen.h>
#include <bps/event.h>
#include <bps/orientation.h>
#include <bps/navigator.h>

#include "touchcontroloverlay.h"
#include <unistd.h>
#include <time.h>

#include "SDL_playbookvideo.h"
#include "SDL_playbookvideo_c.h"
#include "SDL_playbookvideo_8bit_c.h"
#include "SDL_playbookevents_c.h"
#include "SDL_playbookhw_c.h"
#include "SDL_playbooktouch_c.h"
#include "SDL_playbookyuv_c.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>

#define PLAYBOOKVID_DRIVER_NAME "playbook"

static int PLAYBOOK_Available(void)
{
	return 1;
}

static void PLAYBOOK_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device);
}

static SDL_VideoDevice *PLAYBOOK_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice) + sizeof(struct SDL_PrivateVideoData));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)(device+1);
	} else {
		SDL_OutOfMemory();
		return(0);
	}

	/* Set the function pointers */
	device->VideoInit = PLAYBOOK_VideoInit;
	device->ListModes = PLAYBOOK_ListModes;
	device->SetVideoMode = PLAYBOOK_SetVideoMode;
	device->CreateYUVOverlay = PLAYBOOK_CreateYUVOverlay;
	device->SetColors = PLAYBOOK_SetColors;
	device->UpdateRects = PLAYBOOK_UpdateRects;
	device->VideoQuit = PLAYBOOK_VideoQuit;
	device->AllocHWSurface = PLAYBOOK_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = PLAYBOOK_FillHWRect;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = PLAYBOOK_LockHWSurface;
	device->UnlockHWSurface = PLAYBOOK_UnlockHWSurface;
	device->FlipHWSurface = PLAYBOOK_FlipHWSurface;
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
	PLAYBOOKVID_DRIVER_NAME, "SDL PlayBook (libscreen) video driver",
	PLAYBOOK_Available, PLAYBOOK_CreateDevice
};

int PLAYBOOK_8Bit_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	if (PLAYBOOK_VideoInit(this, vformat) == -1)
		return -1;
	else {
		/* Determine the screen depth (use default 32-bit depth) */
		vformat->BitsPerPixel = 8;
		vformat->BytesPerPixel = 1;
		this->info.blit_fill = 0;
		this->info.hw_available = 0;
	}
	return 0;
}

int PLAYBOOK_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i;
	int res = 0xDEADBEEF; // Workaround apparent compiler bug. In release mode, this function does not return 0.
	int *result = &res;
	screen_display_t displays[1] = {0};
	int screenResolution[2];

	int rc = screen_create_context(&_priv->screenContext, 0);
	if (rc) {
		SDL_SetError("Cannot create screen context: %s", strerror(errno));
		*result = -1;
		goto end;
	}

	rc = screen_create_event(&_priv->screenEvent);
	if (rc) {
		SDL_SetError("Cannot create event object: %s", strerror(errno));
		screen_destroy_context(_priv->screenContext);
		*result = -1;
		goto end;
	}

	if (BPS_SUCCESS != bps_initialize()) {
		SDL_SetError("Cannot initialize BPS library: %s", strerror(errno));
		screen_destroy_event(_priv->screenEvent);
		screen_destroy_context(_priv->screenContext);
		*result = -1;
		goto end;
	}

	if (BPS_SUCCESS != navigator_rotation_lock(true)) {
		SDL_SetError("Cannot set rotation lock: %s", strerror(errno));
		bps_shutdown();
		screen_destroy_event(_priv->screenEvent);
		screen_destroy_context(_priv->screenContext);
		*result = -1;
		goto end;
	}

	if (BPS_SUCCESS != navigator_request_events(0)) {
		SDL_SetError("Cannot get navigator events: %s", strerror(errno));
		bps_shutdown();
		screen_destroy_event(_priv->screenEvent);
		screen_destroy_context(_priv->screenContext);
		*result = -1;
		goto end;
	}

	if (BPS_SUCCESS != screen_request_events(_priv->screenContext)) {
		SDL_SetError("Cannot get screen events: %s", strerror(errno));
		bps_shutdown();
		screen_destroy_event(_priv->screenEvent);
		screen_destroy_context(_priv->screenContext);
		*result = -1;
		goto end;
	}

	rc = screen_get_context_property_pv(_priv->screenContext, SCREEN_PROPERTY_DISPLAYS, (void**)&displays);
	if (rc) {
		SDL_SetError("Cannot get current display: %s", strerror(errno));
		screen_stop_events(_priv->screenContext);
		screen_destroy_event(_priv->screenEvent);
		screen_destroy_context(_priv->screenContext);
		bps_shutdown();
		*result = -1;
		goto end;
	}

	rc = screen_get_display_property_iv(displays[0], SCREEN_PROPERTY_NATIVE_RESOLUTION, screenResolution);
	if (rc) {
		SDL_SetError("Cannot get native resolution: %s", strerror(errno));
		screen_stop_events(_priv->screenContext);
		screen_destroy_event(_priv->screenEvent);
		screen_destroy_context(_priv->screenContext);
		bps_shutdown();
		*result = -1;
		goto end;
	}

	_priv->screenWindow = 0;
	_priv->surface = 0;

	for ( i=0; i<SDL_NUMMODES; ++i ) {
		_priv->SDL_modelist[i] = SDL_malloc(sizeof(SDL_Rect));
		_priv->SDL_modelist[i]->x = _priv->SDL_modelist[i]->y = 0;
	}

	/* Modes sorted largest to smallest */
	_priv->SDL_modelist[0]->w = screenResolution[0]; _priv->SDL_modelist[0]->h = screenResolution[1];
	_priv->SDL_modelist[1]->w = 800; _priv->SDL_modelist[1]->h = 576;
	_priv->SDL_modelist[2]->w = 640; _priv->SDL_modelist[2]->h = 480;
	_priv->SDL_modelist[3] = NULL;

	/* Determine the screen depth (use default 32-bit depth) */
	vformat->BitsPerPixel = 32;
	vformat->BytesPerPixel = 4;

	/* Hardware surfaces are not available */
	this->info.hw_available = 1;
	this->info.blit_fill = 1;
	this->info.blit_hw = 0;
	this->info.blit_hw_A = 0;
	this->info.blit_hw_CC = 0;

	/* There is no window manager to talk to */
	this->info.wm_available = 0;

	/* Full screen size is 1024x600 */
	this->info.current_w = screenResolution[0];
	this->info.current_h = screenResolution[1];

	/* We're done! */
	*result = 0;
end:
	return *result;
}

SDL_Rect **PLAYBOOK_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	if (flags & SDL_FULLSCREEN ) {
		return _priv->SDL_modelist;
	} else {
		return (SDL_Rect**)-1; // We only support full-screen video modes.
	}
}

screen_window_t PLAYBOOK_CreateWindow(_THIS, SDL_Surface *current,
		int width, int height, int bpp)
{
	screen_window_t screenWindow;
	int rc = 0;
	if (!_priv->screenWindow) {
		rc = screen_create_window(&screenWindow, _priv->screenContext);
		if (rc) {
			SDL_SetError("Cannot create window: %s", strerror(errno));
			return NULL;
		}

		char groupName[256] = {0};
		snprintf(groupName, 256, "sdl-%dx%dx%d-%u", width, height, bpp, time(NULL));
		rc = screen_create_window_group(screenWindow, groupName);
		if (rc) {
			SDL_SetError("Cannot set window group: %s", strerror(errno));
			screen_destroy_window(screenWindow);
			return NULL;
		}
	} else {
		if (current->hwdata)
			SDL_free(current->hwdata);
		tco_shutdown(_priv->emu_context);
		screen_destroy_window_buffers(_priv->screenWindow);
		screenWindow = _priv->screenWindow;
	}

	int position[2] = {0, 0};
	rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_POSITION, position);
	if (rc) {
		SDL_SetError("Cannot position window: %s", strerror(errno));
		screen_destroy_window(screenWindow);
		return NULL;
	}

	int idle_mode = SCREEN_IDLE_MODE_KEEP_AWAKE; // TODO: Handle idle gracefully?
	rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_IDLE_MODE, &idle_mode);
	if (rc) {
		SDL_SetError("Cannot disable idle mode: %s", strerror(errno));
		screen_destroy_window(screenWindow);
		return NULL;
	}

	return screenWindow;
}

SDL_Surface *PLAYBOOK_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
//	fprintf(stderr, "SetVideoMode: %dx%d %dbpp\n", width, height, bpp);
	if (width == 640 && height == 400)
		height = 480;
	screen_window_t screenWindow = PLAYBOOK_CreateWindow(this, current, width, height, bpp);
	if (screenWindow == NULL)
		return NULL;

	int rc;
	int format = 0;
	int sizeOfWindow[2] = {1024, 600};
	rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_SIZE, sizeOfWindow);
	if (rc) {
		SDL_SetError("Cannot resize window: %s", strerror(errno));
		screen_destroy_window(screenWindow);
		return NULL;
	}

	int sizeOfBuffer[2] = {width, height};
	rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_BUFFER_SIZE, sizeOfBuffer);
	if (rc) {
		SDL_SetError("Cannot resize window buffer: %s", strerror(errno));
		screen_destroy_window(screenWindow);
		return NULL;
	}

	switch (bpp) {
	case 8:
		fprintf(stderr, "Unsupported bpp: set pb-8bit environment variable!\n");
		format = SCREEN_FORMAT_BYTE;
		return NULL;
		break;
	case 16:
		SDL_ReallocFormat(current, 16, 0x0000f800, 0x0000007e0, 0x0000001f, 0);
		format = SCREEN_FORMAT_RGB565;
		break;
	case 32:
		SDL_ReallocFormat(current, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
		format = SCREEN_FORMAT_RGBX8888;
		break;
	}
	rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_FORMAT, &format);
	if (rc) {
		SDL_SetError("Cannot set window format: %s", strerror(errno));
		screen_destroy_window(screenWindow);
		return NULL;
	}

	int usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE; // FIXME: GL needs other usage
	rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_USAGE, &usage);
	if (rc) {
		SDL_SetError("Cannot set window usage: %s", strerror(errno));
		screen_destroy_window(screenWindow);
		return NULL;
	}

	int bufferCount = 1; // FIXME: (flags & SDL_DOUBLEBUF)?2:1; - Currently double-buffered surfaces are slow!
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

	rc = screen_get_buffer_property_pv(windowBuffer[0], SCREEN_PROPERTY_POINTER, &_priv->pixels);
	if (rc) {
		SDL_SetError("Cannot get buffer pointer: %s", strerror(errno));
		return NULL;
	}

	rc = screen_get_buffer_property_iv(windowBuffer[0], SCREEN_PROPERTY_STRIDE, &_priv->pitch);
	if (rc) {
		SDL_SetError("Cannot get stride: %s", strerror(errno));
		return NULL;
	}

	initializeOverlay(this, screenWindow);

	_priv->frontBuffer = windowBuffer[0];
	_priv->screenWindow = screenWindow;

	current->hwdata = SDL_malloc(sizeof(struct private_hwdata));
	current->hwdata->pixmap = 0;
	current->hwdata->window = screenWindow;
	current->hwdata->front = windowBuffer[0];
	if (bufferCount > 1) {
		current->flags |= SDL_DOUBLEBUF;
		current->hwdata->back = windowBuffer[1];
	} else {
		current->hwdata->back = 0;
	}
	current->flags &= ~SDL_RESIZABLE; /* no resize for Direct Context */
	current->flags |= SDL_FULLSCREEN;
	current->flags |= SDL_HWSURFACE;
	current->w = width;
	current->h = height;
	current->pitch = _priv->pitch;
	current->pixels = _priv->pixels;
	_priv->surface = current;
	return current;
}

void PLAYBOOK_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	static int dirtyRects[256*4];
	int index = 0, i = 0;
	for (i=0; i<numrects; i++) {
		dirtyRects[index] = rects[i].x;
		dirtyRects[index+1] = rects[i].y;
		dirtyRects[index+2] = rects[i].x + rects[i].w;
		dirtyRects[index+3] = rects[i].y + rects[i].h;
		index += 4;
	}

	screen_post_window(_priv->screenWindow, _priv->frontBuffer, numrects, dirtyRects, 0);
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
	if (_priv->screenWindow) {
		screen_destroy_window_buffers(_priv->screenWindow);
		screen_destroy_window(_priv->screenWindow);
	}
	screen_stop_events(_priv->screenContext);
	screen_destroy_event(_priv->screenEvent);
	screen_destroy_context(_priv->screenContext);
	bps_shutdown();
	tco_shutdown(_priv->emu_context);
	this->screen = 0;
}
