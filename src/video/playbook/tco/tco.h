/*
 * Copyright (c) 2011 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TCO_H_
#define _TCO_H_

#include <sys/platform.h>
#include <screen/screen.h>

__BEGIN_DECLS

#include "tco_common.h"

struct tco_context;

typedef struct tco_context_t *tco_context_t;

/**
 * Initialize the touch control overlay layer and context.
 */
int tco_initialize(tco_context_t *context, screen_context_t screenContext, tco_callbacks callbacks);

/**
 * Load the controls from a file.
 */
int tco_loadcontrols(tco_context_t context, const char* filename);

/**
 * Load default controls as a fallback.
 */
int tco_loadcontrols_default(tco_context_t context);

/**
 * Show configuration window
 * NOTE: the window MUST have a window group set already.
 */
int tco_swipedown(tco_context_t context, screen_window_t window);

/**
 * Show overlay labels
 */
int tco_showlabels(tco_context_t context, screen_window_t window);

/**
 * Provide touch events
 */
int tco_touch(tco_context_t context, screen_event_t event);

/**
 * Cleanup and shutdown
 */
void tco_shutdown(tco_context_t context);

__END_DECLS

#endif /* _TCO_H_ */
