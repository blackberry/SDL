/*
 * SDL_playbookvideo_gl_c.h
 *
 *  Created on: 2012-01-17
 *      Author: asimonov
 */

#ifndef SDL_PLAYBOOKVIDEO_GL_C_H_
#define SDL_PLAYBOOKVIDEO_GL_C_H_

#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_playbookvideo.h"

extern SDL_Surface *PLAYBOOK_SetVideoMode_GL(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags);

/* Sets the dll to use for OpenGL and loads it */
extern int PLAYBOOK_GL_LoadLibrary(_THIS, const char *path);

/* Retrieves the address of a function in the gl library */
extern void* PLAYBOOK_GL_GetProcAddress(_THIS, const char *proc);

/* Get attribute information from the windowing system. */
extern int PLAYBOOK_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);

/* Make the context associated with this driver current */
extern int PLAYBOOK_GL_MakeCurrent(_THIS);

/* Swap the current buffers in double buffer mode. */
extern void PLAYBOOK_GL_SwapBuffers(_THIS);

#endif /* SDL_PLAYBOOKVIDEO_GL_C_H_ */
