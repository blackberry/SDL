/*
 * tco_cfg.h
 *
 *  Created on: Apr 7, 2013
 *      Author: Jeremy
 */

#ifndef TCO_CFG_H_
#define TCO_CFG_H_

#include <screen/screen.h>
#include "tco_structs.h"

tco_config_window_t *tco_cfg_create(screen_context_t screenContext, screen_window_t window);
void tco_cfg_destroy(tco_config_window_t *window);
void tco_cfg_runloop(tco_config_window_t *window, tco_context *ctx);

#endif /* TCO_CFG_H_ */
