/*
 * SDL_playbookyuv.c
 *
 *  Created on: Jul 19, 2011
 *      Author: jnicholl
 */
#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_playbookyuv_c.h"
#include "../SDL_yuvfuncs.h"

static struct private_yuvhwfuncs PLAYBOOK_yuvfuncs =
{
    PLAYBOOK_LockYUVOverlay,
    PLAYBOOK_UnlockYUVOverlay,
    PLAYBOOK_DisplayYUVOverlay,
    PLAYBOOK_FreeYUVOverlay
};

SDL_Overlay* PLAYBOOK_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface* display)
{
	SDL_Overlay* overlay;
	struct private_yuvhwdata* hwdata;
	int screenFormat = 0;
	fprintf(stderr, "Creating YUV overlay: %dx%d\n", width, height);

	overlay = SDL_calloc(1, sizeof(SDL_Overlay));
	if (overlay == NULL)
	{
		SDL_OutOfMemory();
		return NULL;
	}

	switch (format) {
	case SDL_YV12_OVERLAY:
		screenFormat = SCREEN_FORMAT_YV12;
		break;
	case SDL_IYUV_OVERLAY:
		screenFormat = SCREEN_FORMAT_YUV420; // TODO: Verify this!
		break;
	case SDL_YUY2_OVERLAY:
		screenFormat = SCREEN_FORMAT_YUY2;
		break;
	case SDL_UYVY_OVERLAY:
		screenFormat = SCREEN_FORMAT_UYVY;
		break;
	case SDL_YVYU_OVERLAY:
		screenFormat = SCREEN_FORMAT_YVYU;
		break;
	default:
		SDL_SetError("PLAYBOOK_CreateYUVOverlay: unrecognized format\n");
		return NULL;
	}

	overlay->format = format;
	overlay->w = width;
	overlay->h = height;
	overlay->hwdata = NULL;

	overlay->hwfuncs = &PLAYBOOK_yuvfuncs;
//	hwdata = SDL_calloc(1, sizeof(struct private_yuvhwdata));
//
//	if (hwdata == NULL)
//	{
//		SDL_OutOfMemory();
//		SDL_FreeYUVOverlay(overlay);
//		return NULL;
//	}
//	overlay->hwdata = hwdata;
	return overlay;
}

int PLAYBOOK_LockYUVOverlay(_THIS, SDL_Overlay* overlay)
{
	return 0;
}

void PLAYBOOK_UnlockYUVOverlay(_THIS, SDL_Overlay* overlay)
{

}

int PLAYBOOK_DisplayYUVOverlay(_THIS, SDL_Overlay* overlay, SDL_Rect* src, SDL_Rect* dst)
{
	return -1;
}

void PLAYBOOK_FreeYUVOverlay(_THIS, SDL_Overlay* overlay)
{

}
