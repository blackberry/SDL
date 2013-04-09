/*
 * tco_controlfactory.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Jeremy
 */

#ifndef TCO_CONTROLFACTORY_H_
#define TCO_CONTROLFACTORY_H_

#include "tco_structs.h"

#include <libxml/tree.h>

tco_control_t *tco_controlfactory_create_control(const tco_context *ctx, const xmlNode *cur);

#endif /* TCO_CONTROLFACTORY_H_ */
