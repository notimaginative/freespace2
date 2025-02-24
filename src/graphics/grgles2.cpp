/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <SDL3/SDL_opengles2.h>

#include "grgles2.h"
#include "grgles2internal.h"
#include "2d.h"
#include "mouse.h"
#include "pstypes.h"
#include "bmpman.h"
#include "grinternal.h"
#include "osregistry.h"
#include "cfile.h"
#include "osapi.h"
#include "cmdline.h"


static bool GLES2_inited = false;

static SDL_Window *GLES2_window = nullptr;
static SDL_GLContext GLES2_context = nullptr;

static GLuint FB_texture = 0;
static GLuint FB_id = 0;
static GLuint FB_rb_id = 0;

static GLuint GL_saved_screen_tex = 0;
static GLuint GL_stream_tex = 0;

static volatile int GLES2_activate = 0;
static volatile int GLES2_deactivate = 0;

int GLES2_viewport_x = 0;
int GLES2_viewport_y = 0;
int GLES2_viewport_w = 640;
int GLES2_viewport_h = 480;
float GLES2_viewport_scale_w = 1.0f;
float GLES2_viewport_scale_h = 1.0f;
int GLES2_min_texture_width = 0;
int GLES2_max_texture_width = 0;
int GLES2_min_texture_height = 0;
int GLES2_max_texture_height = 0;

static rb_t *render_buffer = nullptr;
static size_t render_buffer_size = 0;

// GLES2 function prototypes
PFNGLBINDBUFFERPROC pglBindFramebuffer = nullptr;
PFNGLBINDRENDERBUFFERPROC pglBindRenderbuffer = nullptr;
PFNGLBLENDFUNCSEPARATEPROC pglBlendFuncSeparate = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC pglCheckFramebufferStatus = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC pglDeleteFramebuffers = nullptr;
PFNGLDELETERENDERBUFFERSPROC pglDeleteRenderbuffers = nullptr;
PFNGLDEPTHRANGEFPROC pglDepthRangef = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC pglDisableVertexAttribArray = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC pglFramebufferRenderbuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC pglFramebufferTexture2D = nullptr;
PFNGLGENFRAMEBUFFERSPROC pglGenFramebuffers = nullptr;
PFNGLGENRENDERBUFFERSPROC pglGenRenderbuffers = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC pglRenderbufferStorage = nullptr;
PFNGLVERTEXATTRIB4FPROC pglVertexAttrib4f = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC pglVertexAttribPointer = nullptr;
PFNGLGENERATEMIPMAPPROC pglGenerateMipmap = nullptr;
PFNGLATTACHSHADERPROC pglAttachShader = nullptr;
PFNGLBINDATTRIBLOCATIONPROC pglBindAttribLocation = nullptr;
PFNGLCOMPILESHADERPROC pglCompileShader = nullptr;
PFNGLCREATEPROGRAMPROC pglCreateProgram = nullptr;
PFNGLCREATESHADERPROC pglCreateShader = nullptr;
PFNGLDELETEPROGRAMPROC pglDeleteProgram = nullptr;
PFNGLDELETESHADERPROC pglDeleteShader = nullptr;
PFNGLGETPROGRAMIVPROC pglGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC pglGetProgramInfoLog = nullptr;
PFNGLGETSHADERIVPROC pglGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC pglGetShaderInfoLog = nullptr;
PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation = nullptr;
PFNGLLINKPROGRAMPROC pglLinkProgram = nullptr;
PFNGLSHADERSOURCEPROC pglShaderSource = nullptr;
PFNGLUNIFORMMATRIX4FVPROC pglUniformMatrix4fv = nullptr;
PFNGLUSEPROGRAMPROC pglUseProgram = nullptr;

static gr_alpha_blend GL_current_alpha_blend = (gr_alpha_blend) -1;
static gr_zbuffer_type GL_current_zbuffer_type = (gr_zbuffer_type) -1;

void gles2_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt)
{
	gles2_set_texture_state(ts);

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
				pglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
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

rb_t *gles2_get_render_buffer(size_t num_elems)
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

	render_buffer = reinterpret_cast<rb_t *>(malloc(sizeof(rb_t) * num_elems));
	render_buffer_size = num_elems;

	return render_buffer;
}

static void gles2_free_render_buffer()
{
	if (render_buffer) {
		free(render_buffer);
		render_buffer = nullptr;
		render_buffer_size = 0;
	}
}

static bool gles2_set_variables()
{
	GLES2_min_texture_height = 16;
	GLES2_min_texture_width = 16;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GLES2_max_texture_width);
	GLES2_max_texture_height = GLES2_max_texture_width;

	gr_screen.use_sections = 0;

	// we don't support sections here, so if that's a problem then fail
	return (GLES2_max_texture_width >= 1024);
}

static int gles2_create_framebuffer()
{
	// create texture
	glGenTextures(1, &FB_texture);
	glBindTexture(GL_TEXTURE_2D, FB_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gr_screen.max_w, gr_screen.max_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	// create renderbuffer
	pglGenRenderbuffers(1, &FB_rb_id);
	pglBindRenderbuffer(GL_RENDERBUFFER, FB_rb_id);

	pglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, gr_screen.max_w, gr_screen.max_h);

	pglBindRenderbuffer(GL_RENDERBUFFER, 0);

	// create framebuffer
	pglGenFramebuffers(1, &FB_id);
	pglBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// attach texture and renderbuffer
	pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FB_texture, 0);
	pglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FB_rb_id);

	GLenum status = pglCheckFramebufferStatus(GL_FRAMEBUFFER);

	pglBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		if (FB_texture) {
			glDeleteTextures(1, &FB_texture);
			FB_texture = 0;
		}

		if (FB_rb_id) {
			pglDeleteRenderbuffers(1, &FB_rb_id);
			FB_rb_id = 0;
		}

		if (FB_id) {
			pglDeleteFramebuffers(1, &FB_id);
			FB_id = 0;
		}

		return 0;
	}

	return 1;
}

static void gles2_init_func_pointers()
{
	gr_screen.gf_flip = gr_gles2_flip;
	gr_screen.gf_set_clip = gr_gles2_set_clip;
	gr_screen.gf_reset_clip = gr_gles2_reset_clip;

	gr_screen.gf_clear = gr_gles2_clear;

	gr_screen.gf_aabitmap = gr_gles2_aabitmap;
	gr_screen.gf_aabitmap_ex = gr_gles2_aabitmap_ex;

	gr_screen.gf_rect = gr_gles2_rect;
	gr_screen.gf_shade = gr_gles2_shade;
	gr_screen.gf_string = gr_gles2_string;
	gr_screen.gf_circle = gr_gles2_circle;

	gr_screen.gf_line = gr_gles2_line;
	gr_screen.gf_aaline = gr_gles2_aaline;
	gr_screen.gf_pixel = gr_gles2_pixel;
	gr_screen.gf_scaler = gr_gles2_scaler;
	gr_screen.gf_tmapper = gr_gles2_tmapper;

	gr_screen.gf_gradient = gr_gles2_gradient;

	gr_screen.gf_print_screen = gr_gles2_print_screen;

	gr_screen.gf_fade_in = gr_gles2_fade_in;
	gr_screen.gf_fade_out = gr_gles2_fade_out;
	gr_screen.gf_flash = gr_gles2_flash;

	gr_screen.gf_zbuffer_clear = gr_gles2_zbuffer_clear;

	gr_screen.gf_save_screen = gr_gles2_save_screen;
	gr_screen.gf_restore_screen = gr_gles2_restore_screen;
	gr_screen.gf_free_screen = gr_gles2_free_screen;

	gr_screen.gf_dump_frame_start = gr_gles2_dump_frame_start;
	gr_screen.gf_dump_frame_stop = gr_gles2_dump_frame_stop;
	gr_screen.gf_dump_frame = gr_gles2_dump_frame;

	gr_screen.gf_stream_start = gr_gles2_stream_start;
	gr_screen.gf_stream_frame = gr_gles2_stream_frame;
	gr_screen.gf_stream_stop = gr_gles2_stream_stop;

	gr_screen.gf_set_gamma = gr_gles2_set_gamma;

	gr_screen.gf_lock = gr_gles2_lock;
	gr_screen.gf_unlock = gr_gles2_unlock;

	gr_screen.gf_fog_set = gr_gles2_fog_set;

	gr_screen.gf_get_region = gr_gles2_get_region;

	gr_screen.gf_set_cull = gr_gles2_set_cull;

	gr_screen.gf_cross_fade = gr_gles2_cross_fade;

	gr_screen.gf_preload_init = gr_gles2_preload_init;
	gr_screen.gf_preload = gr_gles2_preload;

	gr_screen.gf_zbias = gr_gles2_zbias;

	gr_screen.gf_set_viewport = gr_gles2_set_viewport;

	gr_screen.gf_activate = gr_gles2_activate;

	gr_screen.gf_release_texture = gr_gles2_release_texture;
}

static bool gles2_init_prototypes()
{
	#define GET_PROC(type, func)	\
		do {	\
			p##func = reinterpret_cast<type>(SDL_GL_GetProcAddress(#func));	\
			if ( !(p##func) ) {	\
				mprintf(("  Couldn't load GLES2 function %s: %s", #func, SDL_GetError()));	\
				return false;	\
			}	\
		} while(false);
	
	GET_PROC(PFNGLBINDBUFFERPROC, glBindFramebuffer)
	GET_PROC(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer)
	GET_PROC(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate)
	GET_PROC(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus)
	GET_PROC(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)
	GET_PROC(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers)
	GET_PROC(PFNGLDEPTHRANGEFPROC, glDepthRangef)
	GET_PROC(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
	GET_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)
	GET_PROC(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer)
	GET_PROC(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)
	GET_PROC(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)
	GET_PROC(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers)
	GET_PROC(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage)
	GET_PROC(PFNGLVERTEXATTRIB4FPROC, glVertexAttrib4f)
	GET_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)
	GET_PROC(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap)
	GET_PROC(PFNGLATTACHSHADERPROC, glAttachShader)
	GET_PROC(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation)
	GET_PROC(PFNGLCOMPILESHADERPROC, glCompileShader)
	GET_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram)
	GET_PROC(PFNGLCREATESHADERPROC, glCreateShader)
	GET_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram)
	GET_PROC(PFNGLDELETESHADERPROC, glDeleteShader)
	GET_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv)
	GET_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)
	GET_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv)
	GET_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)
	GET_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)
	GET_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram)
	GET_PROC(PFNGLSHADERSOURCEPROC, glShaderSource)
	GET_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)
	GET_PROC(PFNGLUSEPROGRAMPROC, glUseProgram)

	return true;
}

void gr_gles2_reset_clip()
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

uint gr_gles2_lock()
{
	return 1;
}

void gr_gles2_unlock()
{
}

void gr_gles2_clear()
{
	glClearColor(gr_screen.current_clear_color.red / 255.0f,
		gr_screen.current_clear_color.green / 255.0f,
		gr_screen.current_clear_color.blue / 255.0f, 1.0f);

	glClear( GL_COLOR_BUFFER_BIT );
}


void gr_gles2_zbias(int bias)
{
	if (bias) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, GLfloat(-bias));
	} else {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void gr_gles2_set_cull(int cull)
{
	if (cull) {
		glEnable (GL_CULL_FACE);
		glFrontFace (GL_CCW);
	} else {
		glDisable (GL_CULL_FACE);
	}
}

void gr_gles2_activate(int active)
{
	if (active) {
		GLES2_activate++;
	} else {
		GLES2_deactivate++;
	}
}

void gr_gles2_cleanup()
{
	if ( !GLES2_inited ) {
		return;
	}

	if (GLES2_window && pglBindFramebuffer) {
		pglBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if (FB_texture) {
		glDeleteTextures(1, &FB_texture);
		FB_texture = 0;
	}

	if (FB_rb_id) {
		pglDeleteRenderbuffers(1, &FB_rb_id);
		FB_rb_id = 0;
	}

	if (FB_id) {
		pglDeleteFramebuffers(1, &FB_id);
		FB_id = 0;
	}

	gles2_tcache_cleanup();
	gles2_shader_cleanup();

	gles2_free_render_buffer();

	if (GLES2_context) {
		SDL_GL_DestroyContext(GLES2_context);
		GLES2_context = nullptr;
	}

	if (GLES2_window) {
		os_set_window(nullptr);
		SDL_DestroyWindow(GLES2_window);
		GLES2_window = nullptr;
	}

	GLES2_inited = false;
}

void gr_gles2_init()
{
	if (GLES2_inited) {
		return;
	}

	mprintf(( "Initializing OpenGL ES2 graphics device...\n" ));

	GLES2_inited = true;

	if ( !SDL_InitSubSystem(SDL_INIT_VIDEO) ) {
		Error(LOCATION, "Couldn't init SDL: %s", SDL_GetError());
	}

	Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

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

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	GLES2_window = SDL_CreateWindow(os_get_title(),
									gr_screen.max_w,
									gr_screen.max_h,
									window_flags);

	if ( !GLES2_window ) {
		// This will generally happen when the GLES2 library isn't available. In
		// which case we should automatically fall back to "safe mode".
		mprintf(("  Window creation failed! \n    %s\n", SDL_GetError()));
		mprintf(("  Restarting graphics in safe mode...\n"));
		gr_init(true);	// will call _cleanup() for us
		return;
	}

	os_set_window(GLES2_window);

	SDL_SetWindowMinimumSize(GLES2_window, 640, 480);

	GLES2_context = SDL_GL_CreateContext(GLES2_window);

	if ( !GLES2_context ) {
		mprintf(("  GLES2 context creation failed! \n    %s\n", SDL_GetError()));
		mprintf(("  Restarting graphics in safe mode...\n"));
		gr_init(true);	// will call _cleanup() for us
		return;
	}

	mprintf(("  Vendor   : %s\n", glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", glGetString(GL_VERSION)));

	// first thing after context is ready, init gles2 function prototypes
	if ( !gles2_init_prototypes() ) {
		mprintf(("  Restarting graphics in safe mode...\n"));
		gr_init(true);	// will call _cleanup() for us
		return;
	}

	if ( !gles2_set_variables() ) {
		mprintf(("  Hardware/Software requirements not met!\n"));
		mprintf(("  Restarting graphics in safe mode...\n"));
		gr_init(true);	// will call _cleanup() for us
		return;
	}

	// initial viewport setup
	gr_gles2_set_viewport(gr_screen.max_w, gr_screen.max_h);

	gles2_init_func_pointers();
	gles2_tcache_init();

	if ( !gles2_shader_init() ) {
		Error(LOCATION, "GLES2 shader init failure!");
	}

	if ( !gles2_create_framebuffer() ) {
		Error(LOCATION, "GLES2 framebuffer init failure!");
	}

	glEnable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	pglDepthRangef(0.0f, 1.0f);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glFlush();

	gr_gles2_clear();
	gr_gles2_set_cull(1);

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

	SDL_StopTextInput(os_get_window());
	SDL_DisableScreenSaver();
	SDL_HideCursor();

	// maybe go fullscreen - should be done *after* main GL init
	int fullscreen = os_config_read_uint("Video", "Fullscreen", 1);
	if ( !Cmdline_window && (fullscreen || Cmdline_fullscreen) ) {
		gr_force_fullscreen();
	}

	// if fullscreen or using resizable window then poll for window events
	if ( gr_screen.fullscreen || (window_flags & SDL_WINDOW_RESIZABLE) ) {
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

void gr_gles2_flip()
{
	if ( !GLES2_inited ) {
		return;
	}

	pglBindFramebuffer(GL_FRAMEBUFFER, 0);

	gr_gles2_reset_clip();

	// set viewport to window size
	glViewport(GLES2_viewport_x, GLES2_viewport_y, GLES2_viewport_w, GLES2_viewport_h);

	glClear(GL_COLOR_BUFFER_BIT);

	{
		float x = 0.0f;
		float y = 0.0f;
		float w = i2fl(GLES2_viewport_w);
		float h = i2fl(GLES2_viewport_h);

		const float tex_coord[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };
		const float ver_coord[] = { x, y, x, h, w, y, w, h };

		gles2_shader_use(PROG_WINDOW);

		pglEnableVertexAttribArray(SDRI_POSITION);
		pglVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

		pglEnableVertexAttribArray(SDRI_TEXCOORD);
		pglVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

		glBindTexture(GL_TEXTURE_2D, FB_texture);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, 0);

		pglDisableVertexAttribArray(SDRI_TEXCOORD);
		pglDisableVertexAttribArray(SDRI_POSITION);
	}

	mouse_eval_deltas();

	if ( mouse_is_visible() ) {
		int mx, my;

		mouse_get_pos(&mx, &my);

		if ( gles2_tcache_set(Gr_cursor, TCACHE_TYPE_BITMAP_INTERFACE) ) {
			gles2_set_state(TEXTURE_SOURCE_DECAL, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE);

			int bw, bh;
			bm_get_info(Gr_cursor, &bw, &bh);

			float x = i2fl(mx) * GLES2_viewport_scale_w;
			float y = i2fl(my) * GLES2_viewport_scale_h;
			float w = x + (bw * GLES2_viewport_scale_w);
			float h = y + (bh * GLES2_viewport_scale_h);

			const float tex_coord[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
			const float ver_coord[] = { x, y, x, h, w, y, w, h };

			gles2_shader_use(PROG_WINDOW);

			pglEnableVertexAttribArray(SDRI_POSITION);
			pglVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

			pglEnableVertexAttribArray(SDRI_TEXCOORD);
			pglVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			pglDisableVertexAttribArray(SDRI_TEXCOORD);
			pglDisableVertexAttribArray(SDRI_POSITION);
		}
#ifndef NDEBUG
		else {
			gr_set_color(255,255,255);
			gr_gles2_line(mx, my, mx+7, my + 7);
			gr_gles2_line(mx, my, mx+5, my );
			gr_gles2_line(mx, my, mx, my+5);
		}
#endif
	}

#ifndef NDEBUG
	GLenum error = glGetError();

	if (error != GL_NO_ERROR) {
		mprintf(("!!DEBUG!! OpenGL Error: %d\n", error));
	}
#endif

	SDL_GL_SwapWindow(GLES2_window);

	gles2_tcache_frame();

	pglBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// set viewport to game screen size
	glViewport(0, 0, gr_screen.max_w, gr_screen.max_h);
}

void gr_gles2_set_clip(int x, int y, int w, int h)
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
	glScissor(x, gr_screen.max_h-y-h, w, h);
}

void gr_gles2_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far)
{
	gr_screen.current_fog_mode = fog_mode;

	if (fog_mode == GR_FOGMODE_NONE) {
		return;
	}

	gr_screen.fog_near = fog_near;
	gr_screen.fog_far = fog_far;

	gr_init_color(&gr_screen.current_fog_color, r, g, b);
}

void gr_gles2_zbuffer_clear(int mode)
{
	if (mode) {
		Gr_zbuffering = 1;
		Gr_zbuffering_mode = GR_ZBUFF_FULL;
		Gr_global_zbuffering = 1;

		gles2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_NONE, ZBUFFER_TYPE_FULL);
		glClear(GL_DEPTH_BUFFER_BIT);
	} else {
		Gr_zbuffering = 0;
		Gr_zbuffering_mode = GR_ZBUFF_NONE;
		Gr_global_zbuffering = 0;
	}
}

void gr_gles2_print_screen(const char *filename)
{
	char tmp[MAX_FILENAME_LEN];
	ubyte *buf = NULL;

	SDL_strlcpy( tmp, filename, SDL_arraysize(tmp) );
	SDL_strlcat( tmp, NOX(".tga"), SDL_arraysize(tmp) );

	int b_size = gr_screen.max_w * gr_screen.max_h;

	buf = (ubyte*)malloc(b_size * 4);

	if (buf == NULL) {
		return;
	}

	CFILE *f = cfopen(tmp, "wb", CFILE_NORMAL, CF_TYPE_ROOT);

	if (f == NULL) {
		free(buf);
		return;
	}

	// Write the TGA header
	cfwrite_ubyte( 0, f );	//	IDLength;
	cfwrite_ubyte( 0, f );	//	ColorMapType;
	cfwrite_ubyte( 2, f );	//	ImageType;		// 2 = 24bpp, uncompressed, 10=24bpp rle compressed
	cfwrite_ushort( 0, f );	// CMapStart;
	cfwrite_ushort( 0, f );	//	CMapLength;
	cfwrite_ubyte( 0, f );	// CMapDepth;
	cfwrite_ushort( 0, f );	//	XOffset;
	cfwrite_ushort( 0, f );	//	YOffset;
	cfwrite_ushort( (ushort)gr_screen.max_w, f );	//	Width;
	cfwrite_ushort( (ushort)gr_screen.max_h, f );	//	Height;
	cfwrite_ubyte( 24, f );	//PixelDepth;
	cfwrite_ubyte( 0, f );	//ImageDesc;

	memset(buf, 0, b_size * 4);

	glReadPixels(0, 0, gr_screen.max_w, gr_screen.max_h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	int b_offset = 0;

	// convert from rgba to bgr
	for (int i = 0; i < (b_size * 4); i += 4) {
		ubyte r = buf[i];
		ubyte g = buf[i+1];
		ubyte b = buf[i+2];

		buf[b_offset++] = b;
		buf[b_offset++] = g;
		buf[b_offset++] = r;
	}

	cfwrite(buf, b_size * 3, 1, f);

	cfclose(f);

	free(buf);
}

void gr_gles2_fade_in(int instantaneous)
{

}

void gr_gles2_fade_out(int instantaneous)
{

}

void gr_gles2_get_region(int, int w, int h, ubyte *data)
{
	gles2_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	glReadPixels(0, gr_screen.max_h-h-1, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

int gr_gles2_save_screen()
{
	gr_gles2_reset_clip();

	if (GL_saved_screen_tex) {
		mprintf(( "Screen already saved!\n" ));
		return -1;
	}

	glGenTextures(1, &GL_saved_screen_tex);

	if ( !GL_saved_screen_tex ) {
		mprintf(( "Couldn't create texture for saved screen!\n" ));
		return -1;
	}

	glBindTexture(GL_TEXTURE_2D, GL_saved_screen_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0,
			gr_screen.max_w, gr_screen.max_h, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

void gr_gles2_restore_screen(int)
{
	gr_gles2_reset_clip();

	if ( !GL_saved_screen_tex ) {
		gr_gles2_clear();
		return;
	}

	float x = 0.0f;
	float y = 0.0f;
	float w = i2fl(gr_screen.max_w);
	float h = i2fl(gr_screen.max_h);

	const float tex_coord[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };	// y-flipped
	const float ver_coord[] = { x, y, x, h, w, y, w, h };

	gles2_shader_use(PROG_TEX);

	pglVertexAttrib4f(SDRI_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

	pglEnableVertexAttribArray(SDRI_POSITION);
	pglVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

	pglEnableVertexAttribArray(SDRI_TEXCOORD);
	pglVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

	glBindTexture(GL_TEXTURE_2D, GL_saved_screen_tex);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	pglDisableVertexAttribArray(SDRI_TEXCOORD);
	pglDisableVertexAttribArray(SDRI_POSITION);
}

void gr_gles2_free_screen(int)
{
	if (GL_saved_screen_tex) {
		glDeleteTextures(1, &GL_saved_screen_tex);
		GL_saved_screen_tex = 0;
	}
}

void gr_gles2_dump_frame_start(int first_frame, int frames_between_dumps)
{

}

void gr_gles2_dump_frame_stop()
{

}

void gr_gles2_dump_frame()
{

}

static int GL_stream_w = 0;
static int GL_stream_h = 0;

static rb_t GL_stream[4];

void gr_gles2_stream_start(int x, int y, int w, int h)
{
	if (GL_stream_tex) {
		return;
	}

	glGenTextures(1, &GL_stream_tex);

	glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	uint scale = os_config_read_uint("Video", "ScaleMovies", 1);

	int sx, sy;
	int sw, sh;

	if (x < 0) {
		sx = scale ? 0 : ((gr_screen.max_w - w) / 2);
	} else {
		sx = x;
	}

	float h_factor = scale ? (gr_screen.max_w / i2fl(w)) : 1.0f;

	if (y < 0) {
		sy = (gr_screen.max_h - fl2i(h * h_factor)) / 2;
	} else {
		sy = y;
	}

	GL_stream_w = w;
	GL_stream_h = h;

	if (scale) {
		sw = gr_screen.max_w - (sx * 2);
		sh = gr_screen.max_h - (sy * 2);
	} else {
		sw = w;
		sh = h;
	}

	GL_stream[0].x = i2fl(sx);
	GL_stream[0].y = i2fl(sy);
	GL_stream[0].u = 0.0f;
	GL_stream[0].v = 0.0f;

	GL_stream[1].x = i2fl(sx);
	GL_stream[1].y = i2fl(sy + sh);
	GL_stream[1].u = 0.0f;
	GL_stream[1].v = 1.0f;

	GL_stream[2].x = i2fl(sx + sw);
	GL_stream[2].y = i2fl(sy);
	GL_stream[2].u = 1.0f;
	GL_stream[2].v = 0.0f;

	GL_stream[3].x = i2fl(sx + sw);
	GL_stream[3].y = i2fl(sy + sh);
	GL_stream[3].u = 1.0f;
	GL_stream[3].v = 1.0f;

	glDisable(GL_DEPTH_TEST);
}

void gr_gles2_stream_frame(ubyte *frame)
{
	if ( !GL_stream_tex ) {
		return;
	}

	gles2_shader_use(PROG_TEX);

	pglVertexAttrib4f(SDRI_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

	pglEnableVertexAttribArray(SDRI_POSITION);
	pglVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &GL_stream[0].x);

	pglEnableVertexAttribArray(SDRI_TEXCOORD);
	pglVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &GL_stream[0].u);

	glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GL_stream_w, GL_stream_h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, frame);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	pglDisableVertexAttribArray(SDRI_TEXCOORD);
	pglDisableVertexAttribArray(SDRI_POSITION);
}

void gr_gles2_stream_stop()
{
	if (GL_stream_tex) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &GL_stream_tex);
		GL_stream_tex = 0;

		glEnable(GL_DEPTH_TEST);
	}
}

void gr_gles2_set_viewport(int width, int height)
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

	GLES2_viewport_x = x;
	GLES2_viewport_y = y;
	GLES2_viewport_w = w;
	GLES2_viewport_h = h;
	GLES2_viewport_scale_w = w / i2fl(gr_screen.max_w);
	GLES2_viewport_scale_h = h / i2fl(gr_screen.max_h);

	gr_screen.viewport_offset_x = x;
	gr_screen.viewport_offset_y = y;

	gr_screen.viewport_scale_factor_x = 1.0f / GLES2_viewport_scale_w;
	gr_screen.viewport_scale_factor_y = 1.0f / GLES2_viewport_scale_h;

	gles2_shader_update();
}
