/*
 * tco_label.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_LABEL_H_
#define TCO_LABEL_H_

#include "tco_structs.h"

tco_label_t *tco_label_create(screen_context_t screenContext, int x, int y, unsigned w, unsigned h, const char *childImage);
void tco_label_destroy(tco_label_t *label);
void tco_label_draw(tco_label_t *label, screen_window_t window, int x, int y);
void tco_label_move(tco_label_t *label, int x, int y);

#endif /* TCO_LABEL_H_ */
