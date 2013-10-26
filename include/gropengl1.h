/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef _GROPENGL1_H
#define _GROPENGL1_H

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

extern PFNGLSECONDARYCOLORPOINTEREXTPROC vglSecondaryColorPointerEXT;

void opengl1_init();
void opengl1_cleanup();

void opengl1_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt);
void opengl1_set_texture_state(gr_texture_source ts);

void opengl1_tcache_init (int use_sections);
void opengl1_tcache_cleanup();
void opengl1_tcache_flush();
void opengl1_tcache_frame();
int opengl1_tcache_set(int bitmap_id, int bitmap_type, float *u_scale, float *v_scale, int fail_on_full = 0, int sx = -1, int sy = -1, int force = 0);

void gr_opengl1_rect(int x,int y,int w,int h);
void gr_opengl1_shade(int x,int y,int w,int h);
void gr_opengl1_aabitmap_ex(int x,int y,int w,int h,int sx,int sy);
void gr_opengl1_aabitmap(int x, int y);
void gr_opengl1_string( int sx, int sy, const char *s );
void gr_opengl1_line(int x1,int y1,int x2,int y2);
void gr_opengl1_aaline(vertex *v1, vertex *v2);
void gr_opengl1_gradient(int x1,int y1,int x2,int y2);
void gr_opengl1_circle( int xc, int yc, int d );
void gr_opengl1_pixel(int x, int y);
void gr_opengl1_cross_fade(int bmap1, int bmap2, int x1, int y1, int x2, int y2, float pct);
void gr_opengl1_flash(int r, int g, int b);
void gr_opengl1_tmapper( int nverts, vertex **verts, uint flags );
void gr_opengl1_scaler(vertex *va, vertex *vb );
void gr_opengl1_save_mouse_area(int x, int y, int w, int h);
void gr_opengl1_set_gamma(float gamma);
void gr_opengl1_preload_init();
int gr_opengl1_preload(int bitmap_num, int is_aabitmap);

#endif	// _GROPENGL1_H
