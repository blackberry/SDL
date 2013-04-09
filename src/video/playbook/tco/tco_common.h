/*
 * tco_common.h
 *
 *  Created on: Apr 7, 2013
 *      Author: Jeremy
 */

#ifndef TCO_COMMON_H_
#define TCO_COMMON_H_

#include <sys/platform.h>
#include <screen/screen.h>

/**
 * tco library functions that may fail
 * return \c TCO_SUCCESS on success.
 */
#define TCO_SUCCESS (0)

/**
 * tco library functions that may fail return
 * \c TCO_FAILURE on error and
 * may set errno.
 */
#define TCO_FAILURE (-1)

#define TCO_UNHANDLED (2)

/**
 * File version number
 */
#define TCO_FILE_VERSION 1

typedef int(*HandleKeyFunc)(int sym, int mod, int scancode, uint16_t unicode, int event);
typedef int(*HandleDPadFunc)(int angle, int event);
typedef int(*HandleTouchFunc)(int dx, int dy);
typedef int(*HandleMouseButtonFunc)(int button, int mask, int event);
typedef int(*HandleTapFunc)();
typedef int(*HandleTouchScreenFunc)(int x, int y, int tap, int hold);

typedef struct _tco_callbacks {
	HandleKeyFunc handleKeyFunc;
	HandleDPadFunc handleDPadFunc;
	HandleTouchFunc handleTouchFunc;
	HandleMouseButtonFunc handleMouseButtonFunc; // TODO: Unify keyboard mod with mouse mask
	HandleTapFunc handleTapFunc;
	HandleTouchScreenFunc handleTouchScreenFunc;
} tco_callbacks;

enum EmuKeyButtonState {
	TCO_KB_DOWN = 0,
	TCO_KB_UP = 1
};

enum EmuMouseButton {
	TCO_MOUSE_LEFT_BUTTON = 0,
	TCO_MOUSE_RIGHT_BUTTON = 1,
	TCO_MOUSE_MIDDLE_BUTTON = 2
};

enum EmuKeyMask {
	TCO_SHIFT = 0x1,
	TCO_CTRL = 0x2,
	TCO_ALT = 0x4
};

enum EmuMouseButtonState {
	TCO_MOUSE_BUTTON_DOWN = 0,
	TCO_MOUSE_BUTTON_UP = 1
};

#endif /* TCO_COMMON_H_ */
