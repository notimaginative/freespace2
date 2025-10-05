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

#ifndef NDEBUG
	#define CHECK_FOR_ERRORS()	\
		do {	\
			GLenum error = glGetError();	\
			if (error != GL_NO_ERROR) {	\
				printf("!!DEBUG!! OpenGL Error: %d\n", error);	\
			}	\
		} while(false)
#else
	#define CHECK_FOR_ERRORS()
#endif

bool opengl_init_prototypes();
void opengl_set_variables();
void opengl_init_viewport();

void opengl_stuff_fog_value(float z, float *f_val);


void opengl_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt);
void opengl_set_texture_state(gr_texture_source ts);

void opengl_tcache_init();
void opengl_tcache_cleanup();
void opengl_tcache_flush();
void opengl_tcache_frame();
int opengl_tcache_set(int bitmap_id, int bitmap_type, float *u_scale, float *v_scale, int fail_on_full = 0, int sx = -1, int sy = -1, int force = 0);

// gr_* pointer functions
void gr_opengl_force_fullscreen();
void gr_opengl_force_windowed();
void gr_opengl_toggle_fullscreen();
void gr_opengl_clear();
void gr_opengl_reset_clip();
uint gr_opengl_lock();
void gr_opengl_unlock();
void gr_opengl_zbias(int bias);
void gr_opengl_set_cull(int cull);
void gr_opengl_activate(int active);
void gr_opengl_rect(int x,int y,int w,int h);
void gr_opengl_shade(int x,int y,int w,int h);
void gr_opengl_aabitmap_ex(int x,int y,int w,int h,int sx,int sy);
void gr_opengl_aabitmap(int x, int y);
void gr_opengl_string( int sx, int sy, const char *s );
void gr_opengl_line(int x1,int y1,int x2,int y2);
void gr_opengl_aaline(vertex *v1, vertex *v2);
void gr_opengl_aalines(vertex *verts, int count);
void gr_opengl_gradient(int x1,int y1,int x2,int y2);
void gr_opengl_circle( int xc, int yc, int d );
void gr_opengl_pixel(int x, int y);
void gr_opengl_cross_fade(int bmap1, int bmap2, int x1, int y1, int x2, int y2, float pct);
void gr_opengl_flash(int r, int g, int b);
void gr_opengl_tmapper( int nverts, vertex **verts, uint flags );
void gr_opengl_scaler(vertex *va, vertex *vb );
void gr_opengl_aascaler(vertex *va, vertex *vb);
void gr_opengl_set_gamma(float gamma);
void gr_opengl_preload_init();
int gr_opengl_preload(int bitmap_num, int is_aabitmap);
void gr_opengl_flip();
void gr_opengl_set_clip(int x, int y, int w, int h);
void gr_opengl_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far);
void gr_opengl_zbuffer_clear(int mode);
void gr_opengl_print_screen(const char *filename);
void gr_opengl_fade_in(int instantaneous);
void gr_opengl_fade_out(int instantaneous);
void gr_opengl_get_region(int front, int w, int h, ubyte *data);
int gr_opengl_save_screen();
void gr_opengl_restore_screen(int id);
void gr_opengl_free_screen(int id);
void gr_opengl_dump_frame_start(int first_frame, int frames_between_dumps);
void gr_opengl_dump_frame_stop();
void gr_opengl_dump_frame();
void gr_opengl_stream_start(int x, int y, int w, int h);
void gr_opengl_stream_frame(const SDL_Surface *frame);
void gr_opengl_stream_stop();
void gr_opengl_set_viewport(int width, int height);
void gr_opengl_release_texture(int handle);

// GL function prototypes
typedef struct GL_func_context {
	PFNGLACTIVETEXTUREPROC glActiveTexture;
	PFNGLBINDTEXTUREPROC glBindTexture;
	PFNGLBLENDFUNCPROC glBlendFunc;
	PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
	PFNGLCLEARCOLORPROC glClearColor;
	PFNGLCLEARPROC glClear;
	PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture;
	PFNGLCOLOR4UBPROC glColor4ub;
	PFNGLCOLORPOINTERPROC glColorPointer;
	PFNGLCOPYTEXIMAGE2DPROC glCopyTexImage2D;
	PFNGLDELETETEXTURESPROC glDeleteTextures;
	PFNGLDEPTHFUNCPROC glDepthFunc;
	PFNGLDEPTHMASKPROC glDepthMask;
	PFNGLDEPTHRANGEPROC glDepthRange;
	PFNGLDISABLECLIENTSTATEPROC glDisableClientState;
	PFNGLDISABLEPROC glDisable;
	PFNGLDRAWARRAYSPROC glDrawArrays;
	PFNGLENABLECLIENTSTATEPROC glEnableClientState;
	PFNGLENABLEPROC glEnable;
	PFNGLFLUSHPROC glFlush;
	PFNGLFOGFPROC glFogf;
	PFNGLFOGFVPROC glFogfv;
	PFNGLFOGIPROC glFogi;
	PFNGLFRONTFACEPROC glFrontFace;
	PFNGLGENTEXTURESPROC glGenTextures;
	PFNGLGETERRORPROC glGetError;
	PFNGLGETINTEGERVPROC glGetIntegerv;
	PFNGLGETSTRINGPROC glGetString;
	PFNGLHINTPROC glHint;
	PFNGLLOADIDENTITYPROC glLoadIdentity;
	PFNGLMATRIXMODEPROC glMatrixMode;
	PFNGLORTHOPROC glOrtho;
	PFNGLPIXELSTOREIPROC glPixelStorei;
	PFNGLPOLYGONOFFSETPROC glPolygonOffset;
	PFNGLPOPATTRIBPROC glPopAttrib;
	PFNGLPOPCLIENTATTRIBPROC glPopClientAttrib;
	PFNGLPUSHATTRIBPROC glPushAttrib;
	PFNGLPUSHCLIENTATTRIBPROC glPushClientAttrib;
	PFNGLREADBUFFERPROC glReadBuffer;
	PFNGLREADPIXELSPROC glReadPixels;
	PFNGLSCALEFPROC glScalef;
	PFNGLSCISSORPROC glScissor;
	PFNGLSHADEMODELPROC glShadeModel;
	PFNGLTEXCOORDPOINTERPROC glTexCoordPointer;
	PFNGLTEXENVFPROC glTexEnvf;
	PFNGLTEXENVFVPROC glTexEnvfv;
	PFNGLTEXENVIPROC glTexEnvi;
	PFNGLTEXIMAGE2DPROC glTexImage2D;
	PFNGLTEXPARAMETERIPROC glTexParameteri;
	PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
	PFNGLVERTEXPOINTERPROC glVertexPointer;
	PFNGLVIEWPORTPROC glViewport;
} GL_func_context;

extern GL_func_context GL_ctx;

#endif	// _OPENGLINTERNAL_H
