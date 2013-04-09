/*
 * tco_cfg.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_cfg.h"
#include "tco_win.h"
#include "tco_c.h"
#include "tco_control.h"

#include <stdlib.h>
#include <bps/bps.h>
#include <bps/navigator.h>
#include <bps/screen.h>

tco_config_window_t *tco_cfg_create(screen_context_t screenContext, screen_window_t parent)
{
	tco_config_window_t *win = (tco_config_window_t *)malloc(sizeof(tco_config_window_t));
	if (!win) return win;

	if (!tco_win_create(&win->base, screenContext, parent)) {
		free(win);
		return 0;
	}

	if (!win->base.valid || !tco_win_setZOrder(&win->base, 10) ||
			!tco_win_setSensitivity(&win->base, 1)) {
		tco_win_destroy(&win->base);
		free(win);
		return 0;
	}

	win->selected = 0;
	return win;
}

void tco_cfg_destroy(tco_config_window_t *window)
{
	if (!window) return;
	tco_win_destroy(&window->base);
	free(window);
}

static screen_buffer_t tco_cfg_draw(tco_config_window_t *window, tco_context *ctx)
{
	screen_buffer_t buffer;
	unsigned char *pixels;
	int stride;
	if (!tco_win_pixels(&window->base, &buffer, &pixels, &stride)) {
		return 0;
	}

	{
		// Fill pixels
		int i=0,j=0;
		for (i=0; i<window->base.size[1]; i++) {
			for (j=0; j<window->base.size[0]; j++) {
				pixels[i*stride+j*4] = 0xa0;
				pixels[i*stride+j*4+1] = 0xa0;
				pixels[i*stride+j*4+2] = 0xa0;
				pixels[i*stride+j*4+3] = 0xa0;
			}
		}
	}
	tco_drawControls(ctx, buffer);
	return buffer;
}

void tco_cfg_runloop(tco_config_window_t *window, tco_context *ctx)
{
	screen_buffer_t buffer = tco_cfg_draw(window, ctx);

	bool showingWindow = true;
	bps_initialize();
	screen_request_events(window->base.context);
	bps_event_t *event;
	screen_event_t se;
	int eventType;
	int contactId;
	bool touching = false;
	bool releasedThisRound = false;
	int startPos[2] = {0,0};
	int endPos[2] = {0,0};
	bool scaling = false;

	while (showingWindow)
	{
		const int dirtyRects[4] = {0, 0, window->base.size[0], window->base.size[1]};

		releasedThisRound = false;
		bps_get_event(&event, 0);
		while (showingWindow && event)
		{
			int domain = bps_event_get_domain(event);
			if (domain == navigator_get_domain())
			{
				if (bps_event_get_code(event) == NAVIGATOR_SWIPE_DOWN)
					showingWindow = false;
				else if (bps_event_get_code(event) == NAVIGATOR_EXIT) {
					showingWindow = false;
				}
			} else if (domain == screen_get_domain()) {
				se = screen_event_get_event(event);
				screen_get_event_property_iv(se, SCREEN_PROPERTY_TYPE, &eventType);
				screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
				switch (eventType)
				{
				case SCREEN_EVENT_MTOUCH_TOUCH:
					screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
					if (contactId == 0 && !touching && !releasedThisRound) {
						touching = true;
						screen_get_event_property_iv(se, SCREEN_PROPERTY_SOURCE_POSITION, startPos);
						endPos[0] = startPos[0];
						endPos[1] = startPos[1];
					}
					break;
				case SCREEN_EVENT_MTOUCH_MOVE:
					screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
					if (contactId == 0 && touching) {
						screen_get_event_property_iv(se, SCREEN_PROPERTY_SOURCE_POSITION, endPos);
					}
					break;
				case SCREEN_EVENT_MTOUCH_RELEASE:
					screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
					if (contactId == 0 && touching) {
						touching = false;
						releasedThisRound = true;
						screen_get_event_property_iv(se, SCREEN_PROPERTY_SOURCE_POSITION, endPos);
					}
					break;
				default:
					//fprintf(stderr, "Unknown screen event: %d\n", eventType);
					break;
				}
			}

			bps_get_event(&event, 0);
		}

		if (releasedThisRound) {
			window->selected = 0;
		} else if (touching) {
			if (!window->selected)
				window->selected = tco_controlAt(ctx, startPos);
			if (window->selected && (endPos[0] != startPos[0] || endPos[1] != startPos[1])) {
				tco_control_move(window->selected, endPos[0] - startPos[0], endPos[1] - startPos[1], (unsigned*)window->base.size);
				buffer = tco_cfg_draw(window, ctx);
				startPos[0] = endPos[0];
				startPos[1] = endPos[1];
			}
		}

		if (buffer)
			tco_win_post(&window->base, buffer);
	}
	bps_shutdown();
}
