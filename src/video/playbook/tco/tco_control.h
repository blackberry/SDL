/*
 * tco_control.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_CONTROL_H_
#define TCO_CONTROL_H_

#include "tco_structs.h"

tco_control_t *tco_control_create(screen_context_t context, TCOControlType type,
		int x, int y, unsigned width, unsigned height,
		tco_event_dispatcher_t *dispatcher, tco_event_dispatcher_t *tapDispatcher);
void tco_control_destroy(tco_control_t *control);
void tco_control_fill(tco_control_t *control);
int tco_control_loadImage(tco_control_t *control, const char *filename);

void tco_control_draw(tco_control_t *control, screen_buffer_t buffer);
int tco_control_handleTouch(tco_control_t *control, int type, int contactId, const int pos[], long long timestamp);
int tco_control_handleTap(tco_control_t *control, int contactId, const int pos[]);
int tco_control_handlesTap(tco_control_t *control );
int tco_control_inBounds(tco_control_t *control, const int pos[]);
void tco_control_move(tco_control_t *control, int dx, int dy, unsigned maxDimensions[]);

void tco_control_showLabel(tco_control_t *control, screen_window_t window);
void tco_control_addLabel(tco_control_t *control, tco_label_t *label);
#endif /* TCO_CONTROL_H_ */
