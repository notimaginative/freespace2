/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef GRGLES2INTERNAL_H
#define GRGLES2INTERNAL_H

#include "pstypes.h"

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

void gles2_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt);

void gles2_tcache_init();
void gles2_tcache_cleanup();
void gles2_tcache_frame();
void gles2_tcache_flush();
int gles2_tcache_set(int bitmap_id, int bitmap_type, int fail_on_full = 0);
void gles2_set_texture_state(gr_texture_source ts);

// shader program types
typedef enum {
	PROG_INVALID = -1,
	PROG_AABITMAP = 0,
	PROG_TEX = 1,
	PROG_TEX_FOG = 2,
	PROG_COLOR = 3,
	PROG_COLOR_FOG = 4,
	PROG_WINDOW = 5,
	PROG_NONDARK = 6,
	PROG_NONDARK_FOG = 7,
} sdr_prog_t;

// shader variable indexes
enum {
	SDRI_POSITION = 1,
	SDRI_COLOR = 2,
	SDRI_SEC_COLOR = 3,
	SDRI_TEXCOORD = 4
};

extern int GLES2_viewport_x;
extern int GLES2_viewport_y;
extern int GLES2_viewport_w;
extern int GLES2_viewport_h;
extern float GLES2_viewport_scale_w;
extern float GLES2_viewport_scale_h;
extern int GLES2_min_texture_width;
extern int GLES2_max_texture_width;
extern int GLES2_min_texture_height;
extern int GLES2_max_texture_height;

int gles2_shader_init();
void gles2_shader_cleanup();
void gles2_shader_use(sdr_prog_t prog);
void gles2_shader_update(int width = 0, int height = 0);

void gr_gles2_flip();
void gr_gles2_set_clip(int x, int y, int w, int h);
void gr_gles2_reset_clip();
void gr_gles2_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far);
void gr_gles2_zbuffer_clear(int mode);
void gr_gles2_print_screen(const char *filename);
void gr_gles2_fade_in(int instantaneous);
void gr_gles2_fade_out(int instantaneous);
void gr_gles2_get_region(int front, int w, int h, ubyte *data);
int gr_gles2_save_screen();
void gr_gles2_restore_screen(int);
void gr_gles2_free_screen(int);
void gr_gles2_dump_frame_start(int first_frame, int frames_between_dumps);
void gr_gles2_dump_frame_stop();
void gr_gles2_dump_frame();
void gr_gles2_stream_start(int x, int y, int w, int h);
void gr_gles2_stream_frame(const SDL_Surface *frame);
void gr_gles2_stream_stop();
void gr_gles2_set_viewport(int width, int height);
void gr_gles2_preload_init();
int gr_gles2_preload(int bitmap_num, int is_aabitmap);
void gr_gles2_set_gamma(float);
void gr_gles2_release_texture(int handle);
void gr_gles2_rect(int x, int y, int w, int h);
void gr_gles2_shade(int x, int y, int w, int h);
void gr_gles2_aabitmap_ex(int x, int y, int w, int h, int sx, int sy);
void gr_gles2_aabitmap(int x, int y);
void gr_gles2_string(int sx, int sy, const char *s);
void gr_gles2_line(int x1, int y1, int x2, int y2);
void gr_gles2_aaline(vertex *v1, vertex *v2);
void gr_gles2_aalines(vertex *verts, int count);
void gr_gles2_gradient(int x1, int y1, int x2, int y2);
void gr_gles2_circle(int xc, int yc, int d);
void gr_gles2_pixel(int x, int y);
void gr_gles2_cross_fade(int bmap1, int bmap2, int x1, int y1, int x2, int y2, float pct);
void gr_gles2_flash(int r, int g, int b);
void gr_gles2_tmapper(int nverts, vertex **verts, uint flags);
void gr_gles2_scaler(vertex *va, vertex *vb);
void gr_gles2_aascaler(vertex *va, vertex *vb);
void gr_gles2_set_cull(int cull);
void gr_gles2_clear();
void gr_gles2_zbias(int bias);
void gr_gles2_activate(int active);
uint gr_gles2_lock();
void gr_gles2_unlock();

// GLES2 function prototypes
typedef struct GLES2_func_context {
	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
	PFNGLBINDBUFFERPROC glBindFramebuffer;
	PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
	PFNGLBINDTEXTUREPROC glBindTexture;
	PFNGLBLENDFUNCPROC glBlendFunc;
	PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
	PFNGLCLEARCOLORPROC glClearColor;
	PFNGLCLEARPROC glClear;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLCOPYTEXIMAGE2DPROC glCopyTexImage2D;
	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLDELETETEXTURESPROC glDeleteTextures;
	PFNGLDEPTHFUNCPROC glDepthFunc;
	PFNGLDEPTHMASKPROC glDepthMask;
	PFNGLDEPTHRANGEFPROC glDepthRangef;
	PFNGLDISABLEPROC glDisable;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	PFNGLDRAWARRAYSPROC glDrawArrays;
	PFNGLENABLEPROC glEnable;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLFLUSHPROC glFlush;
	PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
	PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	PFNGLFRONTFACEPROC glFrontFace;
	PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
	PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
	PFNGLGENTEXTURESPROC glGenTextures;
	PFNGLGETERRORPROC glGetError;
	PFNGLGETINTEGERVPROC glGetIntegerv;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLGETSTRINGPROC glGetString;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLPIXELSTOREIPROC glPixelStorei;
	PFNGLPOLYGONOFFSETPROC glPolygonOffset;
	PFNGLREADPIXELSPROC glReadPixels;
	PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
	PFNGLSCISSORPROC glScissor;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLTEXIMAGE2DPROC glTexImage2D;
	PFNGLTEXPARAMETERIPROC glTexParameteri;
	PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLVIEWPORTPROC glViewport;
} GLES2_func_context;

extern GLES2_func_context GLES2_ctx;


#endif	// GRGLES2INTERNAL_H
