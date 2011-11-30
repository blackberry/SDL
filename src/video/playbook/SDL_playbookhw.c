/*
 * SDL_playbookhw.c
 *
 *  Created on: Nov 23, 2011
 *      Author: jnicholl
 */

#include "SDL_config.h"

#include "SDL_playbookhw_c.h"
#include <errno.h>

int PLAYBOOK_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	fprintf(stderr, "Allocate HW surface %08x\n", surface);
	if (surface->hwdata != NULL) {
		fprintf(stderr, "Surface already has hwdata\n");
		return -1;
	}

	surface->hwdata = SDL_malloc(sizeof(struct private_hwdata));
	if (surface->hwdata == NULL) {
		SDL_OutOfMemory();
		return -1;
	}

	int rc = screen_create_pixmap( &surface->hwdata->pixmap, _priv->screenContext);
	if (rc) {
		fprintf(stderr, "Failed to create HW surface: screen_create_pixmap returned %s\n", strerror(errno));
		goto fail1;
	}

	int size[2] = {surface->w, surface->h};
	rc = screen_set_pixmap_property_iv(surface->hwdata->pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
	if (rc) {
		fprintf(stderr, "Failed to set SCREEN_PROPERTY_BUFFER_SIZE: screen_set_pixmap_property_iv returned %s\n", strerror(errno));
		goto fail1;
	}

	int format = SCREEN_FORMAT_RGBA8888;
	rc = screen_set_pixmap_property_iv(surface->hwdata->pixmap, SCREEN_PROPERTY_FORMAT, &format);
	if (rc) {
		fprintf(stderr, "Failed to set SCREEN_PROPERTY_FORMAT: screen_set_pixmap_property_iv returned %s\n", strerror(errno));
		goto fail1;
	}

	rc = screen_create_pixmap_buffer(surface->hwdata->pixmap);
	if (rc) {
		fprintf(stderr, "Failed to allocate HW surface: screen_create_pixmap_buffer returned %s\n", strerror(errno));
		goto fail2;
	}

	surface->flags |= SDL_HWSURFACE;
	surface->flags |= SDL_PREALLOC;

	return 0;

fail2:
	screen_destroy_pixmap(surface->hwdata->pixmap);
fail1:
	SDL_free(surface->hwdata);
	surface->hwdata = 0;

	return -1;
}

void PLAYBOOK_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	fprintf(stderr, "Free HW surface %08x\n", surface);
	if (surface->hwdata) {
		screen_destroy_pixmap_buffer(surface->hwdata->pixmap);
		screen_destroy_pixmap(surface->hwdata->pixmap);
	}
	return;
}

int PLAYBOOK_LockHWSurface(_THIS, SDL_Surface *surface)
{
	/* Currently does nothing */
	return(0);
}

void PLAYBOOK_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	/* Currently does nothing */
	return;
}

int PLAYBOOK_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	fprintf(stderr, "Flip HW surface %08x\n", surface);
	// FIXME: This doesn't work properly yet. It flashes black, I think the new render buffers are wrong.
	static int fullRect[] = {0, 0, 1024, 600};
	//screen_flush_blits(_priv->screenContext, 0);
	int result = screen_post_window(_priv->screenWindow, surface->hwdata->front, 1, fullRect, 0);

	screen_buffer_t windowBuffer[2];
	int rc = screen_get_window_property_pv(_priv->screenWindow,
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
	surface->hwdata->front = windowBuffer[0];
	surface->pixels = _priv->pixels;
	return 0;
}

int PLAYBOOK_FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
	fprintf(stderr, "Fill HW rect\n");
	if (dst->flags & SDL_HWSURFACE) {
		int attribs[] = {SCREEN_BLIT_DESTINATION_X, rect->x,
						SCREEN_BLIT_DESTINATION_Y, rect->y,
						SCREEN_BLIT_DESTINATION_WIDTH, rect->w,
						SCREEN_BLIT_DESTINATION_HEIGHT, rect->h,
						SCREEN_BLIT_COLOR, color,
						SCREEN_BLIT_END};
		screen_fill(_priv->screenContext, _priv->frontBuffer, attribs);
	}
	return 0;
}

