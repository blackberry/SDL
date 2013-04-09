/*
 * tco_labelwindow.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_label_window.h"
#include "tco_win.h"

#include <stdlib.h>
#include <string.h>

tco_label_window_t *tco_label_window_create(screen_context_t context, int width, int height)
{
	tco_label_window_t *win = (tco_label_window_t *)malloc(sizeof(tco_label_window_t));
	if (!win) return win;

	if (!tco_win_create2(&win->base, context, width, height, 0)) {
		free(win);
		return 0;
	}

	if (!win->base.valid || !tco_win_setZOrder(&win->base, 6) ||
			!tco_win_setSensitivity(&win->base, 0)) {
		tco_win_destroy(&win->base);
		free(win);
		return 0;
	}

	win->offset[0] = 0;
	win->offset[1] = 0;
	win->scale[0] = 1.0f;
	win->scale[1] = 1.0f;

	return win;
}

void tco_label_window_destroy(tco_label_window_t *window)
{
	if (!window) return;
	tco_win_destroy(&window->base);
	free(window);
}

void tco_label_window_draw(tco_label_window_t *window, screen_pixmap_t pixmap, int width, int height)
{
	screen_buffer_t buffer;
	unsigned char *pixels;
	int stride;
	if (!tco_win_pixels(&window->base, &buffer, &pixels, &stride)) {
		fprintf(stderr, "Unable to get label window buffer\n");
		return;
	}

	screen_buffer_t pixmapBuffer;
	screen_get_pixmap_property_pv(pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&pixmapBuffer);

	// FIXME: Figure out why the blit here doesn't work.
//	int attribs[] = {
//			SCREEN_BLIT_SOURCE_X, 0,
//			SCREEN_BLIT_SOURCE_Y, 0,
//			SCREEN_BLIT_SOURCE_WIDTH, width,
//			SCREEN_BLIT_SOURCE_HEIGHT, height,
//			SCREEN_BLIT_DESTINATION_X, 0,
//			SCREEN_BLIT_DESTINATION_Y, 0,
//			SCREEN_BLIT_DESTINATION_WIDTH, window->base.size[0],
//			SCREEN_BLIT_DESTINATION_HEIGHT, window->base.size[1],
//			SCREEN_BLIT_END
//	};
//	screen_blit(window->base.context, buffer, pixmapBuffer, attribs);
	{
		unsigned char *srcData;
		int srcStride;
		screen_get_buffer_property_pv(pixmapBuffer, SCREEN_PROPERTY_POINTER, (void **)&srcData);
		screen_get_buffer_property_iv(pixmapBuffer, SCREEN_PROPERTY_STRIDE, (int*)&srcStride);
		if (srcStride == stride) {
			memcpy(pixels, srcData, stride * window->base.size[1]);
		} else {
			fprintf(stderr, "Scaling labels is not supported, sorry!\n");
			int s = srcStride;
			int i=0;
			if (stride < s) s = stride;
			for (i=0; i<window->base.size[1]; i++) {
				memcpy(pixels + i * stride, srcData + i * srcStride, s);
			}
		}
	}
	tco_win_post(&window->base, buffer);
	int visible = 0;
	int rc = screen_set_window_property_iv(window->base.window, SCREEN_PROPERTY_VISIBLE, &visible);
}

void tco_label_window_showAt(tco_label_window_t *window, screen_window_t parent, int x, int y)
{
	int rc = 0;

	if (parent && parent != window->base.parent) {
		int parentBufferSize[2];
		int parentSize[2];
		rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_POSITION, window->offset);
		rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_BUFFER_SIZE, parentBufferSize);
		rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_SIZE, parentSize);
		window->scale[0] = parentSize[0] / (float)parentBufferSize[0];
		window->scale[1] = parentSize[1] / (float)parentBufferSize[1];
		int newSize[] = {window->base.size[0] * window->scale[0], window->base.size[1] * window->scale[1]};
		rc = screen_set_window_property_iv(window->base.window, SCREEN_PROPERTY_SIZE, newSize);
	}

	if (!tco_win_setParent(&window->base, parent))
		return;

	tco_label_window_move(window, x, y);

	int visible = 1;
	rc = screen_set_window_property_iv(window->base.window, SCREEN_PROPERTY_VISIBLE, &visible);
	if (rc) {
		perror("set label window visible: ");
	}
}

void tco_label_window_move(tco_label_window_t *window, int x, int y)
{
	int position[] = {window->offset[0] + (x * window->scale[0]), window->offset[1] + (y * window->scale[1])};
	int rc = screen_set_window_property_iv(window->base.window, SCREEN_PROPERTY_POSITION, position);
	if (rc) {
		perror("LabelWindow set position: ");
		return;
	}
}
