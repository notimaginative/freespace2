/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "osregistry.h"
#include "gropengl.h"
#include "gropengl1.h"
#include "gropenglinternal.h"
#include "2d.h"
#include "bmpman.h"
#include "grinternal.h"
#include "cmdline.h"
#include "mouse.h"


bool OGL_inited = false;
int GL_version = 0;

SDL_Window *GL_window = NULL;
SDL_GLContext GL_context;

static int FSAA = 0;

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
	if (nelems < 0) {
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

static void opengl_set_variables()
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

void gr_opengl_set_viewport(int width, int height)
{
	int w, h, x, y;

	float ratio = gr_screen.max_w / i2fl(gr_screen.max_h);

	w = width;
	h = i2fl((width / ratio) + 0.5f);

	if (h > height) {
		h = height;
		w = i2fl((height * ratio) + 0.5f);
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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GL_viewport_w, GL_viewport_h, 0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(GL_viewport_scale_w, GL_viewport_scale_h, 1.0f);
}

void gr_opengl_force_windowed()
{
	SDL_SetWindowFullscreen(GL_window, 0);
}

void gr_opengl_force_fullscreen()
{
	int fullscreen = os_config_read_uint(NULL, "Fullscreen", 1);
	int flag = SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (fullscreen == 2) {
		flag = SDL_WINDOW_FULLSCREEN;
	}

	SDL_SetWindowFullscreen(GL_window, flag);
}

void gr_opengl_set_color_fast(color *dst)
{
	if ( dst->screen_sig != gr_screen.signature )	{
		if ( dst->is_alphacolor )       {
			gr_opengl_init_alphacolor( dst, dst->red, dst->green, dst->blue, dst->alpha, dst->ac_type );
		} else {
			gr_opengl_init_color( dst, dst->red, dst->green, dst->blue );
		}
	}
	gr_screen.current_color = *dst;
}

void gr_opengl_get_color( int * r, int * g, int * b )
{
	if (r) *r = gr_screen.current_color.red;
	if (g) *g = gr_screen.current_color.green;
	if (b) *b = gr_screen.current_color.blue;
}

void gr_opengl_init_color(color *c, int r, int g, int b)
{
	c->screen_sig = gr_screen.signature;
	c->red = (unsigned char)r;
	c->green = (unsigned char)g;
	c->blue = (unsigned char)b;
	c->alpha = 255;
	c->ac_type = AC_TYPE_NONE;
	c->alphacolor = -1;
	c->is_alphacolor = 0;
	c->magic = 0xAC01;
}

void gr_opengl_init_alphacolor( color *clr, int r, int g, int b, int alpha, int type )
{
	if ( r < 0 ) r = 0; else if ( r > 255 ) r = 255;
	if ( g < 0 ) g = 0; else if ( g > 255 ) g = 255;
	if ( b < 0 ) b = 0; else if ( b > 255 ) b = 255;
	if ( alpha < 0 ) alpha = 0; else if ( alpha > 255 ) alpha = 255;

	gr_opengl_init_color( clr, r, g, b );

	clr->alpha = (unsigned char)alpha;
	clr->ac_type = (ubyte)type;
	clr->alphacolor = -1;
	clr->is_alphacolor = 1;
}

void gr_opengl_set_color( int r, int g, int b )
{
	SDL_assert((r >= 0) && (r < 256));
	SDL_assert((g >= 0) && (g < 256));
	SDL_assert((b >= 0) && (b < 256));

	gr_opengl_init_color( &gr_screen.current_color, r, g, b );
}

void gr_opengl_set_clear_color(int r, int g, int b)
{
	gr_opengl_init_color(&gr_screen.current_clear_color, r, g, b);
}

void gr_opengl_set_bitmap(int bitmap_num, int alphablend_mode, int bitblt_mode, float alpha, int sx, int sy)
{
	gr_screen.current_alpha = alpha;
	gr_screen.current_alphablend_mode = alphablend_mode;
	gr_screen.current_bitblt_mode = bitblt_mode;
	gr_screen.current_bitmap = bitmap_num;

	gr_screen.current_bitmap_sx = sx;
	gr_screen.current_bitmap_sy = sy;
}

void gr_opengl_create_shader(shader * shade, float r, float g, float b, float c )
{
	shade->screen_sig = gr_screen.signature;
	shade->r = r;
	shade->g = g;
	shade->b = b;
	shade->c = c;
}

void gr_opengl_set_shader( shader * shade )
{
	if ( shade )	{
		if (shade->screen_sig != gr_screen.signature)	{
			gr_create_shader( shade, shade->r, shade->g, shade->b, shade->c );
		}
		gr_screen.current_shader = *shade;
	} else {
		gr_create_shader( &gr_screen.current_shader, 0.0f, 0.0f, 0.0f, 0.0f );
	}
}

int gr_opengl_zbuffer_get()
{
	if ( !Gr_global_zbuffering ) {
		return GR_ZBUFF_NONE;
	}

	return Gr_zbuffering_mode;
}

int gr_opengl_zbuffer_set(int mode)
{
	int tmp = Gr_zbuffering_mode;

	Gr_zbuffering_mode = mode;

	if (Gr_zbuffering_mode == GR_ZBUFF_NONE) {
		Gr_zbuffering = 0;
	} else {
		Gr_zbuffering = 1;
	}

	return tmp;
}

void gr_opengl_cleanup()
{
	opengl1_cleanup();

	opengl_free_render_buffer();
}

void gr_opengl_init()
{
	if ( OGL_inited )	{
		gr_opengl_cleanup();
		OGL_inited = false;
	}

	mprintf(( "Initializing OpenGL graphics device...\n" ));

	OGL_inited = true;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		Error(LOCATION, "Couldn't init SDL: %s", SDL_GetError());
	}

	int a = 1, r = 5, g = 5, b = 5, bpp = 16, db = 1;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, r);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, g);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, b);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, a);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, bpp);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, db);

	FSAA = os_config_read_uint(NULL, "FSAA", 2);

	if (FSAA) {
	    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, FSAA);
	}

	GL_window = SDL_CreateWindow(Osreg_title, SDL_WINDOWPOS_CENTERED,
						SDL_WINDOWPOS_CENTERED,
						gr_screen.max_w, gr_screen.max_h, SDL_WINDOW_OPENGL);

	if ( !GL_window ) {
		Error(LOCATION, "Couldn't create window: %s\n", SDL_GetError());
	}

	GL_context = SDL_GL_CreateContext(GL_window);

	const char *gl_version = (const char*)glGetString(GL_VERSION);
	int v_major = 0, v_minor = 0;

	sscanf(gl_version, "%d.%d", &v_major, &v_minor);

	GL_version = (v_major * 10) + v_minor;

	// version check, require 1.2+ for sake of simplicity
	if (GL_version < 12) {
		Error(LOCATION, "Minimum OpenGL version is 1.2!");
	}

	mprintf(("  Vendor   : %s\n", glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", gl_version));

	mprintf(("  Attributes requested: ARGB %d%d%d%d, BPP %d, DB %d, AA %d\n", a, r, g, b, bpp, db, FSAA));

	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &bpp);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &db);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &FSAA);

	mprintf(("  Attributes received : ARGB %d%d%d%d, BPP %d, DB %d, AA %d\n", a, r, g, b, bpp, db, FSAA));


	SDL_DisableScreenSaver();
	SDL_ShowCursor(0);

	// initial setup viewport
	gr_opengl_set_viewport(gr_screen.max_w, gr_screen.max_h);

	// maybe go fullscreen - should be done *after* initial viewport setup
	int fullscreen = os_config_read_uint(NULL, "Fullscreen", 1);
	if ( !Cmdline_window && (fullscreen || Cmdline_fullscreen) ) {
		int flag = SDL_WINDOW_FULLSCREEN_DESKTOP;

		if (fullscreen == 2) {
			flag = SDL_WINDOW_FULLSCREEN;
		}

		SDL_SetWindowFullscreen(GL_window, flag);
	}

	// set up generic variables before further init() calls
	opengl_set_variables();

	// main GL init
	opengl1_init();

	mprintf(("\n"));


	switch (bpp) {
		case 15:
		case 16:
			gr_screen.bits_per_pixel = 16;
			gr_screen.bytes_per_pixel = 2;

			// screen values
			Gr_red.bits = 5;
			Gr_red.shift = 10;
			Gr_red.scale = 8;
			Gr_red.mask = 0x7C00;

			Gr_green.bits = 5;
			Gr_green.shift = 5;
			Gr_green.scale = 8;
			Gr_green.mask = 0x3E0;

			Gr_blue.bits = 5;
			Gr_blue.shift = 0;
			Gr_blue.scale = 8;
			Gr_blue.mask = 0x1F;

			Gr_alpha.bits = 1;
			Gr_alpha.shift = 15;
			Gr_alpha.scale = 255;
			Gr_alpha.mask = 0x8000;

			break;

		case 24:
		case 32:
			gr_screen.bits_per_pixel = 32;
			gr_screen.bytes_per_pixel = 4;

			// screen values
			Gr_red.bits = 8;
			Gr_red.shift = 16;
			Gr_red.scale = 1;
			Gr_red.mask = 0xff0000;

			Gr_green.bits = 8;
			Gr_green.shift = 8;
			Gr_green.scale = 1;
			Gr_green.mask = 0xff00;

			Gr_blue.bits = 8;
			Gr_blue.shift = 0;
			Gr_blue.scale = 1;
			Gr_blue.mask = 0xff;

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
	// texture values, always 1555 - 16-bit
	Gr_t_red.mask = 0x7C00;
	Gr_t_red.shift = 10;
	Gr_t_red.scale = 8;

	Gr_t_green.mask = 0x3E0;
	Gr_t_green.shift = 5;
	Gr_t_green.scale = 8;

	Gr_t_blue.mask = 0x1F;
	Gr_t_blue.shift = 0;
	Gr_t_blue.scale = 8;

	Gr_t_alpha.mask = 0x8000;
	Gr_t_alpha.scale = 255;
	Gr_t_alpha.shift = 15;

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
	Mouse_hidden--;
}
