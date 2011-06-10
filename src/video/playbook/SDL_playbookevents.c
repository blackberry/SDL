/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"
#include "SDL.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_keysym.h"

#include "SDL_playbookvideo.h"
#include "SDL_playbookevents_c.h"

static SDL_keysym Playbook_Keycodes[256];

struct TouchEvent {
	int pending;
	int touching;
	int pos[2];
};

static void handlePointerEvent(screen_event_t event, screen_window_t window)
{
	int buttonState = 0;
	screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttonState);

	int coords[2];
	screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, coords);

	int screen_coords[2];
	screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, screen_coords);

	int wheel_delta;
	screen_get_event_property_iv(event, SCREEN_PROPERTY_MOUSE_WHEEL, &wheel_delta);

	// FIXME: Pointer events have never been tested.
	static int lastButtonState = 0;
	if (lastButtonState == buttonState) {
		if (buttonState)
			SDL_PrivateMouseMotion(SDL_BUTTON_LEFT, 0, coords[0], coords[1]);
		else
			SDL_PrivateMouseMotion(0, 0, coords[0], coords[1]);
		return;
	}
	lastButtonState = buttonState;

	SDL_PrivateMouseButton(buttonState ? SDL_PRESSED : SDL_RELEASED, SDL_BUTTON_LEFT, coords[0], coords[1]); // FIXME: window
}

static void handleKeyboardEvent(screen_event_t event)
{
    int sym = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SYM, &sym);
    int modifiers = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_MODIFIERS, &modifiers);
    int flags = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_FLAGS, &flags);
    int scan = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SCAN, &scan);
    int cap = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_CAP, &cap);

    // FIXME: Keyboard handling (modifiers are currently ignored, some keys are as well)
    SDL_keysym keysym;
    if (sym < 256)
    	keysym.sym = Playbook_Keycodes[sym].sym;
    else if (sym == 61448)
    	keysym.sym = SDLK_BACKSPACE;
    else if (sym == 61453)
    	keysym.sym = SDLK_RETURN;

    int shifted = 0;
    // FIXME: These scancodes are really just implemented this way for dosbox.
    // See keyboard.cpp inside dosbox (KEYBOARD_AddKey) for a reference.
    switch (keysym.sym)
    {
    case SDLK_COLON:
    {
    	SDL_keysym temp;
    	temp.scancode = 42;
    	temp.sym = SDLK_LSHIFT;
    	SDL_PrivateKeyboard(SDL_PRESSED, &temp);
    	keysym.scancode = 39;
    	shifted = 1;
    	break;
    }
    case SDLK_1:
    	keysym.scancode = 2;
    	break;
    case SDLK_2:
		keysym.scancode = 3;
		break;
    case SDLK_3:
    	keysym.scancode = 4;
    	break;
    case SDLK_4:
		keysym.scancode = 5;
		break;
    case SDLK_5:
    	keysym.scancode = 6;
    	break;
    case SDLK_6:
		keysym.scancode = 7;
		break;
    case SDLK_7:
    	keysym.scancode = 8;
    	break;
    case SDLK_8:
		keysym.scancode = 9;
		break;
    case SDLK_9:
    	keysym.scancode = 10;
    	break;
    case SDLK_0:
		keysym.scancode = 11;
		break;
    case SDLK_MINUS:
    	keysym.scancode = 12;
    	break;
    case SDLK_EQUALS:
       	keysym.scancode = 13;
       	break;
    case SDLK_BACKSPACE:
       	keysym.scancode = 14;
       	break;
    case SDLK_TAB:
       	keysym.scancode = 15;
       	break;
    case SDLK_q:
		keysym.scancode = 16;
		break;
    case SDLK_w:
    	keysym.scancode = 17;
    	break;
    case SDLK_e:
		keysym.scancode = 18;
		break;
    case SDLK_r:
    	keysym.scancode = 19;
    	break;
    case SDLK_t:
		keysym.scancode = 20;
		break;
    case SDLK_y:
    	keysym.scancode = 21;
    	break;
    case SDLK_u:
		keysym.scancode = 22;
		break;
    case SDLK_i:
		keysym.scancode = 23;
		break;
    case SDLK_o:
    	keysym.scancode = 24;
    	break;
    case SDLK_p:
		keysym.scancode = 25;
		break;
    case SDLK_LEFTBRACKET:
    	keysym.scancode = 26;
    	break;
    case SDLK_RIGHTBRACKET:
		keysym.scancode = 27;
		break;
    case SDLK_RETURN:
  		keysym.scancode = 28;
  		break;
    case SDLK_a:
    	keysym.scancode = 30;
    	break;
    case SDLK_s:
		keysym.scancode = 31;
		break;
    case SDLK_d:
		keysym.scancode = 32;
		break;
    case SDLK_f:
    	keysym.scancode = 33;
    	break;
    case SDLK_g:
		keysym.scancode = 34;
		break;
    case SDLK_h:
    	keysym.scancode = 35;
    	break;
    case SDLK_j:
		keysym.scancode = 36;
		break;
    case SDLK_k:
    	keysym.scancode = 37;
    	break;
    case SDLK_l:
		keysym.scancode = 38;
		break;
    case SDLK_SEMICOLON:
		keysym.scancode = 39;
		break;
    case SDLK_QUOTE:
    	keysym.scancode = 40;
    	break;
    case SDLK_BACKQUOTE:
		keysym.scancode = 41;
		break;
    case SDLK_BACKSLASH:
    	keysym.scancode = 43;
    	break;
    case SDLK_z:
		keysym.scancode = 44;
		break;
    case SDLK_x:
    	keysym.scancode = 45;
    	break;
    case SDLK_c:
		keysym.scancode = 46;
		break;
    case SDLK_v:
		keysym.scancode = 47;
		break;
    case SDLK_b:
    	keysym.scancode = 48;
    	break;
    case SDLK_n:
		keysym.scancode = 49;
		break;
    case SDLK_m:
		keysym.scancode = 50;
		break;
    case SDLK_COMMA:
    	keysym.scancode = 51;
    	break;
    case SDLK_PERIOD:
		keysym.scancode = 52;
		break;
    case SDLK_SPACE:
		keysym.scancode = 57;
		break;
    }
    keysym.mod = KMOD_NONE;

    SDL_PrivateKeyboard((flags == 193)?SDL_PRESSED:SDL_RELEASED, &keysym);

    if (shifted) {
		SDL_keysym temp;
		temp.scancode = 42;
		temp.sym = SDLK_LSHIFT;
		SDL_PrivateKeyboard(SDL_RELEASED, &temp);
    }
}

static struct TouchEvent moveEvent;
static void handleMtouchEvent(screen_event_t event, screen_window_t window, int type)
{
    int contactId;
    int pos[2];
    int screenPos[2];
    int orientation;
    int pressure;
    long long timestamp;
    int sequenceId;

    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, (int*)&contactId);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, pos);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, screenPos);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ORIENTATION, (int*)&orientation);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_PRESSURE, (int*)&pressure);
    screen_get_event_property_llv(event, SCREEN_PROPERTY_TIMESTAMP, (long long*)&timestamp);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_SEQUENCE_ID, (int*)&sequenceId);

    static int touching = 0;
    if (type == SCREEN_EVENT_MTOUCH_TOUCH) {
    	if (touching) {
    		SDL_PrivateMouseMotion(SDL_BUTTON_LEFT, 0, pos[0], pos[1]);
    	} else {
    		SDL_PrivateMouseMotion(0, 0, pos[0], pos[1]);
    		SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_LEFT, pos[0], pos[1]);
    	}
    	moveEvent.pending = 0;
    	touching = 1;
    } else if (type == SCREEN_EVENT_MTOUCH_RELEASE) {
    	if (touching) {
    		SDL_PrivateMouseMotion(SDL_BUTTON_LEFT, 0, pos[0], pos[1]);
    		SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_LEFT, pos[0], pos[1]);
    	} else {
    		SDL_PrivateMouseMotion(0, 0, pos[0], pos[1]);
    	}
    	moveEvent.pending = 0;
    	touching = 0;
    } else if (type == SCREEN_EVENT_MTOUCH_MOVE) {
    	moveEvent.pending = 1;
    	moveEvent.touching = touching;
    	moveEvent.pos[0] = pos[0];
    	moveEvent.pos[1] = pos[1];
    	//SDL_PrivateMouseMotion((touching?SDL_BUTTON_LEFT:0), 0, pos[0], pos[1]);
    }

    // TODO: Possibly need more complicated touch handling
}

void
PLAYBOOK_PumpEvents(_THIS)
{
	while (1)
	{
		int rc = screen_get_event(m_screenContext, m_screenEvent, 0 /*timeout*/);
		if (rc)
			break;

		int type;
		rc = screen_get_event_property_iv(m_screenEvent, SCREEN_PROPERTY_TYPE, &type);
		if (rc || type == SCREEN_EVENT_NONE)
			break;

		screen_window_t window;
		screen_get_event_property_pv(m_screenEvent, SCREEN_PROPERTY_WINDOW, (void **)&window);
		if (!window && type != SCREEN_EVENT_KEYBOARD)
			break;

		switch (type)
		{
		case SCREEN_EVENT_PROPERTY:
			// TODO: Orientation events?
			break;
		case SCREEN_EVENT_POINTER:
			handlePointerEvent(m_screenEvent, window);
			break;
		case SCREEN_EVENT_KEYBOARD:
			handleKeyboardEvent(m_screenEvent);
			break;
		case SCREEN_EVENT_MTOUCH_TOUCH:
		case SCREEN_EVENT_MTOUCH_MOVE:
		case SCREEN_EVENT_MTOUCH_RELEASE:
			handleMtouchEvent(m_screenEvent, window, type);
			break;
		}
		break;
	}
	if (moveEvent.pending) {
		SDL_PrivateMouseMotion((moveEvent.touching?SDL_BUTTON_LEFT:0), 0, moveEvent.pos[0], moveEvent.pos[1]);
		moveEvent.pending = 0;
	}
}

void PLAYBOOK_InitOSKeymap(_THIS)
{
	{
		// We match perfectly from 32 to 64
		int i = 32;
		for (; i<65; i++)
		{
			Playbook_Keycodes[i].sym = i;
		}
		// Capital letters
		for (; i<91; i++)
		{
			Playbook_Keycodes[i].sym = i+32;
			Playbook_Keycodes[i].mod = KMOD_LSHIFT;
		}
		// Perfect matches again from 91 to 122
		for (; i<123; i++)
		{
			Playbook_Keycodes[i].sym = i;
		}
	}

#if 0 // Possible further keycodes that are available on the VKB
	Playbook_Keycodes[123].sym = SDLK_SPACE; /* { */
	Playbook_Keycodes[124].sym = SDLK_SPACE; /* | */
	Playbook_Keycodes[125].sym = SDLK_SPACE; /* } */
	Playbook_Keycodes[126].sym = SDLK_SPACE; /* ~ */
	Playbook_Keycodes[161].sym = SDLK_SPACE; /* upside down ! */
	Playbook_Keycodes[163].sym = SDLK_SPACE; /* British pound */
	Playbook_Keycodes[164].sym = SDLK_SPACE; /* odd circle/x */
	Playbook_Keycodes[165].sym = SDLK_SPACE; /* Japanese yen */
	Playbook_Keycodes[171].sym = SDLK_SPACE; /* small << */
	Playbook_Keycodes[177].sym = SDLK_SPACE; /* +/- */
	Playbook_Keycodes[183].sym = SDLK_SPACE; /* dot product */
	Playbook_Keycodes[187].sym = SDLK_SPACE; /* small >> */
	Playbook_Keycodes[191].sym = SDLK_SPACE; /* upside down ? */
	Playbook_Keycodes[247].sym = SDLK_SPACE; /* division */
	/*
	 * Playbook_Keycodes[8220] = smart double quote (top)
	 * Playbook_Keycodes[8221] = smart double quote (bottom)
	 * Playbook_Keycodes[8364] = euro
	 * Playbook_Keycodes[61448] = backspace
	 * Playbook_Keycodes[61453] = return
	 */
#endif
}

/* end of SDL_playbookevents.c ... */

