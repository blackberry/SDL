/*
 * tco_label.c
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#include "tco_label.h"
#include "tco_img.h"
#include "tco_label_window.h"

#include <stdlib.h>
#include <screen/screen.h>

tco_label_t *tco_label_create(screen_context_t screenContext, int x, int y, unsigned w, unsigned h, const char *childImage)
{
	tco_label_t *label = (tco_label_t *)malloc(sizeof(*label));
	if (!label) return 0;

	label->x = x;
	label->y = y;
	label->width = w;
	label->height = h;
	label->window = 0;

	if (childImage) {
		unsigned width, height;
		screen_buffer_t buffer;
		screen_pixmap_t pixmap;
		screen_create_pixmap(&pixmap, screenContext);

		if (tco_img_loadfile(childImage, &width, &height, pixmap, &buffer)) {
			label->window = tco_label_window_create(screenContext, w, h);
			tco_label_window_draw(label->window, pixmap, w, h);
		}

		screen_destroy_pixmap(pixmap);
	}
	return label;
}

void tco_label_destroy(tco_label_t *label)
{
	if (!label) return;

	label->control = 0;
	tco_label_window_destroy(label->window);
	label->window = 0;
	free(label);
}

void tco_label_draw(tco_label_t *label, screen_window_t window, int x, int y)
{
	if (!label->window)
		return;
	tco_label_window_showAt(label->window, window, label->x+x, label->y+y);
}

void tco_label_move(tco_label_t *label, int x, int y)
{
	if (!label->window)
		return;
	tco_label_window_move(label->window, label->x+x, label->y+y);
}
