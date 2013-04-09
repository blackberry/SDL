/*
 * tco_dispatcher.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_DISPATCHER_H_
#define TCO_DISPATCHER_H_

#include "tco_structs.h"
#include "tco_common.h"

tco_event_dispatcher_t *tco_dispatcher_tap_create(HandleTapFunc func);

tco_event_dispatcher_t *tco_dispatcher_keyevent_create(HandleKeyFunc func,
		int sym, int mod, int scancode, uint16_t unicode);
tco_event_dispatcher_t *tco_dispatcher_dpad_create(HandleDPadFunc func);
tco_event_dispatcher_t *tco_dispatcher_toucharea_create(HandleTouchFunc func);
tco_event_dispatcher_t *tco_dispatcher_mouseevent_create(HandleMouseButtonFunc func,
		int mask, int button);
tco_event_dispatcher_t *tco_dispatcher_touchscreen_create(HandleTouchScreenFunc func);

#endif /* TCO_DISPATCHER_H_ */
