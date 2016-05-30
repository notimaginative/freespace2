/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef GRGL2_H
#define GRGL2_H

#include "pstypes.h"


int opengl2_init();
void opengl2_cleanup();
void opengl2_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt);

void opengl2_tcache_init();
void opengl2_tcache_cleanup();
void opengl2_tcache_frame();
void opengl2_tcache_flush();
int opengl2_tcache_set(int bitmap_id, int bitmap_type, float *u_scale, float *v_scale, int fail_on_full);
void opengl2_set_texture_state(gr_texture_source ts);

// shader program types
typedef enum {
	PROG_INVALID = -1,
	PROG_TMAPPER = 0,
	PROG_AABITMAP = 1,
	PROG_LINES = 2
} sdr_prog_t;

// shader variable indexes
enum {
	SDRI_POSITION = 1,
	SDRI_COLOR = 2,
	SDRI_SEC_COLOR = 3,
	SDRI_TEXCOORD = 4
};

int opengl2_shader_init();
void opengl2_shader_cleanup();
void opengl2_shader_use(sdr_prog_t prog);

void opengl2_error_check(const char *name, int lno);

// gr_* pointer functions
void gr_opengl2_flip();
void gr_opengl2_set_clip(int x, int y, int w, int h);
void gr_opengl2_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far);
void gr_opengl2_zbuffer_clear(int mode);
void gr_opengl2_fade_in(int instantaneous);
void gr_opengl2_fade_out(int instantaneous);
void gr_opengl2_get_region(int front, int w, int h, ubyte *data);
int gr_opengl2_save_screen();
void gr_opengl2_restore_screen(int);
void gr_opengl2_free_screen(int);
void gr_opengl2_dump_frame_start(int first_frame, int frames_between_dumps);
void gr_opengl2_dump_frame_stop();
void gr_opengl2_dump_frame();
void gr_opengl2_set_viewport(int width, int height);
void gr_opengl2_preload_init();
int gr_opengl2_preload(int bitmap_num, int is_aabitmap);
void gr_opengl2_set_gamma(float);
void gr_opengl2_release_texture(int handle);
void gr_opengl2_rect(int x, int y, int w, int h);
void gr_opengl2_shade(int x, int y, int w, int h);
void gr_opengl2_aabitmap_ex(int x, int y, int w, int h, int sx, int sy);
void gr_opengl2_aabitmap(int x, int y);
void gr_opengl2_string(int sx, int sy, const char *s);
void gr_opengl2_line(int x1, int y1, int x2, int y2);
void gr_opengl2_aaline(vertex *v1, vertex *v2);
void gr_opengl2_gradient(int x1, int y1, int x2, int y2);
void gr_opengl2_circle(int xc, int yc, int d);
void gr_opengl2_pixel(int x, int y);
void gr_opengl2_cross_fade(int bmap1, int bmap2, int x1, int y1, int x2, int y2, float pct);
void gr_opengl2_flash(int r, int g, int b);
void gr_opengl2_tmapper(int nverts, vertex **verts, uint flags);
void gr_opengl2_scaler(vertex *va, vertex *vb);

#endif // GRGL2_H
