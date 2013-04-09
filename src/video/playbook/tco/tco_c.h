/*
 * tco_c.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_C_H_
#define TCO_C_H_

#include "tco_structs.h"

tco_control_t *tco_controlAt(tco_context *ctx, int pos[]);

void tco_drawControls(tco_context *ctx, screen_buffer_t buffer);

#endif /* TCO_C_H_ */
