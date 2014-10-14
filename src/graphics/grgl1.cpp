/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "gropengl.h"
#include "grgl1.h"
#include "gropenglinternal.h"
#include "2d.h"
#include "mouse.h"
#include "pstypes.h"
#include "cfile.h"
#include "bmpman.h"
#include "grinternal.h"


int OGL_fog_mode = 0;

int GL_one_inited = 0;


volatile int GL_activate = 0;
volatile int GL_deactivate = 0;

static GLuint Gr_saved_screen_tex = 0;

static int Gr_opengl_mouse_saved = 0;
static int Gr_opengl_mouse_saved_x = 0;
static int Gr_opengl_mouse_saved_y = 0;
static int Gr_opengl_mouse_saved_w = 0;
static int Gr_opengl_mouse_saved_h = 0;
static ubyte *Gr_opengl_mouse_saved_data = NULL;


PFNGLSECONDARYCOLORPOINTERPROC vglSecondaryColorPointer = NULL;


static gr_alpha_blend GL_current_alpha_blend = (gr_alpha_blend) -1;
static gr_zbuffer_type GL_current_zbuffer_type = (gr_zbuffer_type) -1;

void opengl1_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt)
{
	opengl1_set_texture_state(ts);

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

void opengl1_cleanup()
{
	if ( !GL_one_inited ) {
		return;
	}

	gr_opengl1_reset_clip();
	gr_opengl1_clear();
	gr_opengl1_flip();

	opengl1_tcache_cleanup();

	GL_one_inited = 0;
}

static void opengl1_init_func_pointers()
{
	gr_screen.gf_flip = gr_opengl1_flip;
	gr_screen.gf_set_clip = gr_opengl1_set_clip;
	gr_screen.gf_reset_clip = gr_opengl1_reset_clip;

	gr_screen.gf_clear = gr_opengl1_clear;

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

	gr_screen.gf_set_gamma = gr_opengl1_set_gamma;

	gr_screen.gf_lock = gr_opengl1_lock;
	gr_screen.gf_unlock = gr_opengl1_unlock;

	gr_screen.gf_fog_set = gr_opengl1_fog_set;

	gr_screen.gf_get_region = gr_opengl1_get_region;

	gr_screen.gf_set_cull = gr_opengl1_set_cull;

	gr_screen.gf_cross_fade = gr_opengl1_cross_fade;

	gr_screen.gf_preload_init = gr_opengl1_preload_init;
	gr_screen.gf_preload = gr_opengl1_preload;

	gr_screen.gf_zbias = gr_opengl1_zbias;

	gr_screen.gf_force_windowed = gr_opengl_force_windowed;
	gr_screen.gf_force_fullscreen = gr_opengl_force_fullscreen;
	gr_screen.gf_toggle_fullscreen = gr_opengl_toggle_fullscreen;

	gr_screen.gf_set_viewport = gr_opengl1_set_viewport;

	gr_screen.gf_activate = gr_opengl1_activate;
}

void opengl1_init()
{
	if (GL_one_inited) {
		return;
	}

	/*
	  1 = use secondary color ext
	  2 = use opengl linear fog
	 */
	OGL_fog_mode = 2;

	// only available with OpenGL 1.2+, must get ptr for Windows
	vglSecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTERPROC)SDL_GL_GetProcAddress("glSecondaryColorPointer");

	if (vglSecondaryColorPointer) {
		OGL_fog_mode = 1;
	}

	mprintf(("  Fog mode: %s\n", (OGL_fog_mode == 1) ? "secondary color" : "linear"));

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

	opengl1_init_func_pointers();
	opengl1_tcache_init();

	gr_opengl1_clear();

	GL_one_inited = 1;
}

void gr_opengl1_activate(int active)
{
	if (active) {
		GL_activate++;

		// don't grab key/mouse if cmdline says so or if we're fullscreen
	//	if(!Cmdline_no_grab && !(SDL_GetVideoSurface()->flags & SDL_FULLSCREEN)) {
	//		SDL_WM_GrabInput(SDL_GRAB_ON);
	//	}
	} else {
		GL_deactivate++;

		// let go of mouse/keyboard
	//	SDL_WM_GrabInput(SDL_GRAB_OFF);
	}
}

void gr_opengl1_clear()
{
	glClearColor(gr_screen.current_clear_color.red / 255.0,
		gr_screen.current_clear_color.green / 255.0,
		gr_screen.current_clear_color.blue / 255.0, 1.0);

	glClear( GL_COLOR_BUFFER_BIT );
}

void gr_opengl1_flip()
{
	if ( !GL_one_inited ) {
		return;
	}

	gr_reset_clip();

	mouse_eval_deltas();

	Gr_opengl_mouse_saved = 0;

	if ( mouse_is_visible() )       {
		int mx, my;

	 	gr_reset_clip();
	 	mouse_get_pos( &mx, &my );

	 	gr_opengl1_save_mouse_area(mx,my,32,32);

	 	if (Gr_cursor == -1) {
#ifndef NDEBUG
	 		gr_set_color(255,255,255);
	 		gr_line(mx, my, mx+7, my + 7);
	 		gr_line(mx, my, mx+5, my );
	 		gr_line(mx, my, mx, my+5);
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

	opengl1_tcache_frame();

	int cnt = GL_activate;
	if ( cnt )      {
		GL_activate-=cnt;
		opengl1_tcache_flush();
		// gr_opengl_clip_cursor(1); /* mouse grab, see opengl_activate */
	}

	cnt = GL_deactivate;
	if ( cnt )      {
		GL_deactivate-=cnt;
		// gr_opengl_clip_cursor(0);  /* mouse grab, see opengl_activate */
	}
}

void gr_opengl1_set_clip(int x,int y,int w,int h)
{
	// check for sanity of parameters
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	if (x >= gr_screen.max_w)
		x = gr_screen.max_w - 1;
	if (y >= gr_screen.max_h)
		y = gr_screen.max_h - 1;

	if (x + w > gr_screen.max_w)
		w = gr_screen.max_w - x;
	if (y + h > gr_screen.max_h)
		h = gr_screen.max_h - y;

	if (w > gr_screen.max_w)
		w = gr_screen.max_w;
	if (h > gr_screen.max_h)
		h = gr_screen.max_h;

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

	glEnable(GL_SCISSOR_TEST);
	glScissor(x, GL_viewport_h-y-h, w, h);
}

void gr_opengl1_reset_clip()
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

void gr_opengl1_print_screen(const char *filename)
{
	char tmp[MAX_FILENAME_LEN];
	ubyte *buf = NULL;

	SDL_strlcpy( tmp, filename, sizeof(tmp) );
	SDL_strlcat( tmp, NOX(".tga"), sizeof(tmp) );

	buf = (ubyte*)malloc(GL_viewport_w * GL_viewport_h * 3);

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
	cfwrite_ushort( (ushort)GL_viewport_w, f );	//	Width;
	cfwrite_ushort( (ushort)GL_viewport_h, f );	//	Height;
	cfwrite_ubyte( 24, f );	//PixelDepth;
	cfwrite_ubyte( 0, f );	//ImageDesc;

	memset(buf, 0, GL_viewport_w * GL_viewport_h * 3);

	glReadPixels(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h, GL_BGR, GL_UNSIGNED_BYTE, buf);

	cfwrite(buf, GL_viewport_w * GL_viewport_h * 3, 1, f);

	cfclose(f);

	free(buf);
}

void gr_opengl1_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far)
{
	SDL_assert((r >= 0) && (r < 256));
	SDL_assert((g >= 0) && (g < 256));
	SDL_assert((b >= 0) && (b < 256));

	if (fog_mode == GR_FOGMODE_NONE) {
		if (gr_screen.current_fog_mode != fog_mode) {
			glDisable(GL_FOG);

			if (OGL_fog_mode == 1) {
				glDisable(GL_COLOR_SUM);
			}
		}

		gr_screen.current_fog_mode = fog_mode;

		return;
	}

	if (gr_screen.current_fog_mode != fog_mode) {
		glEnable(GL_FOG);

		if (OGL_fog_mode == 1) {
			glEnable(GL_COLOR_SUM);
		} else if (OGL_fog_mode == 2) {
			glFogi(GL_FOG_MODE, GL_LINEAR);
		}

		gr_screen.current_fog_mode = fog_mode;
	}

	if ( (gr_screen.current_fog_color.red != r) ||
			(gr_screen.current_fog_color.green != g) ||
			(gr_screen.current_fog_color.blue != b) ) {
		GLfloat fc[4];

		gr_init_color( &gr_screen.current_fog_color, r, g, b );

		fc[0] = (float)r/255.0;
		fc[1] = (float)g/255.0;
		fc[2] = (float)b/255.0;
		fc[3] = 1.0;

		glFogfv(GL_FOG_COLOR, fc);
	}

	if( (fog_near >= 0.0f) && (fog_far >= 0.0f) &&
			((fog_near != gr_screen.fog_near) ||
			(fog_far != gr_screen.fog_far)) ) {
		gr_screen.fog_near = fog_near;
		gr_screen.fog_far = fog_far;

		if (OGL_fog_mode == 2) {
			glFogf(GL_FOG_START, fog_near);
			glFogf(GL_FOG_END, fog_far);
		}
	}
}

void gr_opengl1_set_cull(int cull)
{
	if (cull) {
		glEnable (GL_CULL_FACE);
		glFrontFace (GL_CCW);
	} else {
		glDisable (GL_CULL_FACE);
	}
}

void gr_opengl1_zbuffer_clear(int mode)
{
	if (mode) {
		Gr_zbuffering = 1;
		Gr_zbuffering_mode = GR_ZBUFF_FULL;
		Gr_global_zbuffering = 1;

		opengl1_set_state( TEXTURE_SOURCE_NONE, ALPHA_BLEND_NONE, ZBUFFER_TYPE_FULL );
		glClear ( GL_DEPTH_BUFFER_BIT );
	} else {
		Gr_zbuffering = 0;
		Gr_zbuffering_mode = GR_ZBUFF_NONE;
		Gr_global_zbuffering = 0;
	}
}

void gr_opengl1_fade_in(int instantaneous)
{
	// Empty - DDOI
}

void gr_opengl1_fade_out(int instantaneous)
{
	// Empty - DDOI
}

void gr_opengl1_get_region(int front, int w, int h, ubyte *data)
{
	if (front) {
		glReadBuffer(GL_FRONT);
	} else {
		glReadBuffer(GL_BACK);
	}

	opengl1_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, GL_viewport_w);

	int x = GL_viewport_x;
	int y = (GL_viewport_y+GL_viewport_h)-h-1;

	GLenum pxtype = GL_UNSIGNED_SHORT_1_5_5_5_REV;

	if (gr_screen.bytes_per_pixel == 4) {
		pxtype = GL_UNSIGNED_BYTE;
	}

	glReadPixels(x, y, w, h, GL_BGRA, pxtype, data);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void gr_opengl1_save_mouse_area(int x, int y, int w, int h)
{
	int x1, y1, x2, y2;

	w = fl2i((w * GL_viewport_scale_w) + 0.5f);
	h = fl2i((h * GL_viewport_scale_h) + 0.5f);

	x1 = x;
	y1 = y;
	x2 = x+w-1;
	y2 = y+h-1;

	CAP(x1, 0, GL_viewport_w);
	CAP(x2, 0, GL_viewport_w);
	CAP(y1, 0, GL_viewport_h);
	CAP(y2, 0, GL_viewport_h);

	Gr_opengl_mouse_saved_x = x1;
	Gr_opengl_mouse_saved_y = y1;
	Gr_opengl_mouse_saved_w = x2 - x1 + 1;
	Gr_opengl_mouse_saved_h = y2 - y1 + 1;

	if ( (Gr_opengl_mouse_saved_w < 1) || (Gr_opengl_mouse_saved_h < 1) ) {
		return;
	}

	if (Gr_opengl_mouse_saved_data == NULL) {
		Gr_opengl_mouse_saved_data = (ubyte*)malloc(w * h * gr_screen.bytes_per_pixel);

		if ( !Gr_opengl_mouse_saved_data ) {
			return;
		}
	}

	opengl1_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	x1 = GL_viewport_x+Gr_opengl_mouse_saved_x;
	y1 = (GL_viewport_y+GL_viewport_h)-Gr_opengl_mouse_saved_y-Gr_opengl_mouse_saved_h;

	GLenum pxtype = GL_UNSIGNED_SHORT_1_5_5_5_REV;

	if (gr_screen.bytes_per_pixel == 4) {
		pxtype = GL_UNSIGNED_BYTE;
	}

	glReadBuffer(GL_BACK);
	glReadPixels(x1, y1, Gr_opengl_mouse_saved_w, Gr_opengl_mouse_saved_h,
			GL_BGRA, pxtype, Gr_opengl_mouse_saved_data);

	Gr_opengl_mouse_saved = 1;
}

int gr_opengl1_save_screen()
{
	gr_reset_clip();

	if (Gr_saved_screen_tex) {
		mprintf(( "Screen already saved!\n" ));
		return -1;
	}

	glGenTextures(1, &Gr_saved_screen_tex);

	if ( !Gr_saved_screen_tex ) {
		mprintf(( "Couldn't create texture for saved screen!\n" ));
		return -1;
	}

	opengl1_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	glBindTexture(GL_TEXTURE_2D, Gr_saved_screen_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glReadBuffer(GL_FRONT);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GL_viewport_x, GL_viewport_y,
			GL_viewport_w, GL_viewport_h, 0);

	if (Gr_opengl_mouse_saved) {
		int x = Gr_opengl_mouse_saved_x;
		int y = GL_viewport_h-Gr_opengl_mouse_saved_y-Gr_opengl_mouse_saved_h;

		GLenum pxtype = GL_UNSIGNED_SHORT_1_5_5_5_REV;

		if (gr_screen.bytes_per_pixel == 4) {
			pxtype = GL_UNSIGNED_BYTE;
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, Gr_opengl_mouse_saved_w,
				Gr_opengl_mouse_saved_h, GL_BGRA, pxtype,
				Gr_opengl_mouse_saved_data);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

void gr_opengl1_restore_screen(int id)
{
	gr_reset_clip();

	if ( !Gr_saved_screen_tex ) {
		gr_clear();
		return;
	}

	glBindTexture(GL_TEXTURE_2D, Gr_saved_screen_tex);

	if (Gr_opengl_mouse_saved) {
		int x = Gr_opengl_mouse_saved_x;
		int y = GL_viewport_h-Gr_opengl_mouse_saved_y-Gr_opengl_mouse_saved_h;

		GLenum pxtype = GL_UNSIGNED_SHORT_1_5_5_5_REV;

		if (gr_screen.bytes_per_pixel == 4) {
			pxtype = GL_UNSIGNED_BYTE;
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, Gr_opengl_mouse_saved_w,
				Gr_opengl_mouse_saved_h, GL_BGRA, pxtype,
				Gr_opengl_mouse_saved_data);
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glScalef(1.0f, -1.0f, 1.0f);

	int tex_coord[] = { 0, 0, 0, 1, 1, 0, 1, 1 };
	int ver_coord[] = { GL_viewport_x, GL_viewport_y, GL_viewport_x,
			GL_viewport_h, GL_viewport_w, GL_viewport_y, GL_viewport_w,
			GL_viewport_h
	};

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glTexCoordPointer(2, GL_INT, 0, &tex_coord);
	glVertexPointer(2, GL_INT, 0, &ver_coord);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 0);
}

void gr_opengl1_free_screen(int id)
{
	if (Gr_saved_screen_tex) {
		glDeleteTextures(1, &Gr_saved_screen_tex);
		Gr_saved_screen_tex = 0;
	}
}

void gr_opengl1_dump_frame_start(int first_frame, int frames_between_dumps)
{
	STUB_FUNCTION;
}

void gr_opengl1_dump_frame_stop()
{
	STUB_FUNCTION;
}

void gr_opengl1_dump_frame()
{
	STUB_FUNCTION;
}

uint gr_opengl1_lock()
{
	return 1;
}

void gr_opengl1_unlock()
{
}

void gr_opengl1_zbias(int bias)
{
	if (bias) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0, -bias);
	} else {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void gr_opengl1_set_viewport(int width, int height)
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
