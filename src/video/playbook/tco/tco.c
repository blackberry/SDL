/*
 * tco.c
 *
 *  Created on: Apr 7, 2013
 *      Author: Jeremy
 */

#include "tco_common.h"
#include "tco_c.h"
#include "tco_cfg.h"
#include "tco_control.h"
#include "tco_controlfactory.h"
#include "tco_dispatcher.h"
//#include "touchcontroloverlay_priv.h"
//#include "eventdispatcher.h"
#include <bps/bps.h>
#include <bps/screen.h>
#include <bps/event.h>
#include <bps/navigator.h>
//#include "control.h"
//#include "controlfactory.h"
//#include "configwindow.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

int tco_initialize(tco_context_t *context, screen_context_t screenContext, tco_callbacks callbacks)
{
	if (!context) return TCO_FAILURE;

	tco_context *ctx = (tco_context *)malloc(sizeof(tco_context));
	ctx->screenContext = screenContext;
	ctx->appWindow = 0;
	ctx->configWindow = 0;

	ctx->controls = 0;
	memset(ctx->controlMap, 0, sizeof(ctx->controlMap[0]) * MAX_TOUCHPOINTS);

	ctx->callbacks = callbacks;
	*context = ctx;
	return TCO_SUCCESS;
}

int tco_loadcontrols(tco_context_t context, const char* filename)
{
	tco_context *ctx = context;
	int ret = TCO_FAILURE;
	xmlDoc *document = 0;

	document = xmlReadFile(filename, 0, 0);
	if (!document) {
		fprintf(stderr, "Unable to parse control file %s\n", filename);
		goto done;
	}

	xmlNode *root = xmlDocGetRootElement(document);

	int versionMatch = 0;
	if (root && root->properties) {
		xmlAttr *properties = root->properties;
		while (properties) {
			if (!xmlStrncasecmp(properties->name, BAD_CAST "version", strlen("version"))) {
				if (properties->children && properties->children->content) {
					int version = strtol((const char *)properties->children->content, 0, 10);
					if (version != 0 && version <= TCO_FILE_VERSION) {
						versionMatch = 1;
					}
				}
				break;
			}
			properties = properties->next;
		}
	}

	if (!versionMatch) {
		fprintf(stderr, "Version mismatch\n");
		goto done;
	}

	ret = TCO_SUCCESS;
	xmlNode *cur = root->children;
	tco_control_t *control = 0;
	while (cur) {
		control = tco_controlfactory_create_control(ctx, cur);
		if (control) {
			control->next = ctx->controls;
			ctx->controls = control;
		}
		cur = cur->next;
	}

done:
	if (document)
		xmlFreeDoc(document);
	document = 0;
	xmlCleanupParser();
	return ret;
}

int tco_loadcontrols_default(tco_context_t context)
{
	tco_context *ctx = context;
	if (!ctx) return TCO_FAILURE;

	tco_event_dispatcher_t *dispatcher = tco_dispatcher_touchscreen_create(ctx->callbacks.handleTouchScreenFunc);
	tco_control_t *control = tco_control_create(ctx->screenContext,
			TOUCHSCREEN, 0, 0, 1024, 600,
			dispatcher, 0);
	tco_control_fill(control);
	control->next = ctx->controls;
	ctx->controls = control;
	return TCO_SUCCESS;
}

int tco_swipedown(tco_context_t context, screen_window_t window)
{
	tco_context *ctx = context;
	if (!ctx) return TCO_FAILURE;

	if (!ctx->configWindow) {
		ctx->configWindow = tco_cfg_create(ctx->screenContext, window);
		if (!ctx->configWindow)
			return TCO_FAILURE;
		tco_cfg_runloop(ctx->configWindow, ctx);
		tco_cfg_destroy(ctx->configWindow);
		ctx->configWindow = 0;
	}
	return TCO_SUCCESS;
}

int tco_showlabels(tco_context_t context, screen_window_t window)
{
	tco_context *ctx = context;
	if (!ctx) return TCO_FAILURE;

	tco_control_t *control = ctx->controls;
	while (control) {
		tco_control_showLabel(control, window);
		control = control->next;
	}
	return TCO_SUCCESS;
}

int tco_touch(tco_context_t context, screen_event_t event)
{
	tco_context *ctx = context;
	if (!ctx) return TCO_FAILURE;

	int handled = 0;
	int type;
    int contactId;
    int pos[2];
    int screenPos[2];
    int orientation;
    long long timestamp;
    int sequenceId;

    screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, (int*)&contactId);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, pos);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, screenPos);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ORIENTATION, (int*)&orientation);
    screen_get_event_property_llv(event, SCREEN_PROPERTY_TIMESTAMP, (long long*)&timestamp);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_SEQUENCE_ID, (int*)&sequenceId);

    tco_control_t *touchPointOwner = ctx->controlMap[contactId];
    if (touchPointOwner) {
    	handled = tco_control_handleTouch(touchPointOwner, type, contactId, pos, timestamp);
    	if (!handled) {
    		ctx->controlMap[contactId] = 0;
    	}
    }

    if (!handled) {
    	tco_control_t *control = ctx->controls;
    	while (control) {
			if (control == touchPointOwner) {
				control = control->next;
				continue;
			}

			handled |= tco_control_handleTouch(control, type, contactId, pos, timestamp);
			if (handled) {
				ctx->controlMap[contactId] = control;
				break; // Only allow the first control to handle the touch.
			}
			control = control->next;
		}
    }

	if (handled)
		return TCO_SUCCESS;
	else
		return TCO_UNHANDLED;
}

void tco_shutdown(tco_context_t context)
{
	tco_context *ctx = context;
	if (!ctx) return;

	ctx->appWindow = 0;
	tco_cfg_destroy(ctx->configWindow);
	ctx->configWindow = 0;

	tco_control_t *control = ctx->controls;
	tco_control_t *tmp = 0;
	while (control) {
		tmp = control;
		control = control->next;
		tco_control_destroy(tmp);
	}
	memset(ctx->controlMap, 0, sizeof(ctx->controlMap[0]) * MAX_TOUCHPOINTS);
	memset(&ctx->callbacks, 0, sizeof(ctx->callbacks));

	free(ctx);
	ctx = 0;
}

tco_control_t *tco_controlAt(tco_context *ctx, int pos[])
{
	tco_control_t *control = ctx->controls;
	while (control != 0) {
		if (tco_control_inBounds(control, pos))
			return control;
		control = control->next;
	}
	return 0;
}

void tco_drawControls(tco_context *ctx, screen_buffer_t buffer)
{
	tco_control_t *control = ctx->controls;
	while (control != 0) {
		tco_control_draw(control, buffer);
		control = control->next;
	}
}
