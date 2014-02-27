/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "gropengl.h"
#include "gropengl2.h"
#include "gropenglinternal.h"
#include "2d.h"
#include "mouse.h"
#include "pstypes.h"
#include "cfile.h"
#include "bmpman.h"
#include "grinternal.h"


static int GL_two_inited = 0;


void opengl2_cleanup()
{
	if ( !GL_two_inited ) {
		return;
	}

	gr_opengl2_reset_clip();
	gr_opengl2_clear();
	gr_opengl2_flip();

	opengl2_shaders_cleanup();
	opengl2_tcache_cleanup();

	GL_two_inited = 0;
}

static void opengl2_init_func_pointers()
{
	gr_screen.gf_flip = gr_opengl2_flip;
	gr_screen.gf_set_clip = gr_opengl2_set_clip;
	gr_screen.gf_reset_clip = gr_opengl2_reset_clip;
	gr_screen.gf_set_font = grx_set_font;

	gr_screen.gf_set_color = gr_opengl_set_color;
	gr_screen.gf_set_bitmap = gr_opengl_set_bitmap;
	gr_screen.gf_create_shader = gr_opengl_create_shader;
	gr_screen.gf_set_shader = gr_opengl_set_shader;
	gr_screen.gf_clear = gr_opengl2_clear;

	gr_screen.gf_aabitmap = gr_opengl2_aabitmap;
	gr_screen.gf_aabitmap_ex = gr_opengl2_aabitmap_ex;

	gr_screen.gf_rect = gr_opengl2_rect;
	gr_screen.gf_shade = gr_opengl2_shade;
	gr_screen.gf_string = gr_opengl2_string;
	gr_screen.gf_circle = gr_opengl2_circle;

	gr_screen.gf_line = gr_opengl2_line;
	gr_screen.gf_aaline = gr_opengl2_aaline;
	gr_screen.gf_pixel = gr_opengl2_pixel;
	gr_screen.gf_scaler = gr_opengl2_scaler;
	gr_screen.gf_tmapper = gr_opengl2_tmapper;

	gr_screen.gf_gradient = gr_opengl2_gradient;

	gr_screen.gf_get_color = gr_opengl_get_color;
	gr_screen.gf_init_color = gr_opengl_init_color;
	gr_screen.gf_init_alphacolor = gr_opengl_init_alphacolor;
	gr_screen.gf_set_color_fast = gr_opengl_set_color_fast;
	gr_screen.gf_print_screen = gr_opengl2_print_screen;

	gr_screen.gf_fade_in = gr_opengl2_fade_in;
	gr_screen.gf_fade_out = gr_opengl2_fade_out;
	gr_screen.gf_flash = gr_opengl2_flash;

	gr_screen.gf_zbuffer_get = gr_opengl_zbuffer_get;
	gr_screen.gf_zbuffer_set = gr_opengl_zbuffer_set;
	gr_screen.gf_zbuffer_clear = gr_opengl2_zbuffer_clear;

	gr_screen.gf_save_screen = gr_opengl2_save_screen;
	gr_screen.gf_restore_screen = gr_opengl2_restore_screen;
	gr_screen.gf_free_screen = gr_opengl2_free_screen;

	gr_screen.gf_dump_frame_start = gr_opengl2_dump_frame_start;
	gr_screen.gf_dump_frame_stop = gr_opengl2_dump_frame_stop;
	gr_screen.gf_dump_frame = gr_opengl2_dump_frame;

	gr_screen.gf_set_gamma = gr_opengl2_set_gamma;

	gr_screen.gf_lock = gr_opengl2_lock;
	gr_screen.gf_unlock = gr_opengl2_unlock;

	gr_screen.gf_fog_set = gr_opengl2_fog_set;

	gr_screen.gf_get_region = gr_opengl2_get_region;

	gr_screen.gf_set_cull = gr_opengl2_set_cull;

	gr_screen.gf_cross_fade = gr_opengl2_cross_fade;

	gr_screen.gf_set_clear_color = gr_opengl_set_clear_color;

	gr_screen.gf_preload_init = gr_opengl2_preload_init;
	gr_screen.gf_preload = gr_opengl2_preload;

	gr_screen.gf_zbias = gr_opengl2_zbias;

	gr_screen.gf_force_windowed = gr_opengl_force_windowed;
	gr_screen.gf_force_fullscreen = gr_opengl_force_fullscreen;
	gr_screen.gf_set_viewport = gr_opengl_set_viewport;

	gr_screen.gf_activate = gr_opengl2_activate;
}

void opengl2_init()
{
	if (GL_two_inited) {
		return;
	}

	/*
	  1 = use secondary color ext
	  2 = use opengl linear fog
	 */
	OGL_fog_mode = 2;


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

	opengl2_init_func_pointers();
	opengl2_tcache_init();
	opengl2_init_shaders();

	gr_opengl2_clear();

	GL_two_inited = 1;
}
