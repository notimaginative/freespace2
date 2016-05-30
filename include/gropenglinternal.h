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


typedef enum gr_texture_source {
	TEXTURE_SOURCE_NONE,
	TEXTURE_SOURCE_DECAL,
	TEXTURE_SOURCE_NO_FILTERING,
} gr_texture_source;

typedef enum gr_alpha_blend {
		ALPHA_BLEND_NONE,			// 1*SrcPixel + 0*DestPixel
		ALPHA_BLEND_ADDITIVE,			// 1*SrcPixel + 1*DestPixel
		ALPHA_BLEND_ALPHA_ADDITIVE,             // Alpha*SrcPixel + 1*DestPixel
		ALPHA_BLEND_ALPHA_BLEND_ALPHA,          // Alpha*SrcPixel + (1-Alpha)*DestPixel
		ALPHA_BLEND_ALPHA_BLEND_SRC_COLOR,      // Alpha*SrcPixel + (1-SrcPixel)*DestPixel
} gr_alpha_blend;

typedef enum gr_zbuffer_type {
		ZBUFFER_TYPE_NONE,
		ZBUFFER_TYPE_READ,
		ZBUFFER_TYPE_WRITE,
		ZBUFFER_TYPE_FULL,
} gr_zbuffer_type;

extern volatile int GL_activate;
extern volatile int GL_deactivate;

extern SDL_Window *GL_window;
extern SDL_GLContext GL_context;

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


void opengl_set_variables();
void opengl_init_viewport();

void opengl_stuff_fog_value(float z, float *f_val);

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
void gr_opengl_force_fullscreen();
void gr_opengl_force_windowed();
void gr_opengl_toggle_fullscreen();
void gr_opengl_clear();
void gr_opengl_reset_clip();
void gr_opengl_print_screen(const char *filename);
uint gr_opengl_lock();
void gr_opengl_unlock();
void gr_opengl_zbias(int bias);
void gr_opengl_set_cull(int cull);
void gr_opengl_activate(int active);

#endif	// _OPENGLINTERNAL_H
