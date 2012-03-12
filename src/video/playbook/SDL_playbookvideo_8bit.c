/*
 * SDL_playbookvideo_8bit.c
 *
 *  Created on: Nov 23, 2011
 *      Author: jnicholl
 */

#include "SDL_config.h"

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

#include "SDL_playbookvideo_c.h"
#include "SDL_playbookvideo_8bit_c.h"
#include "SDL_playbookevents_c.h"
#include "SDL_playbookhw_c.h"
#include "SDL_playbooktouch_c.h"
#include "SDL_playbookyuv_c.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <errno.h> // ::errno
#include <time.h> // struct tm, clock_gettime

#define PLAYBOOKVID_8Bit_DRIVER_NAME "pb-8bit"

static void egl_perror(const char *msg)
{
    static const char *errmsg[] = {
        "function succeeded",
        "EGL is not initialized, or could not be initialized, for the specified display",
        "cannot access a requested resource",
        "failed to allocate resources for the requested operation",
        "an unrecognized attribute or attribute value was passed in an attribute list",
        "an EGLConfig argument does not name a valid EGLConfig",
        "an EGLContext argument does not name a valid EGLContext",
        "the current surface of the calling thread is no longer valid",
        "an EGLDisplay argument does not name a valid EGLDisplay",
        "arguments are inconsistent",
        "an EGLNativePixmapType argument does not refer to a valid native pixmap",
        "an EGLNativeWindowType argument does not refer to a valid native window",
        "one or more argument values are invalid",
        "an EGLSurface argument does not name a valid surface configured for rendering",
        "a power management event has occurred",
    };

    fprintf(stderr, "%s: %s\n", msg, errmsg[eglGetError() - EGL_SUCCESS]);
}

int PLAYBOOK_8Bit_Available(void);
SDL_VideoDevice *PLAYBOOK_8Bit_CreateDevice(int devindex);

VideoBootStrap PLAYBOOK_8Bit_bootstrap = {
	PLAYBOOKVID_8Bit_DRIVER_NAME, "SDL PlayBook (libscreen) 8-bit video driver",
	PLAYBOOK_8Bit_Available, PLAYBOOK_8Bit_CreateDevice
};

int PLAYBOOK_8Bit_Available(void)
{
	const char *envr = SDL_getenv("SDL_VIDEODRIVER");
	if ((envr) && (SDL_strcmp(envr, PLAYBOOKVID_8Bit_DRIVER_NAME) == 0)) {
		return(1);
	}

	return(0);
}

static void PLAYBOOK_8Bit_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device);
}

SDL_VideoDevice *PLAYBOOK_8Bit_CreateDevice(int devindex)
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
	device->VideoInit = PLAYBOOK_8Bit_VideoInit;
	device->ListModes = PLAYBOOK_8Bit_ListModes;
	device->SetVideoMode = PLAYBOOK_8Bit_SetVideoMode;
	device->CreateYUVOverlay = PLAYBOOK_CreateYUVOverlay;
	device->SetColors = PLAYBOOK_8Bit_SetColors;
	device->UpdateRects = PLAYBOOK_8Bit_UpdateRects;
	device->VideoQuit = PLAYBOOK_8Bit_VideoQuit;
	device->AllocHWSurface = PLAYBOOK_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL; //PLAYBOOK_FillHWRect;
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

	device->free = PLAYBOOK_8Bit_DeleteDevice;

	return device;
}

SDL_Rect **PLAYBOOK_8Bit_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	if (format->BitsPerPixel != 8)
		return (SDL_Rect**)-1; // Only support 8 bpp.

	if (flags & SDL_FULLSCREEN ) {
		return _priv->SDL_modelist;
	} else {
		return (SDL_Rect**)-1; // We only support full-screen video modes.
	}
}

static int initializeGL(_THIS, int width, int height)
{
	const char *vs =
			"attribute vec2 a_position;\n"
			"attribute vec2 a_texcoord;\n"
			"varying vec2 v_texcoord;\n"
			"void main()\n"
			"{\n"
			"    gl_Position = vec4(a_position, 0.0, 1.0);\n"
			"    v_texcoord = a_texcoord;\n"
			"}\n";
	const char *fs =
			"uniform lowp sampler2D u_sampler;\n"
			"uniform lowp sampler2D u_palette;\n" // TODO: Palette should be 1D.
			"varying mediump vec2 v_texcoord;\n"
			"void main()\n"
			"{\n"
			"    lowp float p = texture2D(u_sampler, v_texcoord).r;\n"
			"    gl_FragColor = texture2D(u_palette, vec2(p,0.0));\n"
//			"    gl_FragColor = texture2D(u_palette, v_texcoord);\n"
//			"    gl_FragColor = texture2D(u_sampler, v_texcoord);\n"
//			"    gl_FragColor = vec4(p, 1.0, 0.0, 1.0);\n"
			"}\n";
	GLint status;
	GLuint v, f, id;
	GLchar log[256];
	GLuint textures[3];
	unsigned char palette[256*4] = {0};

	v = glCreateShader(GL_VERTEX_SHADER);
	if (!v) {
		goto error1;
	}
	glShaderSource(v, 1, &vs, 0);
	glCompileShader(v);
	glGetShaderiv(v, GL_COMPILE_STATUS, &status);
	if (GL_FALSE == status) {
		glGetShaderInfoLog(v, 256, NULL, log);
		fprintf(stderr, "Failed to compile vertex shader: %s\n", log);
		goto error2;
	}

	f = glCreateShader(GL_FRAGMENT_SHADER);
	if (!f) {
		goto error2;
	}
	glShaderSource(f, 1, &fs, 0);
	glCompileShader(f);
	glGetShaderiv(f, GL_COMPILE_STATUS, &status);
	if (GL_FALSE == status) {
		glGetShaderInfoLog(f, 256, NULL, log);
		fprintf(stderr, "Failed to compile fragment shader: %s\n", log);
		goto error3;
	}

	id = glCreateProgram();
	if (!id) {
		fprintf(stderr, "Failed to create shader program\n");
		goto error3;
	}
	glAttachShader(id, v);
	glAttachShader(id, f);
	glLinkProgram(id);

	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (GL_FALSE == status) {
		glGetProgramInfoLog(id, 256, NULL, log);
		fprintf(stderr, "Failed to link shader program: %s\n", log);
		goto error4;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	_priv->glInfo.shader = id;
	glUseProgram(id);
	glUniform1i(glGetUniformLocation(id, "u_sampler"), 0); // screen texture is TEXTURE0
	glUniform1i(glGetUniformLocation(id, "u_palette"), 1); // palette texture is TEXTURE1
	_priv->glInfo.positionAttrib = glGetAttribLocation(id, "a_position");
	_priv->glInfo.texcoordAttrib = glGetAttribLocation(id, "a_texcoord");

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(3, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, palette);

	_priv->glInfo.screen[0] = textures[0];
	_priv->glInfo.screen[1] = textures[1];
	_priv->glInfo.palette = textures[2];

	return 1; // Success
error4:
	glDeleteProgram(id);
error3:
	glDeleteShader(f);
error2:
	glDeleteShader(v);
error1:
	return 0; // Failed to initialize
}

static SDL_Palette *AllocatePalette(int size)
{
  SDL_Palette *palette;
  SDL_Color   *colors;

  palette = SDL_calloc (1, sizeof(SDL_Palette));
  if (!palette)
    {
      SDL_OutOfMemory();
      return NULL;
    }

  colors = SDL_calloc (size, sizeof(SDL_Color));
  if (!colors)
    {
      SDL_OutOfMemory();
      return NULL;
    }

  palette->ncolors = size;
  palette->colors  = colors;

  return palette;
}

SDL_Surface *PLAYBOOK_8Bit_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int rc;
	EGLint attributes[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint contextAttributes[3] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	EGLConfig configs[1];
	EGLint configCount;
	screen_window_t screenWindow;
	int format = SCREEN_FORMAT_RGBX8888;
	int sizeOfWindow[2] = {1024, 600};
//	int sizeOfBuffer[2] = {width, height};
	int usage = SCREEN_USAGE_OPENGL_ES2;
	EGLint eglSurfaceAttributes[3] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE };

	if (!_priv->screenWindow) {

		_priv->eglInfo.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (_priv->eglInfo.eglDisplay == EGL_NO_DISPLAY) {
			egl_perror("eglGetDisplay");
			goto error1;
		}

		rc = eglInitialize(_priv->eglInfo.eglDisplay, NULL, NULL);
		if (rc != EGL_TRUE) {
			egl_perror("eglInitialize");
			goto error1;
		}

		rc = eglBindAPI(EGL_OPENGL_ES_API);
		if (rc != EGL_TRUE) {
			egl_perror("eglBindAPI");
			goto error2;
		}

		rc = eglChooseConfig(_priv->eglInfo.eglDisplay, attributes, configs, 1, &configCount);
		if (rc != EGL_TRUE)	{
			egl_perror("eglBindAPI");
			eglTerminate(_priv->eglInfo.eglDisplay);
			return NULL;
		} else if (configCount <= 0)	{
			fprintf(stderr, "No matching configurations found.");
			goto error2;
		}

		_priv->eglInfo.eglContext = eglCreateContext(_priv->eglInfo.eglDisplay, configs[0], EGL_NO_CONTEXT, contextAttributes);
		if (_priv->eglInfo.eglContext == EGL_NO_CONTEXT)	{
			egl_perror("eglCreateContext");
			goto error2;
		}

		screenWindow = PLAYBOOK_CreateWindow(this, current, width, height, bpp);
		if (screenWindow == NULL) {
			goto error3;
		}

		rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_SIZE, sizeOfWindow);
		if (rc) {
			SDL_SetError("Cannot resize window: %s", strerror(errno));
			screen_destroy_window(screenWindow);
			return NULL;
		}

//		rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_BUFFER_SIZE, sizeOfBuffer);
//		if (rc) {
//			SDL_SetError("Cannot resize window buffer: %s", strerror(errno));
//			screen_destroy_window(screenWindow);
//			return NULL;
//		}

		rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_FORMAT, &format);
		if (rc) {
			SDL_SetError("Cannot set window format: %s", strerror(errno));
			goto error4;
		}

		rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_USAGE, &usage);
		if (rc) {
			SDL_SetError("Cannot set window usage: %s", strerror(errno));
			goto error4;
		}

		int angle = 0;
		char *orientation = getenv("ORIENTATION");
		if (orientation) {
			 angle = atoi(orientation);
		}
		rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_ROTATION, &angle);
		if (rc) {
			SDL_SetError("Cannot set window rotation: %s", strerror(errno));
			goto error4;
		}

		rc = screen_create_window_buffers(screenWindow, 2);
		if (rc) {
			SDL_SetError("Cannot create window buffers: %s", strerror(errno));
			goto error4;
		}

		_priv->eglInfo.eglSurface = eglCreateWindowSurface(_priv->eglInfo.eglDisplay, configs[0],
				screenWindow, (EGLint*)&eglSurfaceAttributes);
		if (_priv->eglInfo.eglSurface == EGL_NO_SURFACE) {
			egl_perror("eglCreateWindowSurface");
			goto error4;
		}

		rc = eglMakeCurrent(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglSurface, _priv->eglInfo.eglSurface, _priv->eglInfo.eglContext);
		if (rc != EGL_TRUE) {
			egl_perror("eglMakeCurrent");
			goto error5;
		}

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glDisable(GL_DEPTH_TEST);
		if (!initializeGL(this, width, height)) {
			SDL_SetError("Failed to initialize GL");
			goto error5;
		}

		locateTCOControlFile(this);
		if (_priv->tcoControlsDir) {
			initializeOverlay(this, screenWindow);
		}

		_priv->screenWindow = screenWindow;
	} else {
		glDeleteTextures(2, _priv->glInfo.screen);

		glActiveTexture(GL_TEXTURE0);
		glGenTextures(2, _priv->glInfo.screen);
		glBindTexture(GL_TEXTURE_2D, _priv->glInfo.screen[0]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_2D, _priv->glInfo.screen[1]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		SDL_free(_priv->surface->pixels);
	}
	current->flags &= ~SDL_RESIZABLE;
	current->flags |= SDL_FULLSCREEN;
	current->flags |= SDL_HWSURFACE;
	current->flags |= SDL_HWPALETTE;
	current->w = width;
	current->h = height;
	SDL_ReallocFormat(current, 8, 0, 0, 0, 0); // FIXME: Does this allocate twice?
	current->pitch = width;
	current->pixels = SDL_calloc(width * height, 1);
//	this->physpal = AllocatePalette(256);
//	current->format->palette = this->physpal;
	_priv->surface = current;

	return current;
error5:
	eglDestroySurface(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglSurface);
	_priv->eglInfo.eglSurface = 0;
error4:
	screen_destroy_window(screenWindow);
error3:
	eglDestroyContext(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglContext);
	_priv->eglInfo.eglContext = 0;
error2:
	eglTerminate(_priv->eglInfo.eglDisplay);
	_priv->eglInfo.eglDisplay = 0;
error1:
	return NULL;
}

static const unsigned sampleDuration = 100;
static unsigned frameCounter = 0;
static double lastSampledTime = 0;
static double getTime()
{
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	return tm.tv_sec * 1000.0f + tm.tv_nsec / 1000000.0f;
}

static void printFPS()
{
	double diff = getTime() - lastSampledTime;
	if (diff > sampleDuration) {
		double rawFPS = frameCounter / (diff/1000);
		unsigned sampleFPS = (unsigned)((rawFPS*100)/100);
		frameCounter = 0;
		lastSampledTime = getTime();
		fprintf(stderr, "FPS: %u\n", sampleFPS);
	}
}

void PLAYBOOK_8Bit_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	if (!this || !_priv || !_priv->surface)
		return;

	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _priv->glInfo.screen[_priv->glInfo.writableScreen]);
	_priv->glInfo.writableScreen = !_priv->glInfo.writableScreen;
	int i=0;

	// TODO: Need to respect the rects, rather than always update full screen.

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _priv->surface->w, _priv->surface->h,
			0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _priv->surface->pixels);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _priv->glInfo.palette);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(_priv->glInfo.positionAttrib);
	glEnableVertexAttribArray(_priv->glInfo.texcoordAttrib);
	static const GLfloat vertices[] = {-1.0,-1.0,1.0,-1.0,-1.0,1.0,1.0,1.0};
	static const GLfloat texCoords[] = {0.0,1.0,1.0,1.0,0.0,0.0,1.0,0.0};
	glVertexAttribPointer(_priv->glInfo.positionAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertices);
	glVertexAttribPointer(_priv->glInfo.texcoordAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texCoords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	eglSwapBuffers(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglSurface);

//	frameCounter++;
//	printFPS();
}

int PLAYBOOK_8Bit_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	static int flip = 0;
	flip++;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _priv->glInfo.palette);
	int i=0;
	static unsigned char c[256*4] = {0};
	for (i=0; i<ncolors; i++) {
		int j=i;//+firstcolor;
		c[4*j] = colors[i].r;
		c[4*j+1] = colors[i].g;
		c[4*j+2] = colors[i].b;
		c[4*j+3] = 0xff;
	}
	glTexSubImage2D(GL_TEXTURE_2D, 0, firstcolor, 0, ncolors, 1, GL_RGBA, GL_UNSIGNED_BYTE, c);//+firstcolor*4);
	if (flip) {
		glClear(GL_COLOR_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _priv->glInfo.screen[!_priv->glInfo.writableScreen]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(_priv->glInfo.positionAttrib);
		glEnableVertexAttribArray(_priv->glInfo.texcoordAttrib);
		static const GLfloat vertices[] = {-1.0,-1.0,1.0,-1.0,-1.0,1.0,1.0,1.0};
		static const GLfloat texCoords[] = {0.0,1.0,1.0,1.0,0.0,0.0,1.0,0.0};
		glVertexAttribPointer(_priv->glInfo.positionAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertices);
		glVertexAttribPointer(_priv->glInfo.texcoordAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texCoords);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		eglSwapBuffers(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglSurface);
		flip=0;
	}
//	frameCounter++;
//	printFPS();
	return(1);
}

void PLAYBOOK_8Bit_VideoQuit(_THIS)
{
	if (_priv->screenWindow) {
		eglDestroySurface(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglSurface);
		screen_destroy_window(_priv->screenWindow);
		eglDestroyContext(_priv->eglInfo.eglDisplay, _priv->eglInfo.eglContext);
		eglTerminate(_priv->eglInfo.eglDisplay);
	}
	screen_stop_events(_priv->screenContext);
	screen_destroy_event(_priv->screenEvent);
	screen_destroy_context(_priv->screenContext);
	bps_shutdown();
	if (_priv->tcoControlsDir) {
		tco_shutdown(_priv->emu_context);
	}
	this->screen = 0;
}

