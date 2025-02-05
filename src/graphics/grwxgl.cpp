/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifdef LEGACY_GL

#include <SDL3/SDL_opengl.h>

#include "pstypes.h"
#include "2d.h"
#include "grwxgl.h"
#include "gropengl.h"
#include "gropenglinternal.h"
#include "grgl1.h"
#include "grinternal.h"
#include "mouse.h"


extern bool OGL_inited;
extern int GL_one_inited;

extern void opengl_set_variables();
extern void opengl_init_viewport();


static void wxgl_init_func_pointers()
{
	gr_screen.gf_flip = gr_wxgl_flip;
	gr_screen.gf_set_clip = gr_opengl1_set_clip;
	gr_screen.gf_reset_clip = gr_opengl_reset_clip;

	gr_screen.gf_clear = gr_opengl_clear;

	gr_screen.gf_aabitmap = gr_opengl1_aabitmap;
	gr_screen.gf_aabitmap_ex = gr_opengl1_aabitmap_ex;

	gr_screen.gf_rect = gr_opengl1_rect;
	gr_screen.gf_shade = gr_opengl1_shade;
	gr_screen.gf_string = gr_opengl1_string;
	gr_screen.gf_circle = gr_opengl1_circle;

	gr_screen.gf_line = gr_opengl1_line;
	gr_screen.gf_aaline = gr_opengl1_aaline;
	gr_screen.gf_pixel = gr_opengl1_pixel;
	gr_screen.gf_scaler = gr_opengl1_scaler;
	gr_screen.gf_tmapper = gr_opengl1_tmapper;

	gr_screen.gf_gradient = gr_opengl1_gradient;

	gr_screen.gf_print_screen = gr_opengl1_print_screen;

	gr_screen.gf_fade_in = gr_opengl1_fade_in;
	gr_screen.gf_fade_out = gr_opengl1_fade_out;
	gr_screen.gf_flash = gr_opengl1_flash;

	gr_screen.gf_zbuffer_clear = gr_opengl1_zbuffer_clear;

	gr_screen.gf_save_screen = gr_opengl1_save_screen;
	gr_screen.gf_restore_screen = gr_opengl1_restore_screen;
	gr_screen.gf_free_screen = gr_opengl1_free_screen;

	gr_screen.gf_dump_frame_start = gr_opengl1_dump_frame_start;
	gr_screen.gf_dump_frame_stop = gr_opengl1_dump_frame_stop;
	gr_screen.gf_dump_frame = gr_opengl1_dump_frame;

	gr_screen.gf_stream_start = gr_opengl1_stream_start;
	gr_screen.gf_stream_frame = gr_opengl1_stream_frame;
	gr_screen.gf_stream_stop = gr_opengl1_stream_stop;

	gr_screen.gf_set_gamma = gr_opengl1_set_gamma;

	gr_screen.gf_lock = gr_opengl_lock;
	gr_screen.gf_unlock = gr_opengl_unlock;

	gr_screen.gf_fog_set = gr_opengl1_fog_set;

	gr_screen.gf_get_region = gr_opengl1_get_region;

	gr_screen.gf_set_cull = gr_opengl_set_cull;

	gr_screen.gf_cross_fade = gr_opengl1_cross_fade;

	gr_screen.gf_preload_init = gr_opengl1_preload_init;
	gr_screen.gf_preload = gr_opengl1_preload;

	gr_screen.gf_zbias = gr_opengl_zbias;

	gr_screen.gf_set_viewport = gr_wxgl_set_viewport;

	gr_screen.gf_activate = gr_opengl_activate;

	gr_screen.gf_release_texture = gr_opengl1_release_texture;
}

static void wxgl_init()
{
	if (GL_one_inited) {
		return;
	}

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DITHER);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_FOG_HINT, GL_NICEST);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);

	glDepthRange(0.0, 1.0);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glFlush();

	wxgl_init_func_pointers();
	opengl1_tcache_init();

	gr_opengl_clear();
	gr_opengl_set_cull(1);

	GL_one_inited = 1;

}

void gr_wxgl_flip()
{
#ifndef NDEBUG
	GLenum error = glGetError();

	if (error != GL_NO_ERROR) {
		mprintf(("!!DEBUG!! OpenGL Error: %d\n", error));
	}
#endif
}

void gr_wxgl_set_viewport(int width, int height)
{
	GL_viewport_x = 0;
	GL_viewport_y = 0;
	GL_viewport_w = width;
	GL_viewport_h = height;
	GL_viewport_scale_w = 1.0f;
	GL_viewport_scale_h = 1.0f;

	glViewport(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GL_viewport_w, GL_viewport_h, 0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(GL_viewport_scale_w, GL_viewport_scale_h, 1.0f);

	// update gr_screen, which will deal with view scaling too
	gr_screen.max_w = width;
	gr_screen.max_h = height;
}

void gr_wxgl_cleanup()
{
	opengl1_tcache_cleanup();

	opengl_free_render_buffer();

	OGL_inited = false;
	GL_one_inited = 0;
}

void gr_wxgl_init()
{
	if ( OGL_inited )	{
		gr_opengl_cleanup();
		OGL_inited = false;
	}

	mprintf(( "Setting up OpenGL for wxWidgets...\n" ));

	OGL_inited = true;

	const char *gl_version = (const char*)glGetString(GL_VERSION);
	int v_major = 0, v_minor = 0;

	sscanf(gl_version, "%d.%d", &v_major, &v_minor);

	int GL_version = (v_major * 10) + v_minor;

	// version check, require 1.2+ for sake of simplicity
	if (GL_version < 12) {
		Error(LOCATION, "Minimum OpenGL version is 1.2!");
	}

	mprintf(("  Vendor   : %s\n", glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", gl_version));

	// initial viewport setup
	opengl_init_viewport();

	// set up generic variables before further init() calls
	opengl_set_variables();

	// main GL init
	wxgl_init();

	mprintf(("\n"));


	gr_screen.bits_per_pixel = 16;
	gr_screen.bytes_per_pixel = 2;

	// screen values
	Gr_red.bits = 5;
	Gr_red.shift = 11;
	Gr_red.scale = 8;
	Gr_red.mask = 0x7C01;

	Gr_green.bits = 5;
	Gr_green.shift = 6;
	Gr_green.scale = 8;
	Gr_green.mask = 0x3E1;

	Gr_blue.bits = 5;
	Gr_blue.shift = 1;
	Gr_blue.scale = 8;
	Gr_blue.mask = 0x20;

	Gr_alpha.bits = 1;
	Gr_alpha.shift = 0;
	Gr_alpha.scale = 255;
	Gr_alpha.mask = 0x1;

	// DDOI - set these so no one else does!
	// texture values, always 1555 - 16-bit
	Gr_t_red.mask = 0x7C01;
	Gr_t_red.shift = 11;
	Gr_t_red.scale = 8;

	Gr_t_green.mask = 0x3E1;
	Gr_t_green.shift = 6;
	Gr_t_green.scale = 8;

	Gr_t_blue.mask = 0x20;
	Gr_t_blue.shift = 1;
	Gr_t_blue.scale = 8;

	Gr_t_alpha.mask = 0x1;
	Gr_t_alpha.shift = 0;
	Gr_t_alpha.scale = 255;

	// alpha-texture values
	Gr_ta_red.mask = 0x0f00;
	Gr_ta_red.shift = 8;
	Gr_ta_red.scale = 16;

	Gr_ta_green.mask = 0x00f0;
	Gr_ta_green.shift = 4;
	Gr_ta_green.scale = 16;

	Gr_ta_blue.mask = 0x000f;
	Gr_ta_blue.shift = 0;
	Gr_ta_blue.scale = 16;

	Gr_ta_alpha.mask = 0xf000;
	Gr_ta_alpha.shift = 12;
	Gr_ta_alpha.scale = 16;

	// default to screen
	Gr_current_red = &Gr_red;
	Gr_current_blue = &Gr_blue;
	Gr_current_green = &Gr_green;
	Gr_current_alpha = &Gr_alpha;


	Mouse_hidden++;
	gr_reset_clip();
	gr_clear();
	gr_flip();
	gr_clear();
}

#else

void gr_wxgl_init()
{
}

void gr_wxgl_cleanup()
{
}

void gr_wxgl_flip()
{
}

void gr_wxgl_set_viewport(int width, int height)
{
}

#endif
