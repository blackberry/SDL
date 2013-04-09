/*
 * tco_control.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_control.h"
#include "tco_label.h"
#include "tco_img.h"

#include <stdlib.h>
#include <math.h>

const static int TAP_THRESHOLD = 150000000L;
const static int JITTER_THRESHOLD = 10;

tco_control_t *tco_control_create(screen_context_t context, TCOControlType type,
		int x, int y, unsigned width, unsigned height,
		tco_event_dispatcher_t *dispatcher, tco_event_dispatcher_t *tapDispatcher)
{
	tco_control_t *control = (tco_control_t *)malloc(sizeof(tco_control_t));
	if (!control) return 0;

	control->type = type;
	control->x = x;
	control->y = y;
	control->width = width;
	control->height = height;
	control->srcWidth = width;
	control->srcHeight = height;
	control->dispatcher = dispatcher;
	control->tapDispatcher = tapDispatcher;

	control->context = context;
	control->pixmap = 0;
	control->buffer = 0;

	control->contactId = -1;

	control->lastPos[0] = 0;
	control->lastPos[1] = 0;
	control->touchDownTime = 0;

	control->startPos[0] = 0;
	control->startPos[1] = 0;
	control->touchScreenStartTime = 0;
	control->touchScreenInMoveEvent = 0;
	control->touchScreenInHoldEvent = 0;

	control->labels = 0;
	control->next = 0;
	return control;
}

void tco_control_destroy(tco_control_t *control)
{
	if (!control) return;

	tco_label_t *label = control->labels;
	tco_label_t *oldLabel = 0;
	while (label) {
		oldLabel = label;
		label = label->next;
		tco_label_destroy(oldLabel);
	}
	screen_destroy_pixmap(control->pixmap);
	free(control->dispatcher);
	free(control->tapDispatcher);
	free(control);
}

void tco_control_fill(tco_control_t *control)
{
	static unsigned controlNum = 0;
	static uint32_t controlColors[] = { 0xaaff0000, 0xaa00ff00, 0xaa0000ff, 0xaaffff00, 0xaaff00ff, 0xaa00ffff };

	int format = SCREEN_FORMAT_RGBA8888;
	int size[2] = {control->width, control->height};
	unsigned char *pixels;
	int stride;

	int rc = screen_create_pixmap(&control->pixmap, control->context); // FIXME: Check failure
	rc = screen_set_pixmap_property_iv(control->pixmap, SCREEN_PROPERTY_FORMAT, &format);
	rc = screen_set_pixmap_property_iv(control->pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
	rc = screen_create_pixmap_buffer(control->pixmap);
	rc = screen_get_pixmap_property_pv(control->pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&control->buffer);
//	rc = screen_get_buffer_property_pv(control->buffer, SCREEN_PROPERTY_POINTER, (void **)&pixels);
//	rc = screen_get_buffer_property_iv(control->buffer, SCREEN_PROPERTY_STRIDE, &stride);
	int attribs[] = {
		SCREEN_BLIT_COLOR, (int)controlColors[controlNum],
		SCREEN_BLIT_END
	};
	rc = screen_fill(control->context, control->buffer, attribs);
	controlNum++;
	if (controlNum > 5)
		controlNum = 0;
}

int tco_control_loadImage(tco_control_t *control, const char *filename)
{
	screen_create_pixmap(&control->pixmap, control->context);
	int ret = tco_img_loadfile(filename, &control->srcWidth, &control->srcHeight, control->pixmap, &control->buffer);
	if (!ret) {
		screen_destroy_pixmap(control->pixmap);
		control->pixmap = 0;
	}
	return ret;
}

int tco_control_handleTouch(tco_control_t *control, int type, int contactId, const int pos[], long long timestamp)
{
	if (control->contactId != -1 && control->contactId != contactId) {
		// We have a contact point set and this isn't it.
		return 0;
	}

	if (control->contactId == -1) {
		// Don't handle orphaned release events.
		if (type == SCREEN_EVENT_MTOUCH_RELEASE)
			return 0;
		if (!tco_control_inBounds(control, pos))
			return 0;

		// This is a new touch point that we should start handling
		control->contactId = contactId;

		switch (control->type)
		{
		case KEY:
			{
				TCOKeyEvent event = KEY_DOWN;
				control->dispatcher->runCallback(control->dispatcher, &event);
			}
			break;
		case DPAD:
			{
				TCODPadEvent event;
				event.angle = atan2((pos[1] - control->y - control->height / 2.0f), (pos[0] - control->x - control->width / 2.0f)) * 180 / M_PI;
				event.event = DPAD_DOWN;
				control->dispatcher->runCallback(control->dispatcher, &event);
			}
			break;
		case TOUCHAREA:
			control->touchDownTime = timestamp;
			control->lastPos[0] = pos[0];
			control->lastPos[1] = pos[1];
			break;
		case MOUSEBUTTON:
			{
				TCOMouseButtonEvent event = MOUSE_DOWN;
				control->dispatcher->runCallback(control->dispatcher, &event);
			}
			break;
		case TOUCHSCREEN:
			control->startPos[0] = pos[0];
			control->startPos[1] = pos[1];
			control->touchScreenStartTime = timestamp;
			break;
		default:
			break;
		}
	} else {
		if (!tco_control_inBounds(control, pos)) {
			// Act as if we received a key up
			switch (control->type)
			{
			case KEY:
				{
					TCOKeyEvent event = KEY_UP;
					control->dispatcher->runCallback(control->dispatcher, &event);
				}
				break;
			case DPAD:
				{
					TCODPadEvent event;
					event.angle = atan2((pos[1] - control->y - control->height / 2.0f), (pos[0] - control->x - control->width / 2.0f)) * 180 / M_PI;
					event.event = DPAD_UP;
					control->dispatcher->runCallback(control->dispatcher, &event);
				}
				break;
			case TOUCHAREA:
				{
					TCOTouchAreaEvent event;
					event.dx = pos[0] - control->lastPos[0];
					event.dy = pos[1] - control->lastPos[1];
					if (event.dx != 0 || event.dy != 0) {
						control->dispatcher->runCallback(control->dispatcher, &event);
						control->lastPos[0] = pos[0];
						control->lastPos[1] = pos[1];
					}
				}
				break;
			case MOUSEBUTTON:
				{
					TCOMouseButtonEvent event = MOUSE_UP;
					control->dispatcher->runCallback(control->dispatcher, &event);
				}
				break;
			case TOUCHSCREEN:
				control->touchScreenInHoldEvent = 0;
				control->touchScreenInMoveEvent = 0;
				break;
			default:
				break;
			}
			control->contactId = -1;
			return 0;
		}

		// We have had a previous touch point from this contact and this point is in bounds
		switch (control->type)
		{
		case KEY:
			if (type == SCREEN_EVENT_MTOUCH_RELEASE)
			{
				TCOKeyEvent event = KEY_UP;
				control->dispatcher->runCallback(control->dispatcher, &event);
			}
			break;
		case DPAD:
			{
				TCODPadEvent event;
				event.angle = atan2((pos[1] - control->y - control->height / 2.0f), (pos[0] - control->x - control->width / 2.0f)) * 180 / M_PI;
				if (type == SCREEN_EVENT_MTOUCH_RELEASE)
					event.event = DPAD_UP;
				else
					event.event = DPAD_DOWN;
				control->dispatcher->runCallback(control->dispatcher, &event);
			}
			break;
		case TOUCHAREA:
			{
				if (control->tapDispatcher && type == SCREEN_EVENT_MTOUCH_RELEASE
						&& (timestamp - control->touchDownTime) < TAP_THRESHOLD) {
					control->tapDispatcher->runCallback(control->tapDispatcher, 0);
				} else {
					if (type == SCREEN_EVENT_MTOUCH_TOUCH)
						control->touchDownTime = timestamp;

					TCOTouchAreaEvent event;
					event.dx = pos[0] - control->lastPos[0];
					event.dy = pos[1] - control->lastPos[1];
					if (event.dx != 0 || event.dy != 0) {
						control->dispatcher->runCallback(control->dispatcher, &event);
						control->lastPos[0] = pos[0];
						control->lastPos[1] = pos[1];
					}
				}
			}
			break;
		case MOUSEBUTTON:
			if (type == SCREEN_EVENT_MTOUCH_RELEASE)
			{
				TCOMouseButtonEvent event = MOUSE_UP;
				control->dispatcher->runCallback(control->dispatcher, &event);
			}
			break;
		case TOUCHSCREEN:
			{
				TCOTouchScreenEvent event;
				event.x = pos[0];
				event.y = pos[1];
				event.tap = 0;
				event.hold = 0;
				int distance = abs(pos[0] - control->startPos[0]) + abs(pos[1] - control->startPos[1]);
				if (!control->touchScreenInHoldEvent) {
					if ((type == SCREEN_EVENT_MTOUCH_RELEASE)
							&& (timestamp - control->touchScreenStartTime) < TAP_THRESHOLD
							&& distance < JITTER_THRESHOLD) {
						event.tap = 1;
						control->dispatcher->runCallback(control->dispatcher, &event);
					} else if ((type == SCREEN_EVENT_MTOUCH_MOVE)
							&& (control->touchScreenInMoveEvent
									|| (distance > JITTER_THRESHOLD))) {
						control->touchScreenInMoveEvent = 1;
						control->dispatcher->runCallback(control->dispatcher, &event);
					} else if ((type == SCREEN_EVENT_MTOUCH_MOVE)
							&& (!control->touchScreenInMoveEvent)
							&& (timestamp - control->touchScreenStartTime) > 2*TAP_THRESHOLD) {
						event.hold = 1;
						control->touchScreenInHoldEvent = 1;
						control->dispatcher->runCallback(control->dispatcher, &event);
					}
				}
			}
			break;
		default:
			break;
		}

		if (type == SCREEN_EVENT_MTOUCH_RELEASE) {
			//fprintf(stderr, "Update touch - release\n");
			control->contactId = -1;
			control->touchScreenInHoldEvent = 0;
			control->touchScreenInMoveEvent = 0;
			return 0;
		}
	}

	return 1;
}

int tco_control_handleTap(tco_control_t *control, int contactId, const int pos[])
{
	if (!control->tapDispatcher)
		return 0;

	if (control->contactId != -1) {
		// We have a contact point set already. No taps allowed.
		return 0;
	}
	if (!tco_control_inBounds(control, pos))
		return 0;

	return control->tapDispatcher->runCallback(control->tapDispatcher, 0);
}

int tco_control_handlesTap(tco_control_t *control )
{
	return control->tapDispatcher != 0;
}

void tco_control_showLabel(tco_control_t *control, screen_window_t window)
{
	tco_label_t *label = control->labels;
	while (label) {
		tco_label_draw(label, window, control->x, control->y);
		label = label->next;
	}
}

void tco_control_addLabel(tco_control_t *control, tco_label_t *label)
{
	label->next = control->labels;
	control->labels = label;
	label->control = control;
}

void tco_control_draw(tco_control_t *control, screen_buffer_t buffer)
{
	screen_get_pixmap_property_pv(control->pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&control->buffer);
	int attribs[] = {
			SCREEN_BLIT_SOURCE_X, 0,
			SCREEN_BLIT_SOURCE_Y, 0,
			SCREEN_BLIT_SOURCE_WIDTH, control->srcWidth,
			SCREEN_BLIT_SOURCE_HEIGHT, control->srcHeight,
			SCREEN_BLIT_DESTINATION_X, control->x,
			SCREEN_BLIT_DESTINATION_Y, control->y,
			SCREEN_BLIT_DESTINATION_WIDTH, control->width,
			SCREEN_BLIT_DESTINATION_HEIGHT, control->height,
			SCREEN_BLIT_TRANSPARENCY, SCREEN_TRANSPARENCY_NONE,
			SCREEN_BLIT_GLOBAL_ALPHA, 192,
			SCREEN_BLIT_END
	};
	screen_blit(control->context, buffer, control->buffer, attribs);
}

int tco_control_inBounds(tco_control_t *control, const int pos[])
{
	return (pos[0] >= control->x && pos[0] <= (int)(control->x + control->width) &&
			pos[1] >= control->y && pos[1] <= (int)(control->y + control->height));
}

void tco_control_move(tco_control_t *control, int dx, int dy, unsigned maxDimensions[])
{
	control->x += dx;
	control->y += dy;
	if (control->x <= 0)
		control->x = 0;
	if (control->y <= 0)
		control->y = 0;
	if (control->x + control->width >= maxDimensions[0])
		control->x = maxDimensions[0] - control->width;
	if (control->y + control->height >= maxDimensions[1])
		control->y = maxDimensions[1] - control->height;
	tco_label_t *label = control->labels;
	while (label) {
		tco_label_move(label, control->x, control->y);
		label = label->next;
	}
}
