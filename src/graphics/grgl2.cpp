/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "SDL_opengles2.h"

#include "gropengl.h"
#include "gropenglinternal.h"
#include "grgl2.h"
#include "2d.h"
#include "mouse.h"
#include "pstypes.h"
#include "cfile.h"
#include "bmpman.h"
#include "grinternal.h"
#include "osapi.h"
#include "osregistry.h"


int GL_two_inited = 0;

bool Use_mipmaps = false;


static gr_alpha_blend GL_current_alpha_blend = (gr_alpha_blend) -1;
static gr_zbuffer_type GL_current_zbuffer_type = (gr_zbuffer_type) -1;

void opengl2_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt)
{
	opengl2_set_texture_state(ts);

	if (ab != GL_current_alpha_blend) {
		switch (ab) {
			case ALPHA_BLEND_NONE:			// 1*SrcPixel + 0*DestPixel
				glBlendFunc(GL_ONE, GL_ZERO);
				break;
			case ALPHA_BLEND_ADDITIVE:		// 1*SrcPixel + 1*DestPixel
				glBlendFunc(GL_ONE, GL_ONE);
				break;
			case ALPHA_BLEND_ALPHA_ADDITIVE:	// Alpha*SrcPixel + 1*DestPixel
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case ALPHA_BLEND_ALPHA_BLEND_ALPHA:	// Alpha*SrcPixel + (1-Alpha)*DestPixel
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case ALPHA_BLEND_ALPHA_BLEND_SRC_COLOR:	// Alpha*SrcPixel + (1-SrcPixel)*DestPixel
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
				break;
			default:
				break;
		}

		GL_current_alpha_blend = ab;
	}

	if (zt != GL_current_zbuffer_type) {
		switch (zt) {
			case ZBUFFER_TYPE_NONE:
				glDepthFunc(GL_ALWAYS);
				glDepthMask(GL_FALSE);
				break;
			case ZBUFFER_TYPE_READ:
				glDepthFunc(GL_LESS);
				glDepthMask(GL_FALSE);
				break;
			case ZBUFFER_TYPE_WRITE:
				glDepthFunc(GL_ALWAYS);
				glDepthMask(GL_TRUE);
				break;
			case ZBUFFER_TYPE_FULL:
				glDepthFunc(GL_LESS);
				glDepthMask(GL_TRUE);
				break;
			default:
				break;
		}

		GL_current_zbuffer_type = zt;
	}
}

void opengl2_cleanup()
{
	if ( !GL_two_inited ) {
		return;
	}

	opengl2_tcache_cleanup();
	opengl2_shader_cleanup();

	GL_two_inited = 0;
}

static void opengl2_init_func_pointers()
{
	gr_screen.gf_flip = gr_opengl2_flip;
	gr_screen.gf_set_clip = gr_opengl2_set_clip;
	gr_screen.gf_reset_clip = gr_opengl_reset_clip;

	gr_screen.gf_clear = gr_opengl_clear;

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

	gr_screen.gf_print_screen = gr_opengl_print_screen;

	gr_screen.gf_fade_in = gr_opengl2_fade_in;
	gr_screen.gf_fade_out = gr_opengl2_fade_out;
	gr_screen.gf_flash = gr_opengl2_flash;

	gr_screen.gf_zbuffer_clear = gr_opengl2_zbuffer_clear;

	gr_screen.gf_save_screen = gr_opengl2_save_screen;
	gr_screen.gf_restore_screen = gr_opengl2_restore_screen;
	gr_screen.gf_free_screen = gr_opengl2_free_screen;

	gr_screen.gf_dump_frame_start = gr_opengl2_dump_frame_start;
	gr_screen.gf_dump_frame_stop = gr_opengl2_dump_frame_stop;
	gr_screen.gf_dump_frame = gr_opengl2_dump_frame;

	gr_screen.gf_set_gamma = gr_opengl2_set_gamma;

	gr_screen.gf_lock = gr_opengl_lock;
	gr_screen.gf_unlock = gr_opengl_unlock;

	gr_screen.gf_fog_set = gr_opengl2_fog_set;

	gr_screen.gf_get_region = gr_opengl2_get_region;

	gr_screen.gf_set_cull = gr_opengl_set_cull;

	gr_screen.gf_cross_fade = gr_opengl2_cross_fade;

	gr_screen.gf_preload_init = gr_opengl2_preload_init;
	gr_screen.gf_preload = gr_opengl2_preload;

	gr_screen.gf_zbias = gr_opengl_zbias;

	gr_screen.gf_force_windowed = gr_opengl_force_windowed;
	gr_screen.gf_force_fullscreen = gr_opengl_force_fullscreen;
	gr_screen.gf_toggle_fullscreen = gr_opengl_toggle_fullscreen;

	gr_screen.gf_set_viewport = gr_opengl2_set_viewport;

	gr_screen.gf_activate = gr_opengl_activate;

	gr_screen.gf_release_texture = gr_opengl2_release_texture;
}

int opengl2_init()
{
	if (GL_two_inited) {
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	GL_context = SDL_GL_CreateContext(GL_window);

	if ( !GL_context ) {
		return 0;
	}

	mprintf(("  Vendor   : %s\n", glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", glGetString(GL_VERSION)));

	// set up generic variables
	opengl_set_variables();

	opengl2_init_func_pointers();
	opengl2_tcache_init();
	opengl2_shader_init();

	// initial viewport setup
	gr_opengl2_set_viewport(gr_screen.max_w, gr_screen.max_h);

	glEnable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glDepthRangef(0.0f, 1.0f);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if ( SDL_GL_ExtensionSupported("GL_OES_texture_npot") ) {
		Use_mipmaps	= true;
	}

	mprintf(("  Mipmaps  : %s\n", Use_mipmaps ? "Enabled" : "Disabled"));

	glFlush();

	gr_opengl_clear();
	gr_opengl_set_cull(1);

	GL_two_inited = 1;

	return 1;
}

void gr_opengl2_flip()
{
	if ( !GL_two_inited ) {
		return;
	}

	gr_opengl_reset_clip();

	mouse_eval_deltas();

	if ( mouse_is_visible() ) {
		int mx, my;

		mouse_get_pos(&mx, &my);

		if (Gr_cursor == -1) {
#ifndef NDEBUG
			gr_set_color(255, 255, 255);
			gr_opengl2_line(mx, my, mx+7, my+7);
			gr_opengl2_line(mx, my, mx+5, my);
			gr_opengl2_line(mx, my, mx, my+5);
#endif
		} else {
			gr_set_bitmap(Gr_cursor);
			gr_bitmap(mx, my);
		}
	}

#ifndef NDEBUG
	GLenum error = GL_NO_ERROR;

	do {
		error = glGetError();

		if (error != GL_NO_ERROR) {
			nprintf(("Warning", "!!DEBUG!! OpenGL Error: %d\n", error));
		}
	} while (error != GL_NO_ERROR);
#endif

	SDL_GL_SwapWindow(GL_window);

	opengl2_tcache_frame();
}

void gr_opengl2_set_clip(int x, int y, int w, int h)
{
	// check for sanity of parameters
	CAP(x, 0, gr_screen.max_w - 1);
	CAP(y, 0, gr_screen.max_h - 1);
	CAP(w, 0, gr_screen.max_w - x);
	CAP(h, 0, gr_screen.max_h - y);

	gr_screen.offset_x = x;
	gr_screen.offset_y = y;
	gr_screen.clip_left = 0;
	gr_screen.clip_right = w-1;
	gr_screen.clip_top = 0;
	gr_screen.clip_bottom = h-1;
	gr_screen.clip_width = w;
	gr_screen.clip_height = h;

	glEnable(GL_SCISSOR_TEST);
	glScissor(x, GL_viewport_h-y-h, w, h);
}

void gr_opengl2_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far)
{
	gr_screen.current_fog_mode = fog_mode;
	gr_screen.fog_near = fog_near;
	gr_screen.fog_far = fog_far;

	gr_init_color(&gr_screen.current_fog_color, r, g, b);
}

void gr_opengl2_zbuffer_clear(int mode)
{
	if (mode) {
		Gr_zbuffering = 1;
		Gr_zbuffering_mode = GR_ZBUFF_FULL;
		Gr_global_zbuffering = 1;

		opengl2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_NONE, ZBUFFER_TYPE_FULL);
		glClear(GL_DEPTH_BUFFER_BIT);
	} else {
		Gr_zbuffering = 0;
		Gr_zbuffering_mode = GR_ZBUFF_NONE;
		Gr_global_zbuffering = 0;
	}
}

void gr_opengl2_fade_in(int instantaneous)
{

}

void gr_opengl2_fade_out(int instantaneous)
{

}

void gr_opengl2_get_region(int front, int w, int h, ubyte *data)
{

}

int gr_opengl2_save_screen()
{
	return -1;
}

void gr_opengl2_restore_screen(int)
{

}

void gr_opengl2_free_screen(int)
{

}

void gr_opengl2_dump_frame_start(int first_frame, int frames_between_dumps)
{

}

void gr_opengl2_dump_frame_stop()
{

}

void gr_opengl2_dump_frame()
{

}

void gr_opengl2_set_viewport(int width, int height)
{
	int w, h, x, y;

	float ratio = gr_screen.max_w / i2fl(gr_screen.max_h);

	w = width;
	h = fl2i((width / ratio) + 0.5f);

	if (h > height) {
		h = height;
		w = fl2i((height * ratio) + 0.5f);
	}

	x = (width - w) / 2;
	y = (height - h) / 2;

	GL_viewport_x = x;
	GL_viewport_y = y;
	GL_viewport_w = w;
	GL_viewport_h = h;
	GL_viewport_scale_w = w / i2fl(gr_screen.max_w);
	GL_viewport_scale_h = h / i2fl(gr_screen.max_h);

	glViewport(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h);

	int left = 0, right = GL_viewport_w;
	int top = 0, bottom = GL_viewport_h;
	int far = 1.0f, near = 0.0f;

	float a = 2.0f / (right - left);
	float b = 2.0f / (top - bottom);
	float c = -2.0f / (far - near);

	float tx = - (right + left)/(right - left);
	float ty = - (top + bottom)/(top - bottom);
	float tz = - (far + near)/(far - near);

	float ortho[16] = {
		a, 0, 0, 0,
		0, b, 0, 0,
		0, 0, c, 0,
		tx, ty, tz, 1
	};

	extern GLuint basicTexture;
	GLint loc = glGetUniformLocation(basicTexture, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	// clear screen once to fix issues with edges on non-4:3
	gr_opengl_clear();
}
