/*
 * tco_structs.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_STRUCTS_H_
#define TCO_STRUCTS_H_

#include <screen/screen.h>
#include "tco_common.h"

#define MAX_TOUCHPOINTS 16

typedef enum _TCOControlType {
	KEY,              // Used to provide keyboard input
	DPAD,             // Provides angle and magnitude from center (0 east, 90 north, 180 west, 270 south)
	TOUCHAREA,        // Used to provide relative mouse motion
	MOUSEBUTTON,      // Used to provide mouse button state
	TOUCHSCREEN		  // Provides: mouse move, left click tap and right click tap-hold
} TCOControlType;

typedef enum _TCOKeyEvent {
	KEY_DOWN = TCO_KB_DOWN,
	KEY_UP = TCO_KB_UP
} TCOKeyEvent;

typedef struct _TCODPadEvent {
	int angle;
	enum {
		DPAD_DOWN = TCO_KB_DOWN,
		DPAD_UP = TCO_KB_UP
	} event;
} TCODPadEvent;

typedef struct _TCOTouchAreaEvent {
	int dx;
	int dy;
} TCOTouchAreaEvent;

typedef enum _TCOMouseButtonEvent {
	MOUSE_DOWN = TCO_MOUSE_BUTTON_DOWN,
	MOUSE_UP = TCO_MOUSE_BUTTON_UP
} TCOMouseButtonEvent;

typedef struct _TCOTouchScreenEvent {
	int x;
	int y;
	int tap;
	int hold;
} TCOTouchScreenEvent;

struct _tco_control_t;
struct _tco_label_window_t;
struct _tco_label_t;

typedef struct _tco_label_t
{
	int x,y;
	unsigned width,height;
	struct _tco_control_t *control;
	struct _tco_label_window_t *window;
	struct _tco_label_t *next;
} tco_label_t;

typedef union _tco_event_data_t
{
	TCOKeyEvent keyEvent;

} tco_event_data_t;

typedef struct _tco_event_dispatcher_t
{
	int (*runCallback)(struct _tco_event_dispatcher_t *dispatcher, void *params);
	union {
		HandleKeyFunc handleKeyFunc;
		HandleDPadFunc handleDPadFunc;
		HandleTouchFunc handleTouchFunc;
		HandleMouseButtonFunc handleMouseButtonFunc;
		HandleTapFunc handleTapFunc;
		HandleTouchScreenFunc handleTouchScreenFunc;
	} callback;

	union {
		struct {
			int sym;
			int mod;
			int scancode;
			uint16_t unicode;
		} key;
		struct {
			int mask;
			int button;
		} mouse;
	};
} tco_event_dispatcher_t;

typedef struct _tco_control_t
{
	TCOControlType type;
	int x,y;
	unsigned width,height,srcWidth,srcHeight;
	tco_event_dispatcher_t *dispatcher;
	tco_event_dispatcher_t *tapDispatcher;

	screen_context_t context;
	screen_pixmap_t pixmap;
	screen_buffer_t buffer;

	int contactId;

	// Touch areas
	int lastPos[2];
	long long touchDownTime;

	// Touch screens
	int startPos[2];
	long long touchScreenStartTime;
	int touchScreenInMoveEvent;
	int touchScreenInHoldEvent;

	tco_label_t *labels;
	struct _tco_control_t *next;
} tco_control_t;

typedef struct _tco_window_t
{
	int valid;
	screen_context_t context;
	screen_window_t window;
	screen_window_t parent;
	int size[2];
} tco_window_t;

typedef struct _tco_label_window_t
{
	tco_window_t base;
	int offset[2];
	float scale[2];
} tco_label_window_t;

typedef struct _tco_config_window_t
{
	tco_window_t base;
	tco_control_t *selected;
} tco_config_window_t;

typedef struct _tco_context
{
	screen_context_t screenContext;
	screen_window_t appWindow;
	tco_config_window_t *configWindow;

	tco_control_t *controls;
	tco_control_t *controlMap[MAX_TOUCHPOINTS];

	tco_callbacks callbacks;
} tco_context,*tco_context_t;

#endif /* TCO_STRUCTS_H_ */
