/*
 * SDL_playbookyuv_c.h
 *
 *  Created on: Jul 19, 2011
 *      Author: jnicholl
 */

#include "SDL_config.h"

#ifndef SDL_PLAYBOOKYUV_C_H_
#define SDL_PLAYBOOKYUV_C_H_

#include "SDL_video.h"
#include "SDL_playbookvideo.h"

extern SDL_Overlay* PLAYBOOK_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface* display);
extern int PLAYBOOK_LockYUVOverlay(_THIS, SDL_Overlay* overlay);
extern void PLAYBOOK_UnlockYUVOverlay(_THIS, SDL_Overlay* overlay);
extern int PLAYBOOK_DisplayYUVOverlay(_THIS, SDL_Overlay* overlay, SDL_Rect* src, SDL_Rect* dst);
extern void PLAYBOOK_FreeYUVOverlay(_THIS, SDL_Overlay* overlay);

#endif /* SDL_PLAYBOOKYUV_C_H_ */
