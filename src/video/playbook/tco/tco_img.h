/*
 * tco_img.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_IMG_H_
#define TCO_IMG_H_

#include <screen/screen.h>

int tco_img_loadfile(const char *filename, unsigned *width, unsigned *height, screen_pixmap_t pixmap, screen_buffer_t *buffer);

#endif /* TCO_IMG_H_ */
