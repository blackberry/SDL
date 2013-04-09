/*
 * tco_win.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_win.h"

#include <string.h>
#include <errno.h>

static void tco_win_init(tco_window_t *window, screen_window_t parent)
{
	int rc;
	int format = SCREEN_FORMAT_RGBA8888;
	int usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE;
	int temp[2];

	rc = screen_create_window_type(&window->window, window->context, SCREEN_CHILD_WINDOW);
	if (rc) {
		perror("screen_create_window");
		return;
	}

	rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_FORMAT, &format);
	if (rc) {
		perror("screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT)");
		screen_destroy_window(window->window);
		window->window = 0;
		return;
	}

	rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_USAGE, &usage);
	if (rc) {
		perror("screen_set_window_property_iv(SCREEN_PROPERTY_USAGE)");
		screen_destroy_window(window->window);
		window->window = 0;
		return;
	}

	if (parent) {
		rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_SIZE, temp);
		if (rc) {
			perror("screen_get_window_property_iv(SCREEN_PROPERTY_SIZE)");
			screen_destroy_window(window->window);
			window->window = 0;
			return;
		}

		rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_SIZE, temp);
		if (rc) {
			perror("screen_set_window_property_iv(SCREEN_PROPERTY_SIZE)");
			screen_destroy_window(window->window);
			window->window = 0;
			return;
		}

		rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_POSITION, temp);
		if (rc) {
			perror("screen_get_window_property_iv(SCREEN_PROPERTY_POSITION)");
			screen_destroy_window(window->window);
			window->window = 0;
			return;
		}

		rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_POSITION, temp);
		if (rc) {
			perror("screen_set_window_property_iv(SCREEN_PROPERTY_POSITION)");
			screen_destroy_window(window->window);
			window->window = 0;
			return;
		}
	} else {
		rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_SIZE, window->size);
		if (rc) {
			perror("screen_set_window_property_iv(SCREEN_PROPERTY_SIZE)");
			screen_destroy_window(window->window);
			window->window = 0;
			return;
		}
	}

	rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_BUFFER_SIZE, window->size);
	if (rc) {
		perror("screen_set_window_property_iv(SCREEN_PROPERTY_BUFFER_SIZE)");
		screen_destroy_window(window->window);
		window->window = 0;
		return;
	}

	rc = screen_create_window_buffers(window->window, 2);
	if (rc) {
		perror("screen_create_window_buffers");
		screen_destroy_window(window->window);
		window->window = 0;
		return;
	}

	if (!tco_win_setParent(window, parent))
	{
		screen_destroy_window(window->window);
		window->window = 0;
		return;
	}

	window->valid = 1;
}

int tco_win_create(tco_window_t *window, screen_context_t screenContext, screen_window_t parent)
{
	window->context = screenContext;
	window->parent = 0;
	window->valid = 0;
	window->window = 0;
	int size[2] = {0,0};
	int rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_BUFFER_SIZE, size);
	if (rc) {
		perror("screen_get_window_property_iv(size)");
		return 0;
	}

	window->size[0] = size[0];
	window->size[1] = size[1];

	tco_win_init(window, parent);
	return 1;
}

int tco_win_create2(tco_window_t *window, screen_context_t screenContext, int width, int height, screen_window_t parent)
{
	window->context = screenContext;
	window->parent = 0;
	window->valid = 0;
	window->window = 0;
	window->size[0] = width;
	window->size[1] = height;
	tco_win_init(window, parent);
	return 1;
}

void tco_win_destroy(tco_window_t *window)
{
	if (window && window->window) {
		screen_destroy_window(window->window);
		window->window = 0;
	}
}

int tco_win_setZOrder(tco_window_t *window, int zOrder)
{
	int rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_ZORDER, &zOrder);
	if (rc) {
		fprintf(stderr, "Cannot set z-order: %s", strerror(errno));
		return 0;
	}
	return 1;
}

int tco_win_setSensitivity(tco_window_t *window, int isSensitive)
{
	int sensitivity = (isSensitive)?SCREEN_SENSITIVITY_ALWAYS:SCREEN_SENSITIVITY_NEVER;
	int rc = screen_set_window_property_iv(window->window, SCREEN_PROPERTY_SENSITIVITY, &sensitivity);
	if (rc) {
		fprintf(stderr, "Cannot set screen sensitivity: %s", strerror(errno));
		return 0;
	}
	return 1;
}

int tco_win_pixels(tco_window_t *window, screen_buffer_t *buffer, unsigned char **pixels, int *stride)
{
	if (!window->valid)
		return 0;

	screen_buffer_t buffers[2];
	int rc = screen_get_window_property_pv(window->window,
			SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buffers);
	if (rc) {
		fprintf(stderr, "Cannot get window render buffers: %s", strerror(errno));
		return 0;
	}
	*buffer = buffers[0];

	rc = screen_get_buffer_property_pv(*buffer, SCREEN_PROPERTY_POINTER, (void **)pixels);
	if (rc) {
		fprintf(stderr, "Cannot get buffer pointer: %s", strerror(errno));
		return 0;
	}

	if (!*pixels) {
		fprintf(stderr, "Window buffer has no accessible pixels\n");
		return 0;
	}

	rc = screen_get_buffer_property_iv(*buffer, SCREEN_PROPERTY_STRIDE, stride);
	if (rc) {
		fprintf(stderr, "Cannot get stride: %s", strerror(errno));
		return 0;
	}

	return 1;
}

void tco_win_post(tco_window_t *window, screen_buffer_t buffer)
{
	int dirtyRects[4] = {0, 0, window->size[0], window->size[1]};
	screen_post_window(window->window, buffer, 1, dirtyRects, 0);
}

int tco_win_setParent(tco_window_t *window, screen_window_t parent)
{
	int rc;
	if (parent == window->parent)
		return 1;

	if (parent != 0) {
		char buffer[256] = {0};

		rc = screen_get_window_property_cv(parent, SCREEN_PROPERTY_GROUP, 256, buffer);
		if (rc) {
			perror("screen_get_window_property_cv(SCREEN_PROPERTY_GROUP)");
			return 0;
		}

		rc = screen_join_window_group(window->window, buffer);
		if (rc) {
			perror("screen_join_window_group");
			return 0;
		}
		window->parent = parent;
	} else if (window->parent) {
		rc = screen_leave_window_group(window->window);
		if (rc) {
			perror("screen_leave_window_group");
			return 0;
		}
		window->parent = 0;
	}
	return 1;
}

