/*
 * tco_dispatcher.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_dispatcher.h"
#include "tco_structs.h"

#include <stdlib.h>
static int run_tapevent(struct _tco_event_dispatcher_t *dispatcher, void *event)
{
	(void)event;
	return dispatcher->callback.handleTapFunc();
}

tco_event_dispatcher_t *tco_dispatcher_tap_create(HandleTapFunc func)
{
	tco_event_dispatcher_t *dispatcher = (tco_event_dispatcher_t *)malloc(sizeof(*dispatcher));
	dispatcher->callback.handleTapFunc = (void *)func;
	dispatcher->runCallback = run_tapevent;
	return dispatcher;
}

static int run_keyevent(struct _tco_event_dispatcher_t *dispatcher, void *params)
{
	TCOKeyEvent *event = (TCOKeyEvent *)params;
	return dispatcher->callback.handleKeyFunc(dispatcher->key.sym, dispatcher->key.mod,
			dispatcher->key.scancode, dispatcher->key.unicode, *event);
}

tco_event_dispatcher_t *tco_dispatcher_keyevent_create(HandleKeyFunc func,
		int sym, int mod, int scancode, uint16_t unicode)
{
	tco_event_dispatcher_t *dispatcher = (tco_event_dispatcher_t *)malloc(sizeof(*dispatcher));
	dispatcher->callback.handleKeyFunc = (void *)func;
	dispatcher->key.sym = sym;
	dispatcher->key.mod = mod;
	dispatcher->key.scancode = scancode;
	dispatcher->key.unicode = unicode;
	dispatcher->runCallback = run_keyevent;
	return dispatcher;
}

static int run_dpad(struct _tco_event_dispatcher_t *dispatcher, void *params)
{
	TCODPadEvent *event = (TCODPadEvent *)params;
	return dispatcher->callback.handleDPadFunc(event->angle, event->event);
}

tco_event_dispatcher_t *tco_dispatcher_dpad_create(HandleDPadFunc func)
{
	tco_event_dispatcher_t *dispatcher = (tco_event_dispatcher_t *)malloc(sizeof(*dispatcher));
	dispatcher->callback.handleDPadFunc = (void *)func;
	dispatcher->runCallback = run_dpad;
	return dispatcher;
}

static int run_toucharea(struct _tco_event_dispatcher_t *dispatcher, void *params)
{
	TCOTouchAreaEvent *event = (TCOTouchAreaEvent *)params;
	return dispatcher->callback.handleTouchFunc(event->dx, event->dy);
}

tco_event_dispatcher_t *tco_dispatcher_toucharea_create(HandleTouchFunc func)
{
	tco_event_dispatcher_t *dispatcher = (tco_event_dispatcher_t *)malloc(sizeof(*dispatcher));
	dispatcher->callback.handleTouchFunc = (void *)func;
	dispatcher->runCallback = run_toucharea;
	return dispatcher;
}

static int run_mouseevent(struct _tco_event_dispatcher_t *dispatcher, void *params)
{
	TCOMouseButtonEvent *event = (TCOMouseButtonEvent *)params;
	return dispatcher->callback.handleMouseButtonFunc(dispatcher->mouse.mask,
			dispatcher->mouse.button, *event);
}

tco_event_dispatcher_t *tco_dispatcher_mouseevent_create(HandleMouseButtonFunc func,
		int mask, int button)
{
	tco_event_dispatcher_t *dispatcher = (tco_event_dispatcher_t *)malloc(sizeof(*dispatcher));
	dispatcher->callback.handleKeyFunc = (void *)func;
	dispatcher->mouse.mask = mask;
	dispatcher->mouse.button = button;
	dispatcher->runCallback = run_mouseevent;
	return dispatcher;
}

static int run_touchscreen(struct _tco_event_dispatcher_t *dispatcher, void *params)
{
	TCOTouchScreenEvent *event = (TCOTouchScreenEvent *)params;
	return dispatcher->callback.handleTouchScreenFunc(event->x, event->y, event->tap, event->hold);
}

tco_event_dispatcher_t *tco_dispatcher_touchscreen_create(HandleTouchScreenFunc func)
{
	tco_event_dispatcher_t *dispatcher = (tco_event_dispatcher_t *)malloc(sizeof(*dispatcher));
	dispatcher->callback.handleTouchScreenFunc = (void *)func;
	dispatcher->runCallback = run_touchscreen;
	return dispatcher;
}
