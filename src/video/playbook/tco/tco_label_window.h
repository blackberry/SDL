/*
 * tco_labelwindow.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_LABELWINDOW_H_
#define TCO_LABELWINDOW_H_

#include "tco_structs.h"

tco_label_window_t *tco_label_window_create(screen_context_t context, int width, int height);
void tco_label_window_destroy(tco_label_window_t *window);
void tco_label_window_draw(tco_label_window_t *window, screen_pixmap_t pixmap, int width, int height);
void tco_label_window_showAt(tco_label_window_t *window, screen_window_t parent, int x, int y);
void tco_label_window_move(tco_label_window_t *window, int x, int y);

#endif /* TCO_LABELWINDOW_H_ */
