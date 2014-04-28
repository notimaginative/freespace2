/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef _OPENGLINTERNAL_H
#define _OPENGLINTERNAL_H

#include "2d.h"


extern SDL_Window *GL_window;

extern int GL_version;

extern int GL_viewport_x;
extern int GL_viewport_y;
extern int GL_viewport_w;
extern int GL_viewport_h;
extern float GL_viewport_scale_w;
extern float GL_viewport_scale_h;
extern int GL_min_texture_width;
extern int GL_max_texture_width;
extern int GL_min_texture_height;
extern int GL_max_texture_height;


void opengl_alloc_render_buffer(unsigned int nelems);
void opengl_free_render_buffer();

typedef struct rb_t {
	float x, y, z, w;
	float u, v;
	ubyte r, g, b, a;
	ubyte sr, sg, sb, sa;
} rb_t;

extern rb_t *render_buffer;


// gr_* pointer functions
void gr_opengl_set_shader( shader * shade );
void gr_opengl_create_shader(shader * shade, float r, float g, float b, float c );
void gr_opengl_set_bitmap( int bitmap_num, int alphablend_mode = GR_ALPHABLEND_NONE, int bitblt_mode = GR_BITBLT_MODE_NORMAL, float alpha = 1.0f, int sx = -1, int sy = -1 );
void gr_opengl_set_clear_color(int r, int g, int b);
void gr_opengl_set_color( int r, int g, int b );
void gr_opengl_init_alphacolor( color *clr, int r, int g, int b, int alpha, int type );
void gr_opengl_init_color(color *c, int r, int g, int b);
void gr_opengl_get_color( int * r, int * g, int * b );
void gr_opengl_set_color_fast(color *dst);
void gr_opengl_force_fullscreen();
void gr_opengl_force_windowed();
void gr_opengl_toggle_fullscreen();
void gr_opengl_set_viewport(int width, int height);
int gr_opengl_zbuffer_get();
int gr_opengl_zbuffer_set(int mode);

#endif	// _OPENGLINTERNAL_H
