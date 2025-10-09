/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef __EMSCRIPTEN__

#define SDL_OPENGL_1_NO_PROTOTYPES
#define SDL_OPENGL_1_FUNCTION_TYPEDEFS

#include <SDL3/SDL_opengl.h>

#include "pstypes.h"
#include "osregistry.h"
#include "gropengl.h"
#include "gropenglinternal.h"
#include "grinternal.h"
#include "cmdline.h"
#include "mouse.h"
#include "osapi.h"
#include "bmpman.h"
#include "cfile.h"
#include "renderbuffer.h"


bool OGL_inited = false;

SDL_Window *GL_window = NULL;
SDL_GLContext GL_context;

GL_func_context GL_ctx;

static int GL_activate = 0;
static int GL_deactivate = 0;

int GL_version = 0;

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

static GLuint GL_stream_tex = 0;
static GLuint Gr_saved_screen_tex = 0;

static gr_alpha_blend GL_current_alpha_blend = (gr_alpha_blend) -1;
static gr_zbuffer_type GL_current_zbuffer_type = (gr_zbuffer_type) -1;

static void opengl_set_viewport();


void opengl_set_variables()
{
	GL_min_texture_height = 16;
	GL_min_texture_width = 16;

	GL_ctx.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GL_max_texture_width);
	GL_max_texture_height = GL_max_texture_width;

	// no texture is larger than 1024, so maybe don't use sections
	if (GL_max_texture_width >= 1024) {
		gr_screen.use_sections = 0;
	}

	// see if we can use nondark pixels
	gr_screen.use_nondark = 0;

	if (GL_version >= 13) {
		GLint max_tex_units = 0;
		GL_ctx.glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_tex_units);

		gr_screen.use_nondark = (max_tex_units > 2) ? 1 : 0;
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

	GL_ctx.glViewport(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h);
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



void opengl_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt)
{
	opengl_set_texture_state(ts);

	if (ab != GL_current_alpha_blend) {
		switch (ab) {
			case ALPHA_BLEND_NONE:			// 1*SrcPixel + 0*DestPixel
				GL_ctx.glBlendFunc(GL_ONE, GL_ZERO);
				break;
			case ALPHA_BLEND_ADDITIVE:		// 1*SrcPixel + 1*DestPixel
				GL_ctx.glBlendFunc(GL_ONE, GL_ONE);
				break;
			case ALPHA_BLEND_ALPHA_ADDITIVE:	// Alpha*SrcPixel + 1*DestPixel
				GL_ctx.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case ALPHA_BLEND_ALPHA_BLEND_ALPHA:	// Alpha*SrcPixel + (1-Alpha)*DestPixel
				GL_ctx.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case ALPHA_BLEND_ALPHA_BLEND_SRC_COLOR:	// Alpha*SrcPixel + (1-SrcPixel)*DestPixel
				GL_ctx.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
				break;
			default:
				break;
		}

		GL_current_alpha_blend = ab;
	}

	if (zt != GL_current_zbuffer_type) {
		switch (zt) {
			case ZBUFFER_TYPE_NONE:
				GL_ctx.glDepthFunc(GL_ALWAYS);
				GL_ctx.glDepthMask(GL_FALSE);
				break;
			case ZBUFFER_TYPE_READ:
				GL_ctx.glDepthFunc(GL_LESS);
				GL_ctx.glDepthMask(GL_FALSE);
				break;
			case ZBUFFER_TYPE_WRITE:
				GL_ctx.glDepthFunc(GL_ALWAYS);
				GL_ctx.glDepthMask(GL_TRUE);
				break;
			case ZBUFFER_TYPE_FULL:
				GL_ctx.glDepthFunc(GL_LESS);
				GL_ctx.glDepthMask(GL_TRUE);
				break;
			default:
				break;
		}

		GL_current_zbuffer_type = zt;
	}
}

static void opengl_init_func_pointers()
{
	gr_screen.gf_flip = gr_opengl_flip;
	gr_screen.gf_set_clip = gr_opengl_set_clip;
	gr_screen.gf_reset_clip = gr_opengl_reset_clip;

	gr_screen.gf_clear = gr_opengl_clear;

	gr_screen.gf_aabitmap = gr_opengl_aabitmap;
	gr_screen.gf_aabitmap_ex = gr_opengl_aabitmap_ex;

	gr_screen.gf_rect = gr_opengl_rect;
	gr_screen.gf_shade = gr_opengl_shade;
	gr_screen.gf_string = gr_opengl_string;
	gr_screen.gf_circle = gr_opengl_circle;

	gr_screen.gf_line = gr_opengl_line;
	gr_screen.gf_aaline = gr_opengl_aaline;
	gr_screen.gf_aalines = gr_opengl_aalines;
	gr_screen.gf_pixel = gr_opengl_pixel;
	gr_screen.gf_scaler = gr_opengl_scaler;
	gr_screen.gf_aascaler = gr_opengl_aascaler;
	gr_screen.gf_tmapper = gr_opengl_tmapper;

	gr_screen.gf_gradient = gr_opengl_gradient;

	gr_screen.gf_print_screen = gr_opengl_print_screen;

	gr_screen.gf_fade_in = gr_opengl_fade_in;
	gr_screen.gf_fade_out = gr_opengl_fade_out;
	gr_screen.gf_flash = gr_opengl_flash;

	gr_screen.gf_zbuffer_clear = gr_opengl_zbuffer_clear;

	gr_screen.gf_save_screen = gr_opengl_save_screen;
	gr_screen.gf_restore_screen = gr_opengl_restore_screen;
	gr_screen.gf_free_screen = gr_opengl_free_screen;

	gr_screen.gf_dump_frame_start = gr_opengl_dump_frame_start;
	gr_screen.gf_dump_frame_stop = gr_opengl_dump_frame_stop;
	gr_screen.gf_dump_frame = gr_opengl_dump_frame;

	gr_screen.gf_stream_start = gr_opengl_stream_start;
	gr_screen.gf_stream_frame = gr_opengl_stream_frame;
	gr_screen.gf_stream_stop = gr_opengl_stream_stop;

	gr_screen.gf_set_gamma = gr_opengl_set_gamma;

	gr_screen.gf_lock = gr_opengl_lock;
	gr_screen.gf_unlock = gr_opengl_unlock;

	gr_screen.gf_fog_set = gr_opengl_fog_set;

	gr_screen.gf_get_region = gr_opengl_get_region;

	gr_screen.gf_set_cull = gr_opengl_set_cull;

	gr_screen.gf_cross_fade = gr_opengl_cross_fade;

	gr_screen.gf_preload_init = gr_opengl_preload_init;
	gr_screen.gf_preload = gr_opengl_preload;

	gr_screen.gf_zbias = gr_opengl_zbias;

	gr_screen.gf_set_viewport = gr_opengl_set_viewport;

	gr_screen.gf_activate = gr_opengl_activate;

	gr_screen.gf_release_texture = gr_opengl_release_texture;
}

bool opengl_init_prototypes()
{
	#define GET_PROC(type, func)	\
		do {	\
			GL_ctx.func = reinterpret_cast<type>(SDL_GL_GetProcAddress(#func));	\
			if ( !(GL_ctx.func) ) {	\
				mprintf(("  Couldn't load OpenGL function %s: %s", #func, SDL_GetError()));	\
			return false;	\
		}	\
	} while(false);

	if ( (GL_version >= 13) || SDL_GL_ExtensionSupported("GL_ARB_multitexture") ) {
		GET_PROC(PFNGLACTIVETEXTUREPROC, glActiveTexture)
		GET_PROC(PFNGLCLIENTACTIVETEXTUREARBPROC, glClientActiveTexture)
	}

	GET_PROC(PFNGLBINDTEXTUREPROC, glBindTexture);
	GET_PROC(PFNGLBLENDFUNCPROC, glBlendFunc);
	GET_PROC(PFNGLCLEARCOLORPROC, glClearColor);
	GET_PROC(PFNGLCLEARPROC, glClear);
	GET_PROC(PFNGLCOLOR4UBPROC, glColor4ub);
	GET_PROC(PFNGLCOLORPOINTERPROC, glColorPointer);
	GET_PROC(PFNGLCOPYTEXIMAGE2DPROC, glCopyTexImage2D);
	GET_PROC(PFNGLDELETETEXTURESPROC, glDeleteTextures);
	GET_PROC(PFNGLDEPTHFUNCPROC, glDepthFunc);
	GET_PROC(PFNGLDEPTHMASKPROC, glDepthMask);
	GET_PROC(PFNGLDEPTHRANGEPROC, glDepthRange);
	GET_PROC(PFNGLDISABLECLIENTSTATEPROC, glDisableClientState);
	GET_PROC(PFNGLDISABLEPROC, glDisable);
	GET_PROC(PFNGLDRAWARRAYSPROC, glDrawArrays);
	GET_PROC(PFNGLENABLECLIENTSTATEPROC, glEnableClientState);
	GET_PROC(PFNGLENABLEPROC, glEnable);
	GET_PROC(PFNGLFLUSHPROC, glFlush);
	GET_PROC(PFNGLFOGFPROC, glFogf);
	GET_PROC(PFNGLFOGFVPROC, glFogfv);
	GET_PROC(PFNGLFOGIPROC, glFogi);
	GET_PROC(PFNGLFRONTFACEPROC, glFrontFace);
	GET_PROC(PFNGLGENTEXTURESPROC, glGenTextures);
	GET_PROC(PFNGLGETERRORPROC, glGetError);
	GET_PROC(PFNGLGETINTEGERVPROC, glGetIntegerv);
	GET_PROC(PFNGLGETSTRINGPROC, glGetString);
	GET_PROC(PFNGLHINTPROC, glHint);
	GET_PROC(PFNGLLOADIDENTITYPROC, glLoadIdentity);
	GET_PROC(PFNGLMATRIXMODEPROC, glMatrixMode);
	GET_PROC(PFNGLORTHOPROC, glOrtho);
	GET_PROC(PFNGLPIXELSTOREIPROC, glPixelStorei);
	GET_PROC(PFNGLPOLYGONOFFSETPROC, glPolygonOffset);
	GET_PROC(PFNGLPOPATTRIBPROC, glPopAttrib);
	GET_PROC(PFNGLPOPCLIENTATTRIBPROC, glPopClientAttrib);
	GET_PROC(PFNGLPUSHATTRIBPROC, glPushAttrib);
	GET_PROC(PFNGLPUSHCLIENTATTRIBPROC, glPushClientAttrib);
	GET_PROC(PFNGLREADBUFFERPROC, glReadBuffer);
	GET_PROC(PFNGLREADPIXELSPROC, glReadPixels);
	GET_PROC(PFNGLSCALEFPROC, glScalef);
	GET_PROC(PFNGLSCISSORPROC, glScissor);
	GET_PROC(PFNGLSHADEMODELPROC, glShadeModel);
	GET_PROC(PFNGLTEXCOORDPOINTERPROC, glTexCoordPointer);
	GET_PROC(PFNGLTEXENVFPROC, glTexEnvf);
	GET_PROC(PFNGLTEXENVFVPROC, glTexEnvfv);
	GET_PROC(PFNGLTEXENVIPROC, glTexEnvi);
	GET_PROC(PFNGLTEXIMAGE2DPROC, glTexImage2D);
	GET_PROC(PFNGLTEXPARAMETERIPROC, glTexParameteri);
	GET_PROC(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D);
	GET_PROC(PFNGLVERTEXPOINTERPROC, glVertexPointer);
	GET_PROC(PFNGLVIEWPORTPROC, glViewport);

	return true;
}

void gr_opengl_flip()
{
	if ( !OGL_inited ) {
		return;
	}

	gr_opengl_reset_clip();

	mouse_eval_deltas();

#ifndef NDEBUG
	GLenum error = GL_ctx.glGetError();

	if (error != GL_NO_ERROR) {
		mprintf(("!!DEBUG!! OpenGL Error: %d\n", error));
	}
#endif

	SDL_GL_SwapWindow(GL_window);

	GL_ctx.glClear(GL_COLOR_BUFFER_BIT);

	opengl_tcache_frame();

	int cnt = GL_activate;

	if (cnt) {
		GL_activate -= cnt;
		opengl_tcache_flush();
	}

	cnt = GL_deactivate;

	if (cnt) {
		GL_deactivate -= cnt;
	}
}

void gr_opengl_set_clip(int x, int y, int w, int h)
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

	x = fl2i((x * GL_viewport_scale_w) + 0.5f) + GL_viewport_x;
	y = fl2i((y * GL_viewport_scale_h) + 0.5f) + GL_viewport_y;
	w = fl2i((w * GL_viewport_scale_w) + 0.5f);
	h = fl2i((h * GL_viewport_scale_h) + 0.5f);

	GL_ctx.glEnable(GL_SCISSOR_TEST);
	GL_ctx.glScissor(x, GL_viewport_h-y-h, w, h);
}

void gr_opengl_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far)
{
	SDL_assert((r >= 0) && (r < 256));
	SDL_assert((g >= 0) && (g < 256));
	SDL_assert((b >= 0) && (b < 256));

	if (fog_mode == GR_FOGMODE_NONE) {
		if (gr_screen.current_fog_mode != fog_mode) {
			GL_ctx.glDisable(GL_FOG);
		}

		gr_screen.current_fog_mode = fog_mode;

		return;
	}

	if (gr_screen.current_fog_mode != fog_mode) {
		GL_ctx.glEnable(GL_FOG);
		GL_ctx.glFogi(GL_FOG_MODE, GL_LINEAR);

		gr_screen.current_fog_mode = fog_mode;
	}

	if ( (gr_screen.current_fog_color.red != r) ||
			(gr_screen.current_fog_color.green != g) ||
			(gr_screen.current_fog_color.blue != b) ) {
		gr_init_color( &gr_screen.current_fog_color, r, g, b );

		GLfloat fc[4];

		fc[0] = r / 255.0f;
		fc[1] = g / 255.0f;
		fc[2] = b / 255.0f;
		fc[3] = 1.0f;

		GL_ctx.glFogfv(GL_FOG_COLOR, fc);
	}

	if( (fog_near >= 0.0f) && (fog_far >= 0.0f) &&
			((fog_near != gr_screen.fog_near) ||
			(fog_far != gr_screen.fog_far)) ) {
		gr_screen.fog_near = fog_near;
		gr_screen.fog_far = fog_far;

		GL_ctx.glFogf(GL_FOG_START, fog_near);
		GL_ctx.glFogf(GL_FOG_END, fog_far);
	}
}

void gr_opengl_zbuffer_clear(int mode)
{
	if (mode) {
		Gr_zbuffering = 1;
		Gr_zbuffering_mode = GR_ZBUFF_FULL;
		Gr_global_zbuffering = 1;

		opengl_set_state( TEXTURE_SOURCE_NONE, ALPHA_BLEND_NONE, ZBUFFER_TYPE_FULL );
		GL_ctx.glClear ( GL_DEPTH_BUFFER_BIT );
	} else {
		Gr_zbuffering = 0;
		Gr_zbuffering_mode = GR_ZBUFF_NONE;
		Gr_global_zbuffering = 0;
	}
}

void gr_opengl_print_screen(const char *filename)
{
	char tmp[MAX_FILENAME_LEN];
	ubyte *buf = NULL;

	SDL_strlcpy( tmp, filename, SDL_arraysize(tmp) );
	SDL_strlcat( tmp, NOX(".tga"), SDL_arraysize(tmp) );

	buf = (ubyte*)malloc(GL_viewport_w * GL_viewport_h * 3);

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
	cfwrite_ushort( (ushort)GL_viewport_w, f );	//	Width;
	cfwrite_ushort( (ushort)GL_viewport_h, f );	//	Height;
	cfwrite_ubyte( 24, f );	//PixelDepth;
	cfwrite_ubyte( 0, f );	//ImageDesc;

	memset(buf, 0, GL_viewport_w * GL_viewport_h * 3);

	GL_ctx.glReadBuffer(GL_FRONT);

	GL_ctx.glReadPixels(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h, GL_BGR, GL_UNSIGNED_BYTE, buf);

	cfwrite(buf, GL_viewport_w * GL_viewport_h * 3, 1, f);

	cfclose(f);

	free(buf);
}

void gr_opengl_fade_in(int instantaneous)
{
	// Empty - DDOI
}

void gr_opengl_fade_out(int instantaneous)
{
	// Empty - DDOI
}

void gr_opengl_get_region(int front, int w, int h, ubyte *data)
{
	opengl_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	GLenum pxtype = GL_UNSIGNED_SHORT_5_5_5_1;

	if (gr_screen.bytes_per_pixel == 4) {
		pxtype = GL_UNSIGNED_BYTE;
	}

	GL_ctx.glReadBuffer( (front) ? GL_FRONT : GL_BACK );

	GL_ctx.glReadPixels(GL_viewport_x, (GL_viewport_y+GL_viewport_h)-h-1, w, h, GL_RGBA, pxtype, data);
}

int gr_opengl_save_screen()
{
	gr_opengl_reset_clip();

	if (Gr_saved_screen_tex) {
		mprintf(( "Screen already saved!\n" ));
		return -1;
	}

	GL_ctx.glGenTextures(1, &Gr_saved_screen_tex);

	if ( !Gr_saved_screen_tex ) {
		mprintf(( "Couldn't create texture for saved screen!\n" ));
		return -1;
	}

	GL_ctx.glBindTexture(GL_TEXTURE_2D, Gr_saved_screen_tex);

	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GL_ctx.glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	GL_ctx.glReadBuffer(GL_FRONT);

	GL_ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GL_viewport_x, GL_viewport_y,
			GL_viewport_w, GL_viewport_h, 0);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

void gr_opengl_restore_screen(int)
{
	gr_opengl_reset_clip();

	if ( !Gr_saved_screen_tex ) {
		gr_opengl_clear();
		return;
	}

	int x = 0;
	int y = 0;
	int w = fl2i(GL_viewport_w / GL_viewport_scale_w + 0.5f);
	int h = fl2i(GL_viewport_h / GL_viewport_scale_h + 0.5f);

	const int tex_coord[] = { 0, 1, 0, 0, 1, 1, 1, 0 };	// y-flipped
	const int ver_coord[] = { x, y, x, h, w, y, w, h };

	GL_ctx.glColor4ub(255, 255, 255, 255);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, Gr_saved_screen_tex);

	opengl_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	GL_ctx.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_ctx.glEnableClientState(GL_VERTEX_ARRAY);

	GL_ctx.glTexCoordPointer(2, GL_INT, 0, &tex_coord);
	GL_ctx.glVertexPointer(2, GL_INT, 0, &ver_coord);

	GL_ctx.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	GL_ctx.glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_ctx.glDisableClientState(GL_VERTEX_ARRAY);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, 0);
}

void gr_opengl_free_screen(int)
{
	if (Gr_saved_screen_tex) {
		GL_ctx.glDeleteTextures(1, &Gr_saved_screen_tex);
		Gr_saved_screen_tex = 0;
	}
}

void gr_opengl_dump_frame_start(int first_frame, int frames_between_dumps)
{
	STUB_FUNCTION;
}

void gr_opengl_dump_frame_stop()
{
	STUB_FUNCTION;
}

void gr_opengl_dump_frame()
{
	STUB_FUNCTION;
}

static renderbuffer_t GL_stream[4];
static int GL_stream_w = 0;
static int GL_stream_h = 0;

static void opengl_stream_set_viewport()
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

	float scale_by = w / i2fl(GL_stream_w);
	
	GL_ctx.glViewport((window_w - w) / 2,
					  (window_h - h) / -2,
					  window_w,
					  window_h);

	GL_ctx.glMatrixMode(GL_PROJECTION);
	GL_ctx.glLoadIdentity();
	GL_ctx.glOrtho(0, window_w, window_h, 0, 0.0, 1.0);
	GL_ctx.glMatrixMode(GL_MODELVIEW);
	GL_ctx.glLoadIdentity();
	GL_ctx.glScalef(scale_by, scale_by, 1.0f);
}

void gr_opengl_stream_start(int x, int y, int w, int h)
{
	if (GL_stream_tex) {
		return;
	}

	if (gr_screen.use_sections) {
		mprintf(("GR_STREAM: Bitmap sections not supported\n"));
		return;
	}

	int tex_w = next_pow2(w);
	int tex_h = next_pow2(h);

	GL_ctx.glGenTextures(1, &GL_stream_tex);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GL_ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GL_ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB,
						GL_UNSIGNED_SHORT_5_6_5, nullptr);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	GL_stream_w = w;
	GL_stream_h = h;

	opengl_stream_set_viewport();

	int sx = 0, sy = 0;

	if (x > 0) {
		sx = x;
	}

	if (y > 0) {
		sy = y;
	}

	GL_stream[0].x = i2fl(sx);
	GL_stream[0].y = i2fl(sy);
	GL_stream[0].u = 0.0f;
	GL_stream[0].v = 0.0f;

	GL_stream[1].x = i2fl(sx);
	GL_stream[1].y = i2fl(sy + h);
	GL_stream[1].u = 0.0f;
	GL_stream[1].v = i2fl(h) / i2fl(tex_h);

	GL_stream[2].x = i2fl(sx + w);
	GL_stream[2].y = i2fl(sy);
	GL_stream[2].u = i2fl(w) / i2fl(tex_w);
	GL_stream[2].v = 0.0f;

	GL_stream[3].x = i2fl(sx + w);
	GL_stream[3].y = i2fl(sy + h);
	GL_stream[3].u = i2fl(w) / i2fl(tex_w);
	GL_stream[3].v = i2fl(h) / i2fl(tex_h);

	GL_ctx.glDisable(GL_DEPTH_TEST);

	gr_set_clear_color(0, 0, 0);
	GL_ctx.glColor4ub(255, 255, 255, 255);
}

void gr_opengl_stream_frame(const SDL_Surface *frame)
{
	if ( !GL_stream_tex ) {
		return;
	}

	gr_opengl_clear();

	GL_ctx.glEnableClientState(GL_VERTEX_ARRAY);
	GL_ctx.glVertexPointer(2, GL_FLOAT, sizeof(renderbuffer_t), &GL_stream[0].x);

	GL_ctx.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_ctx.glTexCoordPointer(2, GL_FLOAT, sizeof(renderbuffer_t), &GL_stream[0].u);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	GL_ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->w, frame->h, GL_RGB,
						   GL_UNSIGNED_SHORT_5_6_5, frame->pixels);

	GL_ctx.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	GL_ctx.glBindTexture(GL_TEXTURE_2D, 0);

	GL_ctx.glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_ctx.glDisableClientState(GL_VERTEX_ARRAY);

	SDL_GL_SwapWindow(GL_window);
}

void gr_opengl_stream_stop()
{
	if (GL_stream_tex) {
		GL_ctx.glBindTexture(GL_TEXTURE_2D, 0);
		GL_ctx.glDeleteTextures(1, &GL_stream_tex);
		GL_stream_tex = 0;

		GL_ctx.glEnable(GL_DEPTH_TEST);
	}

	// switch back to standard viewport
	opengl_set_viewport();
}

void gr_opengl_set_viewport(int /*width*/, int /*height*/)
{
	int width, height;
	int w, h, x, y;

	SDL_GetWindowSizeInPixels(GL_window, &width, &height);

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

	gr_screen.viewport_offset_x = x;
	gr_screen.viewport_offset_y = y;

	gr_screen.viewport_scale_factor_x = 1.0f / GL_viewport_scale_w;
	gr_screen.viewport_scale_factor_y = 1.0f / GL_viewport_scale_h;

	// if playing movie then just adjust viewport for that
	if (GL_stream_tex) {
		opengl_stream_set_viewport();
	} else {
		opengl_set_viewport();
	}
}

static void opengl_set_viewport()
{
	GL_ctx.glViewport(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h);

	GL_ctx.glMatrixMode(GL_PROJECTION);
	GL_ctx.glLoadIdentity();
	GL_ctx.glOrtho(0, GL_viewport_w, GL_viewport_h, 0, 0.0, 1.0);
	GL_ctx.glMatrixMode(GL_MODELVIEW);
	GL_ctx.glLoadIdentity();
	GL_ctx.glScalef(GL_viewport_scale_w, GL_viewport_scale_h, 1.0f);
}

void gr_opengl_clear()
{
	GL_ctx.glClearColor(gr_screen.current_clear_color.red / 255.0f,
		gr_screen.current_clear_color.green / 255.0f,
		gr_screen.current_clear_color.blue / 255.0f, 1.0f);

	GL_ctx.glClear( GL_COLOR_BUFFER_BIT );
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

	GL_ctx.glDisable(GL_SCISSOR_TEST);
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
		GL_ctx.glEnable(GL_POLYGON_OFFSET_FILL);
		GL_ctx.glPolygonOffset(0.0f, GLfloat(-bias));
	} else {
		GL_ctx.glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void gr_opengl_set_cull(int cull)
{
	if (cull) {
		GL_ctx.glEnable (GL_CULL_FACE);
		GL_ctx.glFrontFace (GL_CCW);
	} else {
		GL_ctx.glDisable (GL_CULL_FACE);
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

#endif	// !__EMSCRIPTEN__

void gr_opengl_cleanup()
{
#ifndef __EMSCRIPTEN__
	if ( !OGL_inited ) {
		return;
	}

	gr_opengl_reset_clip();
	gr_opengl_clear();
	gr_opengl_flip();

	gr_opengl_free_screen(0);

	opengl_tcache_cleanup();

	if (GL_context) {
		SDL_GL_DestroyContext(GL_context);
		GL_context = nullptr;
	}

	if (GL_window) {
		os_set_window(nullptr);
		SDL_DestroyWindow(GL_window);
		GL_window = nullptr;
	}

	OGL_inited = false;
#endif	// !__EMSCRIPTEN__
}

void gr_opengl_init()
{
#ifndef __EMSCRIPTEN__
	if (OGL_inited) {
		gr_opengl_cleanup();
	}

	mprintf(( "Initializing OpenGL graphics device...\n" ));

	OGL_inited = true;

	if ( !SDL_InitSubSystem(SDL_INIT_VIDEO) ) {
		Error(LOCATION, "Couldn't init SDL: %s", SDL_GetError());
	}

	Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;

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

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	GL_window = SDL_CreateWindow(os_get_title(),
								 gr_screen.max_w,
								 gr_screen.max_h,
								 window_flags);

	if ( !GL_window ) {
		Error(LOCATION, "Couldn't create window: %s\n", SDL_GetError());
	}

	os_set_window(GL_window);

	GL_context = SDL_GL_CreateContext(GL_window);

	if ( !GL_context ) {
		Error(LOCATION, "Couldn't create OpenGL context: %s\n", SDL_GetError());
	}

	// first thing after context is ready, init gl function prototypes
	if ( !opengl_init_prototypes() ) {
		Error(LOCATION, "Failed to initialize OpenGL functions!\n");
	}

	auto gl_version = reinterpret_cast<const char *>(GL_ctx.glGetString(GL_VERSION));
	int v_major = 0, v_minor = 0;

	SDL_sscanf(gl_version, "%d.%d", &v_major, &v_minor);

	GL_version = (v_major * 10) + v_minor;

	mprintf(("  Vendor   : %s\n", GL_ctx.glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", GL_ctx.glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", gl_version));

	// we need OpenGL packed pixel formats, so check for core support or EXT
	if ( (GL_version < 12) && !SDL_GL_ExtensionSupported("GL_EXT_packed_pixels") ) {
		Error(LOCATION, "Minimum required OpenGL version is 1.2!");
	}

	// set up generic variables
	opengl_set_variables();

	opengl_init_func_pointers();
	opengl_tcache_init();

	// initial viewport setup
	gr_opengl_set_viewport(gr_screen.max_w, gr_screen.max_h);

	GL_ctx.glShadeModel(GL_SMOOTH);
	GL_ctx.glEnable(GL_DITHER);
	GL_ctx.glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	GL_ctx.glHint(GL_FOG_HINT, GL_NICEST);

	GL_ctx.glEnable(GL_DEPTH_TEST);
	GL_ctx.glEnable(GL_BLEND);

	GL_ctx.glEnable(GL_TEXTURE_2D);

	GL_ctx.glDepthRange(0.0, 1.0);

	GL_ctx.glPixelStorei(GL_PACK_ALIGNMENT, 1);
	GL_ctx.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GL_ctx.glFlush();

	gr_opengl_clear();
	gr_opengl_set_cull(1);

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
#endif	// !__EMSCRIPTEN__
}
