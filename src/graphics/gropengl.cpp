/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "SDL_opengl.h"

#include "pstypes.h"
#include "osregistry.h"
#include "gropengl.h"
#include "gropenglinternal.h"
#include "grgl1.h"
#include "grgl2.h"
#include "grinternal.h"
#include "cmdline.h"
#include "mouse.h"
#include "osapi.h"


bool OGL_inited = false;

SDL_Window *GL_window = NULL;
SDL_GLContext GL_context;

volatile int GL_activate = 0;
volatile int GL_deactivate = 0;

int GL_viewport_x = 0;
int GL_viewport_y = 0;
int GL_viewport_w = 640;
int GL_viewport_h = 480;
float GL_viewport_scale_w = 1.0f;
float GL_viewport_scale_h = 1.0f;
int GL_min_texture_width = 0;
int GL_max_texture_width = 0;
int GL_min_texture_height = 0;
int GL_max_texture_height = 0;

rb_t *render_buffer = NULL;
static size_t render_buffer_size = 0;


void opengl_alloc_render_buffer(unsigned int nelems)
{
	if (nelems < 1) {
		nelems = 1;
	}

	if ( render_buffer && (nelems <= render_buffer_size) ) {
		return;
	}

	if (render_buffer) {
		free(render_buffer);
	}

	render_buffer = (rb_t*) malloc(sizeof(rb_t) * nelems);
	render_buffer_size = nelems;
}

void opengl_free_render_buffer()
{
	if (render_buffer) {
		free(render_buffer);
		render_buffer = NULL;
		render_buffer_size = 0;
	}
}

void opengl_set_variables()
{
	GL_min_texture_height = 16;
	GL_min_texture_width = 16;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GL_max_texture_width);
	GL_max_texture_height = GL_max_texture_width;

	// no texture is larger than 1024, so maybe don't use sections
	if (GL_max_texture_width >= 1024) {
		gr_screen.use_sections = 0;
	}
}

void opengl_init_viewport()
{
	GL_viewport_x = 0;
	GL_viewport_y = 0;
	GL_viewport_w = gr_screen.max_w;
	GL_viewport_h = gr_screen.max_h;
	GL_viewport_scale_w = 1.0f;
	GL_viewport_scale_h = 1.0f;

	glViewport(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h);
}

void opengl_stuff_fog_value(float z, float *f_val)
{
	float f_float;

	if ( !f_val ) {
		return;
	}

	f_float = 1.0f - ((gr_screen.fog_far - z) / (gr_screen.fog_far - gr_screen.fog_near));

	if (f_float < 0.0f) {
		f_float = 0.0f;
	} else if (f_float > 1.0f) {
		f_float = 1.0f;
	}

	*f_val = f_float;
}

void gr_opengl_clear()
{
	glClearColor(gr_screen.current_clear_color.red / 255.0f,
		gr_screen.current_clear_color.green / 255.0f,
		gr_screen.current_clear_color.blue / 255.0f, 1.0f);

	glClear( GL_COLOR_BUFFER_BIT );
}

void gr_opengl_reset_clip()
{
	gr_screen.offset_x = 0;
	gr_screen.offset_y = 0;
	gr_screen.clip_left = 0;
	gr_screen.clip_top = 0;
	gr_screen.clip_right = gr_screen.max_w - 1;
	gr_screen.clip_bottom = gr_screen.max_h - 1;
	gr_screen.clip_width = gr_screen.max_w;
	gr_screen.clip_height = gr_screen.max_h;

	glDisable(GL_SCISSOR_TEST);
}

uint gr_opengl_lock()
{
	return 1;
}

void gr_opengl_unlock()
{
}

void gr_opengl_zbias(int bias)
{
	if (bias) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, GLfloat(-bias));
	} else {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void gr_opengl_set_cull(int cull)
{
	if (cull) {
		glEnable (GL_CULL_FACE);
		glFrontFace (GL_CCW);
	} else {
		glDisable (GL_CULL_FACE);
	}
}

void gr_opengl_activate(int active)
{
	if (active) {
		GL_activate++;
	} else {
		GL_deactivate++;
	}
}

void gr_opengl_cleanup()
{
	opengl1_cleanup();
	opengl2_cleanup();

	opengl_free_render_buffer();

	os_set_window(NULL);

	SDL_DestroyWindow(GL_window);
	GL_window = NULL;

	OGL_inited = false;
}

void gr_opengl_init()
{
	if ( OGL_inited )	{
		gr_opengl_cleanup();
	}

	mprintf(( "Initializing OpenGL graphics device...\n" ));

	OGL_inited = true;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		Error(LOCATION, "Couldn't init SDL: %s", SDL_GetError());
	}

	GL_window = SDL_CreateWindow(os_get_title(), SDL_WINDOWPOS_CENTERED,
						SDL_WINDOWPOS_CENTERED,
						gr_screen.max_w, gr_screen.max_h, SDL_WINDOW_OPENGL);

	if ( !GL_window ) {
		Error(LOCATION, "Couldn't create window: %s\n", SDL_GetError());
	}

	os_set_window(GL_window);

	int a = 1, r = 5, g = 5, b = 5, bpp = 16;
	int FSAA = os_config_read_uint("Video", "AntiAlias", 0);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, r);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, g);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, b);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, a);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, bpp);

	if (FSAA) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, FSAA);
	}

	int rc = 1;

	// try GL 2 first, then fall back to GL 1
	if ( !opengl2_init() ) {
		rc = opengl1_init();
	}

	if ( !rc ) {
		Error(LOCATION, "Unable to initialize OpenGL renderer!\n");
	}

	mprintf(("  Attributes requested : ARGB %d%d%d%d, BPP %d, AA %d\n",
			 a, r, g, b, bpp, FSAA));

	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &bpp);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &FSAA);

	mprintf(("  Attributes received  : ARGB %d%d%d%d, BPP %d, AA %d\n",
			 a, r, g, b, bpp, FSAA));
	mprintf(("\n"));

	SDL_StopTextInput();
	SDL_DisableScreenSaver();
	SDL_ShowCursor(0);

	// maybe go fullscreen - should be done *after* main GL init
	int fullscreen = os_config_read_uint("Video", "Fullscreen", 1);
	if ( !Cmdline_window && (fullscreen || Cmdline_fullscreen) ) {
		gr_force_fullscreen();
		// poll for window events
		os_poll();
	}

	switch (bpp) {
		case 15:
		case 16:
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

			break;

		case 24:
		case 32:
			gr_screen.bits_per_pixel = 32;
			gr_screen.bytes_per_pixel = 4;

			// screen values
			Gr_red.bits = 8;
			Gr_red.shift = 0;
			Gr_red.scale = 1;
			Gr_red.mask = 0xff;

			Gr_green.bits = 8;
			Gr_green.shift = 8;
			Gr_green.scale = 1;
			Gr_green.mask = 0xff00;

			Gr_blue.bits = 8;
			Gr_blue.shift = 16;
			Gr_blue.scale = 1;
			Gr_blue.mask = 0xff0000;

			Gr_alpha.bits = 8;
			Gr_alpha.shift = 24;
			Gr_alpha.scale = 1;
			Gr_alpha.mask = 0xff000000;

			break;

		default:
			Int3();	// Illegal bpp
			break;
	}

	// DDOI - set these so no one else does!
	// texture values, always 5551 - 16-bit
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
	Mouse_hidden--;
}
