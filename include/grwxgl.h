/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef GRWXGL_H
#define GRWXGL_H

void gr_wxgl_init();
void gr_wxgl_cleanup();
void gr_wxgl_flip();
void gr_wxgl_set_viewport(int width, int height);

#endif // GRWXGL_H
