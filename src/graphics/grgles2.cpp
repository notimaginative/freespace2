/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#define SDL_USE_BUILTIN_OPENGL_DEFINITIONS
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
#include "renderbuffer.h"


static bool GLES2_inited = false;

static SDL_Window *GLES2_window = nullptr;
static SDL_GLContext GLES2_context = nullptr;

static GLuint FB_texture = 0;
static GLuint FB_id = 0;
static GLuint FB_rb_id = 0;

static GLuint GL_saved_screen_tex = 0;
static GLuint GL_stream_tex = 0;

static int GLES2_activate = 0;
static int GLES2_deactivate = 0;

static int GLES2_res_scale = 0;

GLES2_func_context GLES2_ctx;

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

static gr_alpha_blend GL_current_alpha_blend = (gr_alpha_blend) -1;
static gr_zbuffer_type GL_current_zbuffer_type = (gr_zbuffer_type) -1;

void gles2_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt)
{
	gles2_set_texture_state(ts);

	if (ab != GL_current_alpha_blend) {
		switch (ab) {
			case ALPHA_BLEND_NONE:			// 1*SrcPixel + 0*DestPixel
				GLES2_ctx.glBlendFunc(GL_ONE, GL_ZERO);
				break;
			case ALPHA_BLEND_ADDITIVE:		// 1*SrcPixel + 1*DestPixel
				GLES2_ctx.glBlendFunc(GL_ONE, GL_ONE);
				break;
			case ALPHA_BLEND_ALPHA_ADDITIVE:	// Alpha*SrcPixel + 1*DestPixel
				GLES2_ctx.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case ALPHA_BLEND_ALPHA_BLEND_ALPHA:	// Alpha*SrcPixel + (1-Alpha)*DestPixel
				GLES2_ctx.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case ALPHA_BLEND_ALPHA_BLEND_SRC_COLOR:	// Alpha*SrcPixel + (1-SrcPixel)*DestPixel
				GLES2_ctx.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
				break;
			default:
				break;
		}

		GL_current_alpha_blend = ab;
	}

	if (zt != GL_current_zbuffer_type) {
		switch (zt) {
			case ZBUFFER_TYPE_NONE:
				GLES2_ctx.glDepthFunc(GL_ALWAYS);
				GLES2_ctx.glDepthMask(GL_FALSE);
				break;
			case ZBUFFER_TYPE_READ:
				GLES2_ctx.glDepthFunc(GL_LESS);
				GLES2_ctx.glDepthMask(GL_FALSE);
				break;
			case ZBUFFER_TYPE_WRITE:
				GLES2_ctx.glDepthFunc(GL_ALWAYS);
				GLES2_ctx.glDepthMask(GL_TRUE);
				break;
			case ZBUFFER_TYPE_FULL:
				GLES2_ctx.glDepthFunc(GL_LESS);
				GLES2_ctx.glDepthMask(GL_TRUE);
				break;
			default:
				break;
		}

		GL_current_zbuffer_type = zt;
	}
}

int gles2_res_scale(int val)
{
	if (GLES2_res_scale > 0) {
		return val * GLES2_res_scale;
	}

	return val;
}

float gles2_res_scale(float val)
{
	if (GLES2_res_scale > 0) {
		return val * GLES2_res_scale;
	}

	return val;
}

bool gles2_need_res_scale()
{
	return (GLES2_res_scale > 1);
}

static bool gles2_init_res_scale()
{
	int AA = os_config_read_uint("Video", "AntiAlias", 0);

	// set default first thing
	GLES2_res_scale = 0;

	if (AA < 2) {
		return false;
	}

	switch (AA) {
		case 2:
			GLES2_res_scale = 2;
			break;
		case 4:
			GLES2_res_scale = 3;
			break;
		case 8:
			GLES2_res_scale = 4;
			break;
		case 16:
			GLES2_res_scale = 5;
			break;
		default:
			return false;
	}

	// make sure we aren't scaling beyond the hardware/driver limits
	int tex_max_size = 0, rb_max_size = 0;

	GLES2_ctx.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tex_max_size);
	GLES2_ctx.glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &rb_max_size);

	int max_size = SDL_min(tex_max_size, rb_max_size);

	while ((gr_screen.max_w * GLES2_res_scale) > max_size) {
		--GLES2_res_scale;
	}

	if (GLES2_res_scale < 2) {
		GLES2_res_scale = 0;
		return false;
	}

	return true;
}

static bool gles2_set_variables()
{
	gles2_init_res_scale();

	GLES2_min_texture_height = 16;
	GLES2_min_texture_width = 16;

	GLES2_ctx.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GLES2_max_texture_width);
	GLES2_max_texture_height = GLES2_max_texture_width;

	gr_screen.use_sections = 0;
	gr_screen.use_nondark = 1;

	// we don't support sections here, so if that's a problem then fail
	return (GLES2_max_texture_width >= 1024);
}

static int gles2_create_framebuffer()
{
	// create texture
	GLES2_ctx.glGenTextures(1, &FB_texture);
	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, FB_texture);

	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLES2_ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gles2_res_scale(gr_screen.max_w),
						   gles2_res_scale(gr_screen.max_h), 0, GL_RGBA, GL_UNSIGNED_BYTE,
						   NULL);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	// create renderbuffer
	GLES2_ctx.glGenRenderbuffers(1, &FB_rb_id);
	GLES2_ctx.glBindRenderbuffer(GL_RENDERBUFFER, FB_rb_id);

	GLES2_ctx.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
									gles2_res_scale(gr_screen.max_w),
									gles2_res_scale(gr_screen.max_h));

	GLES2_ctx.glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// create framebuffer
	GLES2_ctx.glGenFramebuffers(1, &FB_id);
	GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// attach texture and renderbuffer
	GLES2_ctx.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FB_texture, 0);
	GLES2_ctx.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FB_rb_id);

	GLenum status = GLES2_ctx.glCheckFramebufferStatus(GL_FRAMEBUFFER);

	GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		if (FB_texture) {
			GLES2_ctx.glDeleteTextures(1, &FB_texture);
			FB_texture = 0;
		}

		if (FB_rb_id) {
			GLES2_ctx.glDeleteRenderbuffers(1, &FB_rb_id);
			FB_rb_id = 0;
		}

		if (FB_id) {
			GLES2_ctx.glDeleteFramebuffers(1, &FB_id);
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
	gr_screen.gf_aalines = gr_gles2_aalines;
	gr_screen.gf_points = gr_gles2_points;
	gr_screen.gf_pixel = gr_gles2_pixel;
	gr_screen.gf_scaler = gr_gles2_scaler;
	gr_screen.gf_aascaler = gr_gles2_aascaler;
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

	gr_screen.gf_push_unscaled_viewport = gr_gles2_push_unscaled_viewport;
	gr_screen.gf_pop_unscaled_viewport = gr_gles2_pop_unscaled_viewport;
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
			GLES2_ctx.func = reinterpret_cast<type>(SDL_GL_GetProcAddress(#func));	\
			if ( !(GLES2_ctx.func) ) {	\
				mprintf(("  Couldn't load GLES2 function %s: %s", #func, SDL_GetError()));	\
				return false;	\
			}	\
		} while(false);


	GET_PROC(PFNGLATTACHSHADERPROC, glAttachShader)
	GET_PROC(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation)
	GET_PROC(PFNGLBINDBUFFERPROC, glBindFramebuffer)
	GET_PROC(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer)
	GET_PROC(PFNGLBINDTEXTUREPROC, glBindTexture)
	GET_PROC(PFNGLBLENDFUNCPROC, glBlendFunc);
	GET_PROC(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate)
	GET_PROC(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus)
	GET_PROC(PFNGLCLEARCOLORPROC, glClearColor)
	GET_PROC(PFNGLCLEARPROC, glClear)
	GET_PROC(PFNGLCOMPILESHADERPROC, glCompileShader)
	GET_PROC(PFNGLCOPYTEXIMAGE2DPROC, glCopyTexImage2D)
	GET_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram)
	GET_PROC(PFNGLCREATESHADERPROC, glCreateShader)
	GET_PROC(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)
	GET_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram)
	GET_PROC(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers)
	GET_PROC(PFNGLDELETESHADERPROC, glDeleteShader)
	GET_PROC(PFNGLDELETETEXTURESPROC, glDeleteTextures)
	GET_PROC(PFNGLDEPTHFUNCPROC, glDepthFunc);
	GET_PROC(PFNGLDEPTHMASKPROC, glDepthMask);
	GET_PROC(PFNGLDEPTHRANGEFPROC, glDepthRangef)
	GET_PROC(PFNGLDISABLEPROC, glDisable)
	GET_PROC(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
	GET_PROC(PFNGLDRAWARRAYSPROC, glDrawArrays)
	GET_PROC(PFNGLENABLEPROC, glEnable)
	GET_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)
	GET_PROC(PFNGLFLUSHPROC, glFlush)
	GET_PROC(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer)
	GET_PROC(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)
	GET_PROC(PFNGLFRONTFACEPROC, glFrontFace)
	GET_PROC(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap)
	GET_PROC(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)
	GET_PROC(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers)
	GET_PROC(PFNGLGENTEXTURESPROC, glGenTextures)
	GET_PROC(PFNGLGETBOOLEANVPROC, glGetBooleanv)
	GET_PROC(PFNGLGETERRORPROC, glGetError)
	GET_PROC(PFNGLGETFLOATVPROC, glGetFloatv)
	GET_PROC(PFNGLGETINTEGERVPROC, glGetIntegerv)
	GET_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)
	GET_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv)
	GET_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)
	GET_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv)
	GET_PROC(PFNGLGETSTRINGPROC, glGetString)
	GET_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)
	GET_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram)
	GET_PROC(PFNGLPIXELSTOREIPROC, glPixelStorei)
	GET_PROC(PFNGLPOLYGONOFFSETPROC, glPolygonOffset)
	GET_PROC(PFNGLREADPIXELSPROC, glReadPixels)
	GET_PROC(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage)
	GET_PROC(PFNGLSCISSORPROC, glScissor)
	GET_PROC(PFNGLSHADERSOURCEPROC, glShaderSource)
	GET_PROC(PFNGLTEXIMAGE2DPROC, glTexImage2D)
	GET_PROC(PFNGLTEXPARAMETERIPROC, glTexParameteri)
	GET_PROC(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D)
	GET_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)
	GET_PROC(PFNGLUSEPROGRAMPROC, glUseProgram)
	GET_PROC(PFNGLVERTEXATTRIB1FPROC, glVertexAttrib1f)
	GET_PROC(PFNGLVERTEXATTRIB4FPROC, glVertexAttrib4f)
	GET_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)
	GET_PROC(PFNGLVIEWPORTPROC, glViewport)

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

	GLES2_ctx.glDisable(GL_SCISSOR_TEST);
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
	GLES2_ctx.glClearColor(gr_screen.current_clear_color.red / 255.0f,
		gr_screen.current_clear_color.green / 255.0f,
		gr_screen.current_clear_color.blue / 255.0f, 1.0f);

	GLES2_ctx.glClear( GL_COLOR_BUFFER_BIT );
}


void gr_gles2_zbias(int bias)
{
	if (bias) {
		GLES2_ctx.glEnable(GL_POLYGON_OFFSET_FILL);
		GLES2_ctx.glPolygonOffset(0.0f, GLfloat(-bias));
	} else {
		GLES2_ctx.glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void gr_gles2_set_cull(int cull)
{
	if (cull) {
		GLES2_ctx.glEnable (GL_CULL_FACE);
		GLES2_ctx.glFrontFace (GL_CCW);
	} else {
		GLES2_ctx.glDisable (GL_CULL_FACE);
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

	if (GLES2_window && GLES2_ctx.glBindFramebuffer) {
		GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if (FB_texture) {
		GLES2_ctx.glDeleteTextures(1, &FB_texture);
		FB_texture = 0;
	}

	if (FB_rb_id) {
		GLES2_ctx.glDeleteRenderbuffers(1, &FB_rb_id);
		FB_rb_id = 0;
	}

	if (FB_id) {
		GLES2_ctx.glDeleteFramebuffers(1, &FB_id);
		FB_id = 0;
	}

	gles2_tcache_cleanup();
	gles2_shader_cleanup();

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

	Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;

	int a = 1, r = 5, g = 5, b = 5, bpp = 16;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, r);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, g);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, b);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, a);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, bpp);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	GLES2_window = SDL_CreateWindow(os_get_title(),
									gr_screen.max_w,
									gr_screen.max_h,
									window_flags);

	extern bool Gr_allow_fallback;

	if ( !GLES2_window ) {
		if (Gr_allow_fallback) {
			// This will generally happen when the GLES2 library isn't available. In
			// which case we should automatically fall back to "safe mode".
			mprintf(("  Window creation failed! \n    %s\n", SDL_GetError()));
			mprintf(("  Restarting graphics in safe mode...\n"));
			gr_init(true);	// will call _cleanup() for us
			return;
		} else {
			Error(LOCATION, "Couldn't create window: %s\n", SDL_GetError());
		}
	}

	os_set_window(GLES2_window);

	GLES2_context = SDL_GL_CreateContext(GLES2_window);

	if ( !GLES2_context ) {
		if (Gr_allow_fallback) {
			mprintf(("  GLES2 context creation failed! \n    %s\n", SDL_GetError()));
			mprintf(("  Restarting graphics in safe mode...\n"));
			gr_init(true);	// will call _cleanup() for us
			return;
		} else {
			Error(LOCATION, "Couldn't create OpenGL ES 2 context: %s\n", SDL_GetError());
		}
	}

	// first thing after context is ready, init gles2 function prototypes
	if ( !gles2_init_prototypes() ) {
		if (Gr_allow_fallback) {
			mprintf(("  Restarting graphics in safe mode...\n"));
			gr_init(true);	// will call _cleanup() for us
			return;
		} else {
			Error(LOCATION, "Failed to initialize OpenGL ES 2 functions!\n");
		}
	}

	if ( !gles2_set_variables() ) {
		if (Gr_allow_fallback) {
			mprintf(("  Hardware/Software requirements not met!\n"));
			mprintf(("  Restarting graphics in safe mode...\n"));
			gr_init(true);	// will call _cleanup() for us
			return;
		} else {
			Error(LOCATION, "Failed to initialize OpenGL ES 2!\n");
		}
	}

	mprintf(("  Vendor   : %s\n", GLES2_ctx.glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", GLES2_ctx.glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", GLES2_ctx.glGetString(GL_VERSION)));

	// initial viewport setup
	gr_gles2_set_viewport(gr_screen.max_w, gr_screen.max_h);

	gles2_init_func_pointers();
	gles2_tcache_init();

	if ( !gles2_shader_init() ) {
		if (Gr_allow_fallback) {
			mprintf(("  Shader initialization failed!\n"));
			mprintf(("  Restarting graphics in safe mode...\n"));
			gr_init(true);	// will call _cleanup() for us
			return;
		} else {
			Error(LOCATION, "Failed to initialize OpenGL ES 2 shaders!\n");
		}
	}

	if ( !gles2_create_framebuffer() ) {
		Error(LOCATION, "GLES2 framebuffer init failure!");
	}

	GLES2_ctx.glEnable(GL_DITHER);
	GLES2_ctx.glEnable(GL_DEPTH_TEST);
	GLES2_ctx.glEnable(GL_BLEND);
	GLES2_ctx.glEnable(GL_TEXTURE_2D);

	GLES2_ctx.glDepthRangef(0.0f, 1.0f);

	GLES2_ctx.glPixelStorei(GL_PACK_ALIGNMENT, 1);
	GLES2_ctx.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLES2_ctx.glFlush();

	gr_gles2_clear();
	gr_gles2_set_cull(1);

	mprintf(("  Attributes requested : ARGB %d%d%d%d, BPP %d\n",
			 a, r, g, b, bpp));

	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &bpp);

	mprintf(("  Attributes received  : ARGB %d%d%d%d, BPP %d\n",
			 a, r, g, b, bpp));

	if (GLES2_res_scale < 2) {
		mprintf(("  Resolution scaling   : disabled\n"));
	} else {
		mprintf(("  Resolution scaling   : %dx\n", GLES2_res_scale));
	}

	mprintf(("\n"));

	SDL_StopTextInput(os_get_window());
	SDL_DisableScreenSaver();

	// maybe go fullscreen - should be done *after* main GL init
	int fullscreen = os_config_read_uint("Video", "Fullscreen", 1);
	if ( !Cmdline_window && (fullscreen || Cmdline_fullscreen) ) {
		gr_force_fullscreen();
	}

	if (Cmdline_no_vsync) {
		SDL_GL_SetSwapInterval(0);
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


	gr_reset_clip();
	gr_clear();
	gr_flip();
	gr_clear();
}

void gr_gles2_flip()
{
	if ( !GLES2_inited ) {
		return;
	}

	GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gr_gles2_reset_clip();

	// set viewport to window size
	GLES2_ctx.glViewport(GLES2_viewport_x, GLES2_viewport_y, GLES2_viewport_w, GLES2_viewport_h);

	GLES2_ctx.glClear(GL_COLOR_BUFFER_BIT);

	{
		float x = 0.0f;
		float y = 0.0f;
		float w = i2fl(GLES2_viewport_w);
		float h = i2fl(GLES2_viewport_h);

		const float tex_coord[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };
		const float ver_coord[] = { x, y, x, h, w, y, w, h };

		gles2_shader_use(PROG_WINDOW);

		GLES2_ctx.glEnableVertexAttribArray(SDRI_POSITION);
		GLES2_ctx.glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

		GLES2_ctx.glEnableVertexAttribArray(SDRI_TEXCOORD);
		GLES2_ctx.glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

		GLES2_ctx.glBindTexture(GL_TEXTURE_2D, FB_texture);

		GLES2_ctx.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);

		GLES2_ctx.glDisableVertexAttribArray(SDRI_TEXCOORD);
		GLES2_ctx.glDisableVertexAttribArray(SDRI_POSITION);
	}

	mouse_eval_deltas();

#ifndef NDEBUG
	GLenum error = GLES2_ctx.glGetError();

	if (error != GL_NO_ERROR) {
		mprintf(("!!DEBUG!! OpenGL Error: %d\n", error));
	}
#endif

	SDL_GL_SwapWindow(GLES2_window);

	gles2_tcache_frame();

	GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// set viewport to game screen size
	GLES2_ctx.glViewport(0, 0, gles2_res_scale(gr_screen.max_w),
						 gles2_res_scale(gr_screen.max_h));
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

	GLES2_ctx.glEnable(GL_SCISSOR_TEST);
	GLES2_ctx.glScissor(gles2_res_scale(x), gles2_res_scale(gr_screen.max_h-y-h),
						gles2_res_scale(w), gles2_res_scale(h));
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
		GLES2_ctx.glClear(GL_DEPTH_BUFFER_BIT);
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

	const int width = gles2_res_scale(gr_screen.max_w);
	const int height = gles2_res_scale(gr_screen.max_h);

	const int b_size = width * height;

	buf = (ubyte*)malloc(b_size * 4);

	if (buf == NULL) {
		return;
	}

	CFILE *f = cfopen(tmp, "wb", CF_TYPE_ROOT);

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
	cfwrite_ushort( (ushort)width, f );	//	Width;
	cfwrite_ushort( (ushort)height, f );	//	Height;
	cfwrite_ubyte( 24, f );	//PixelDepth;
	cfwrite_ubyte( 0, f );	//ImageDesc;

	memset(buf, 0, b_size * 4);

	GLES2_ctx.glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);

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

static bool viewport_scissor_test_reset = false;

void gr_gles2_push_unscaled_viewport()
{
	GLboolean val = GL_FALSE;

	if ( !gles2_need_res_scale() ) {
		return;
	}

	GLES2_ctx.glViewport(0, 0, gr_screen.max_w, gr_screen.max_h);

	// Disabling the scissor test is easier than changing and then resetting it
	// for the viewport change. But with the current single use case in the
	// fullneb code that shouldn't be a problem.
	GLES2_ctx.glGetBooleanv(GL_SCISSOR_TEST, &val);

	if (val) {
		GLES2_ctx.glDisable(GL_SCISSOR_TEST);
	}

	viewport_scissor_test_reset = (val == GL_TRUE);
}

void gr_gles2_pop_unscaled_viewport()
{
	if ( !gles2_need_res_scale() ) {
		return;
	}

	GLES2_ctx.glViewport(0, 0, gles2_res_scale(gr_screen.max_w),
						 gles2_res_scale(gr_screen.max_h));

	if (viewport_scissor_test_reset) {
		GLES2_ctx.glEnable(GL_SCISSOR_TEST);
	}
}

// NOTE: This should only be called between push/pop unscaled_viewport() calls
//       due to the rest of the code not really knowing about true viewport size
void gr_gles2_get_region(int, int w, int h, ubyte *data)
{
	gles2_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	GLES2_ctx.glReadPixels(0, gr_screen.max_h-h-1, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

int gr_gles2_save_screen()
{
	gr_gles2_reset_clip();

	if (GL_saved_screen_tex) {
		mprintf(( "Screen already saved!\n" ));
		return -1;
	}

	GLES2_ctx.glGenTextures(1, &GL_saved_screen_tex);

	if ( !GL_saved_screen_tex ) {
		mprintf(( "Couldn't create texture for saved screen!\n" ));
		return -1;
	}

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, GL_saved_screen_tex);

	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLES2_ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0,
							   gles2_res_scale(gr_screen.max_w),
							   gles2_res_scale(gr_screen.max_h), 0);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);

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

	GLES2_ctx.glVertexAttrib4f(SDRI_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

	GLES2_ctx.glEnableVertexAttribArray(SDRI_POSITION);
	GLES2_ctx.glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

	GLES2_ctx.glEnableVertexAttribArray(SDRI_TEXCOORD);
	GLES2_ctx.glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, GL_saved_screen_tex);

	GLES2_ctx.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	GLES2_ctx.glDisableVertexAttribArray(SDRI_TEXCOORD);
	GLES2_ctx.glDisableVertexAttribArray(SDRI_POSITION);
}

void gr_gles2_free_screen(int)
{
	if (GL_saved_screen_tex) {
		GLES2_ctx.glDeleteTextures(1, &GL_saved_screen_tex);
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

static renderbuffer_t GL_stream[4];
static int GL_stream_w = 0;
static int GL_stream_h = 0;

static void gles2_stream_set_viewport()
{
	int window_w, window_h;

	SDL_GetWindowSizeInPixels(os_get_window(), &window_w, &window_h);

	float ratio = GL_stream_w / i2fl(GL_stream_h);

	int w = window_w;
	int h = fl2i((window_w / ratio) + 0.5f);

	if (h > window_h) {
		h = window_h;
		w = fl2i((window_h * ratio) + 0.5f);
	}

	GLES2_ctx.glViewport((window_w - w) / 2,
						 (window_h - h) / 2,
						 w, h);

	// set quad to entire window and let viewport fix aspect ratio
	GL_stream[0].x = 0.0f;
	GL_stream[0].y = 0.0f;
	GL_stream[0].u = 0.0f;
	GL_stream[0].v = 0.0f;

	GL_stream[1].x = 0.0f;
	GL_stream[1].y = i2fl(window_h);
	GL_stream[1].u = 0.0f;
	GL_stream[1].v = 1.0f;

	GL_stream[2].x = i2fl(window_w);
	GL_stream[2].y = 0.0f;
	GL_stream[2].u = 1.0f;
	GL_stream[2].v = 0.0f;

	GL_stream[3].x = i2fl(window_w);
	GL_stream[3].y = i2fl(window_h);
	GL_stream[3].u = 1.0f;
	GL_stream[3].v = 1.0f;

	gles2_shader_update(window_w, window_h);
}

void gr_gles2_stream_start(int x, int y, int w, int h)
{
	if (GL_stream_tex) {
		return;
	}

	// render directly so we can make use of entire window size more easily
	GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gr_gles2_reset_clip();

	GLES2_ctx.glClear(GL_COLOR_BUFFER_BIT);

	GLES2_ctx.glGenTextures(1, &GL_stream_tex);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLES2_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLES2_ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
						   GL_UNSIGNED_SHORT_5_6_5, nullptr);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	GL_stream_w = w;
	GL_stream_h = h;

	gles2_stream_set_viewport();

	GLES2_ctx.glDisable(GL_DEPTH_TEST);
}

void gr_gles2_stream_frame(const SDL_Surface *frame)
{
	if ( !GL_stream_tex ) {
		return;
	}

	GLES2_ctx.glClear(GL_COLOR_BUFFER_BIT);

	gles2_shader_use(PROG_WINDOW);

	GLES2_ctx.glEnableVertexAttribArray(SDRI_POSITION);
	GLES2_ctx.glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(renderbuffer_t), &GL_stream[0].x);

	GLES2_ctx.glEnableVertexAttribArray(SDRI_TEXCOORD);
	GLES2_ctx.glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(renderbuffer_t), &GL_stream[0].u);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	GLES2_ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->w, frame->h, GL_RGB,
							  GL_UNSIGNED_SHORT_5_6_5, frame->pixels);

	GLES2_ctx.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	GLES2_ctx.glDisableVertexAttribArray(SDRI_TEXCOORD);
	GLES2_ctx.glDisableVertexAttribArray(SDRI_POSITION);

	SDL_GL_SwapWindow(GLES2_window);
}

void gr_gles2_stream_stop()
{
	if (GL_stream_tex) {
		GLES2_ctx.glBindTexture(GL_TEXTURE_2D, 0);
		GLES2_ctx.glDeleteTextures(1, &GL_stream_tex);
		GL_stream_tex = 0;

		GLES2_ctx.glEnable(GL_DEPTH_TEST);
	}

	GLES2_ctx.glBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// reset viewport to game screen size
	GLES2_ctx.glViewport(0, 0, gr_screen.max_w, gr_screen.max_h);

	gles2_shader_update();
}

void gr_gles2_set_viewport(int /*width*/, int /*height*/)
{
	int width, height;
	int w, h, x, y;

	SDL_GetWindowSizeInPixels(GLES2_window, &width, &height);

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

	// if playing movie then adjust viewport for that
	if (GL_stream_tex) {
		gles2_stream_set_viewport();
	}
}
