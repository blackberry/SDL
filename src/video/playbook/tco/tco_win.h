/*
 * tco_win.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_WIN_H_
#define TCO_WIN_H_

#include "tco_structs.h"
#include <screen/screen.h>

int tco_win_create(tco_window_t *window, screen_context_t screenContext, screen_window_t parent);
int tco_win_create2(tco_window_t *window, screen_context_t screenContext, int width, int height, screen_window_t parent);
void tco_win_destroy(tco_window_t *window);

int tco_win_setZOrder(tco_window_t *window, int zOrder);
int tco_win_setSensitivity(tco_window_t *window, int isSensitive);
int tco_win_pixels(tco_window_t *window, screen_buffer_t *buffer, unsigned char **pixels, int *stride);
void tco_win_post(tco_window_t *window, screen_buffer_t buffer);

int tco_win_setParent(tco_window_t *window, screen_window_t parent);
#endif /* TCO_WIN_H_ */
