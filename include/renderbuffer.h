/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef _RENDERBUFFER_H
#define _RENDERBUFFER_H

typedef struct renderbuffer_t {
	float x, y, z, w;
	float u, v;
	ubyte r, g, b, a;
	ubyte sr, sg, sb, sa;
} renderbuffer_t;

#define RB_OFFSET_VRT	(0)
#define RB_OFFSET_TEX	(sizeof(float) * 4)
#define RB_OFFSET_CLR	(sizeof(float) * 6)
#define RB_OFFSET_SPC	((sizeof(float) * 6) + (sizeof(ubyte) * 4))

renderbuffer_t *gr_get_render_buffer(size_t num_elems);
void gr_destroy_render_buffer();


#endif	// _RENDERBUFFER_H
