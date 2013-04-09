/*
 * tco_controlfactory.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_controlfactory.h"
#include "tco_control.h"
#include "tco_dispatcher.h"
#include "tco_label.h"

#include <string.h>

#define DEFINE_GET_PROPERTY(T) \
int get_##T##_property(xmlAttr *properties, const char *propertyName, T* value) { \
	if (!xmlStrncasecmp(properties->name, BAD_CAST propertyName, strlen(propertyName))) { \
		if (properties->children && properties->children->content) { \
			*value = (T)strtol((const char *)properties->children->content, 0, 10); \
			return 1; \
		} \
	} \
	return 0; \
}

DEFINE_GET_PROPERTY(int)
DEFINE_GET_PROPERTY(unsigned)
DEFINE_GET_PROPERTY(uint16_t)

static int getButtonProperty(xmlAttr *properties, const char *propertyName, int* value) {
	if (!xmlStrncasecmp(properties->name, BAD_CAST propertyName, strlen(propertyName))) {
		if (properties->children && properties->children->content) {
			if (!xmlStrncasecmp(properties->children->content, BAD_CAST "left", strlen("left"))) {
				*value = TCO_MOUSE_LEFT_BUTTON;
			} else if (!xmlStrncasecmp(properties->children->content, BAD_CAST "right", strlen("right"))) {
				*value = TCO_MOUSE_RIGHT_BUTTON;
			} else if (!xmlStrncasecmp(properties->children->content, BAD_CAST "middle", strlen("middle"))) {
				*value = TCO_MOUSE_MIDDLE_BUTTON;
			} else
				return 0;
			return 1;
		}
	}
	return 0;
}

tco_control_t *tco_controlfactory_create_control(const tco_context *ctx, const xmlNode *node)
{
	TCOControlType type = KEY;
	int x = 0, y = 0;
	unsigned w = 100, h = 100;
	int sym = 0, mod = 0, scancode = 0;
	uint16_t unicode = 0;
	int mask = 0, button = 0;
	xmlChar *imageFile = 0;
	int tapSensitive = 0;

	if (!xmlStrncasecmp(node->name, BAD_CAST "key", strlen("key"))) {
		type = KEY;
	} else if (!xmlStrncasecmp(node->name, BAD_CAST "dpad", strlen("dpad"))) {
		type = DPAD;
	} else if (!xmlStrncasecmp(node->name, BAD_CAST "toucharea", strlen("toucharea"))) {
		type = TOUCHAREA;
	} else if (!xmlStrncasecmp(node->name, BAD_CAST "mousebutton", strlen("mousebutton"))) {
		type = MOUSEBUTTON;
	} else if (!xmlStrncasecmp(node->name, BAD_CAST "touchscreen", strlen("touchscreen"))) {
		type = TOUCHSCREEN;
	} else {
		type = -1;
	}

	xmlAttr *properties = node->properties;
	while (properties)
	{
		get_int_property(properties, "x", &x);
		get_int_property(properties, "y", &y);
		get_unsigned_property(properties, "width", &w);
		get_unsigned_property(properties, "height", &h);
		if (!xmlStrncasecmp(properties->name, BAD_CAST "image", strlen("image"))) {
			if (properties->children && properties->children->content)
				imageFile = properties->children->content;
		}

		switch (type) {
		case KEY:
			get_int_property(properties, "sym", &sym);
			get_int_property(properties, "mod", &mod);
			get_int_property(properties, "scancode", &scancode);
			get_uint16_t_property(properties, "unicode", &unicode);
			break;
		case TOUCHAREA:
			get_int_property(properties, "tapSensitive", &tapSensitive);
			break;
		case MOUSEBUTTON:
			get_int_property(properties, "mask", &mask);
			getButtonProperty(properties, "button", &button);
			break;
		}
		properties = properties->next;
	}

	tco_control_t *control = 0;
	tco_event_dispatcher_t *dispatcher = 0;
	tco_event_dispatcher_t *tapDispatcher = 0;
	switch (type) {
	case KEY:
		dispatcher = tco_dispatcher_keyevent_create(ctx->callbacks.handleKeyFunc, sym, mod, scancode, unicode);
		break;
	case DPAD:
		dispatcher = tco_dispatcher_dpad_create(ctx->callbacks.handleDPadFunc);
		break;
	case TOUCHAREA:
		dispatcher = tco_dispatcher_toucharea_create(ctx->callbacks.handleTouchFunc);
		if (tapSensitive > 0)
			tapDispatcher = tco_dispatcher_tap_create(ctx->callbacks.handleTapFunc);
		break;
	case MOUSEBUTTON:
		dispatcher = tco_dispatcher_mouseevent_create(ctx->callbacks.handleMouseButtonFunc, mask, button);
		break;
	case TOUCHSCREEN:
		dispatcher = tco_dispatcher_touchscreen_create(ctx->callbacks.handleTouchScreenFunc);
		break;
	default:
		return 0;
	}
	control = tco_control_create(ctx->screenContext, type, x, y, w, h, dispatcher, tapDispatcher);

	xmlNode *child = node->children;
	xmlChar *childImage;
	tco_label_t *label;
	while (child) {
		if (!xmlStrncasecmp(child->name, BAD_CAST "label", strlen("label"))) {
			xmlAttr *properties = child->properties;
			childImage = 0;
			while (properties)
			{
				get_int_property(properties, "x", &x);
				get_int_property(properties, "y", &y);
				get_unsigned_property(properties, "width", &w);
				get_unsigned_property(properties, "height", &h);
				if (!xmlStrncasecmp(properties->name, BAD_CAST "image", strlen("image"))) {
					if (properties->children && properties->children->content)
						childImage = properties->children->content;
				}

				properties = properties->next;
			}
			label = tco_label_create(ctx->screenContext, x, y, w, h, (char*)childImage);
			tco_control_addLabel(control, label);
		}
		child = child->next;
	}

	if (imageFile) {
		if (!tco_control_loadImage(control, (char *)imageFile))
			fprintf(stderr, "Failed to load from PNG\n");
	} else {
		tco_control_fill(control);
	}
	return control;
}
