/*
 * tco_img.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_img.h"

#include <img/img.h>

#include <string.h>

static int decode_setup(uintptr_t data, img_t *img, unsigned flags)
{
	screen_pixmap_t pixmap = (screen_pixmap_t )data;
	screen_buffer_t buffer;
	int size[2];

	size[0] = img->w;
	size[1] = img->h;
	screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
	screen_create_pixmap_buffer(pixmap);

	screen_get_pixmap_property_pv(pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&buffer);
	screen_get_buffer_property_pv(buffer, SCREEN_PROPERTY_POINTER, (void **)&img->access.direct.data);
	screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_STRIDE, (int*)&img->access.direct.stride);
	img->flags |= IMG_DIRECT;
	return IMG_ERR_OK;
}

static void decode_abort(uintptr_t data, img_t *img)
{
	screen_pixmap_t pixmap = (screen_pixmap_t)data;
	screen_destroy_pixmap_buffer(pixmap);
}

int tco_img_loadfile(const char *filename, unsigned *width, unsigned *height, screen_pixmap_t pixmap, screen_buffer_t *buffer)
{
	img_decode_callouts_t callouts;
	img_lib_t ilib = 0;
	img_t img;
	int rc;

	rc = img_lib_attach(&ilib);
	if (rc != IMG_ERR_OK) {
		return 0;
	}

	memset(&img, 0, sizeof(img));
	img.flags |= IMG_FORMAT;
	img.format = IMG_FMT_PKLE_ABGR8888;

	memset(&callouts, 0, sizeof(callouts));
	callouts.setup_f = decode_setup;
	callouts.abort_f = decode_abort;
	callouts.data = (uintptr_t)pixmap;
	rc = img_load_file(ilib, filename, &callouts, &img);
	*width = img.w;
	*height = img.h;
	img_lib_detach(ilib);
	return (rc == IMG_ERR_OK);
}
