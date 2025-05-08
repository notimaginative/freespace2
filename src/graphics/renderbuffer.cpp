/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "renderbuffer.h"

static size_t render_buffer_size = 0;

static renderbuffer_t *render_buffer = nullptr;


renderbuffer_t *gr_get_render_buffer(size_t num_elems)
{
	if (num_elems < 1) {
		num_elems = 1;
	}

	if ( render_buffer && (num_elems <= render_buffer_size) ) {
		return render_buffer;
	}

	if (render_buffer) {
		free(render_buffer);
	}

	render_buffer = reinterpret_cast<renderbuffer_t *>(malloc(sizeof(renderbuffer_t) * num_elems));
	render_buffer_size = num_elems;

	return render_buffer;
}

void gr_destroy_render_buffer()
{
	if (render_buffer) {
		free(render_buffer);
		render_buffer = nullptr;
		render_buffer_size = 0;
	}
}
