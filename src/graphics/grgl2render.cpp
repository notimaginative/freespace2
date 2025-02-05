/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <SDL3/SDL_opengles2.h>

#include "pstypes.h"
#include "2d.h"
#include "gropengl.h"
#include "gropenglinternal.h"
#include "grgl2.h"
#include "bmpman.h"
#include "grinternal.h"
#include "3d.h"
#include "neb.h"
#include "line.h"
#include "palman.h"


#define NEBULA_COLORS	20


static void opengl2_tmapper_internal(int nv, vertex **verts, uint flags, int is_scaler)
{
	int i;

	gr_texture_source texture_source = TEXTURE_SOURCE_NONE;
	gr_alpha_blend alpha_blend = ALPHA_BLEND_ALPHA_BLEND_ALPHA;
	gr_zbuffer_type zbuffer_type = ZBUFFER_TYPE_NONE;

	if (Gr_zbuffering) {
		if ( is_scaler || (gr_screen.current_alphablend_mode == GR_ALPHABLEND_FILTER) ) {
			zbuffer_type = ZBUFFER_TYPE_READ;
		} else {
			zbuffer_type = ZBUFFER_TYPE_FULL;
		}
	}

	int tmap_type = TCACHE_TYPE_NORMAL;

	ubyte r = 255, g = 255, b = 255, a = 255;

	if ( !(flags & TMAP_FLAG_TEXTURED) ) {
		r = gr_screen.current_color.red;
		g = gr_screen.current_color.green;
		b = gr_screen.current_color.blue;
	}

	if (gr_screen.current_alphablend_mode == GR_ALPHABLEND_FILTER) {
		alpha_blend = ALPHA_BLEND_ALPHA_ADDITIVE;

		// Blend with screen pixel using src*alpha+dst

		if (gr_screen.current_alpha < 1.0f) {
			r = ubyte((r * gr_screen.current_alpha) + 0.5f);
			g = ubyte((g * gr_screen.current_alpha) + 0.5f);
			b = ubyte((b * gr_screen.current_alpha) + 0.5f);
		}
	}

	if (flags & TMAP_FLAG_BITMAP_SECTION) {
		SDL_assert( !(flags & TMAP_FLAG_BITMAP_INTERFACE) );
		tmap_type = TCACHE_TYPE_BITMAP_SECTION;
	} else if (flags & TMAP_FLAG_BITMAP_INTERFACE) {
		SDL_assert( !(flags & TMAP_FLAG_BITMAP_SECTION) );
		tmap_type = TCACHE_TYPE_BITMAP_INTERFACE;
	}

	if (flags & TMAP_FLAG_TEXTURED) {
		if ( !opengl2_tcache_set(gr_screen.current_bitmap, tmap_type) ) {
			mprintf(( "Not rendering a texture because it didn't fit in VRAM!\n" ));
			return;
		}

		// use non-filtered textures for bitmap sections and UI graphics
		switch (tmap_type) {
			case TCACHE_TYPE_BITMAP_INTERFACE:
			case TCACHE_TYPE_BITMAP_SECTION:
				texture_source = TEXTURE_SOURCE_NO_FILTERING;
				break;

			default:
				texture_source = TEXTURE_SOURCE_DECAL;
				break;
		}
	}


	opengl2_set_state( texture_source, alpha_blend, zbuffer_type );

	float ox = gr_screen.offset_x * 16.0f;
	float oy = gr_screen.offset_y * 16.0f;

	if (flags & TMAP_FLAG_PIXEL_FOG) {
		int fr, fg, fb;
		int ra, ga, ba;
		float sx, sy;

		ra = ga = ba = 0;

		for (i = nv-1; i >= 0; i--) {
			vertex * va = verts[i];

			sx = (va->sx * 16.0f + ox) / 16.0f;
			sy = (va->sy * 16.0f + oy) / 16.0f;

			neb2_get_pixel((int)sx, (int)sy, &fr, &fg, &fb);

			ra += fr;
			ga += fg;
			ba += fb;
		}

		ra /= nv;
		ga /= nv;
		ba /= nv;

		gr_opengl2_fog_set(GR_FOGMODE_FOG, ra, ga, ba, gr_screen.fog_near, gr_screen.fog_far);
	}

	opengl_alloc_render_buffer(nv);

	int rb_offset = 0;

	float sx, sy, sz = 0.99f, rhw = 1.0f;

	bool bZval = (Gr_zbuffering || (flags & TMAP_FLAG_NEBULA));
	bool bCorrect = ((flags & TMAP_FLAG_CORRECT) == TMAP_FLAG_CORRECT);
	bool bAlpha = ((flags & TMAP_FLAG_ALPHA) == TMAP_FLAG_ALPHA);
	bool bNebula = ((flags & TMAP_FLAG_NEBULA) == TMAP_FLAG_NEBULA);
	bool bRamp = ((flags & TMAP_FLAG_RAMP) && (flags & TMAP_FLAG_GOURAUD));
	bool bRGB = ((flags & TMAP_FLAG_RGB) && (flags & TMAP_FLAG_GOURAUD));
	bool bTextured = ((flags & TMAP_FLAG_TEXTURED) == TMAP_FLAG_TEXTURED);
	bool bFog = ((flags & TMAP_FLAG_PIXEL_FOG) == TMAP_FLAG_PIXEL_FOG);

	for (i = nv-1; i >= 0; i--) {
		vertex *va = verts[i];

		if (bZval) {
			sz = 1.0f - 1.0f / (1.0f + va->z / (32768.0f / 256.0f));

			if ( sz > 0.98f ) {
				sz = 0.98f;
			}
		}

		if (bCorrect) {
			rhw = 1.0f / va->sw;
		}

		if (bAlpha) {
			a = va->a;
		}

		if (bRGB) {
			// Make 0.75 be 256.0f
			r = (ubyte)Gr_gamma_lookup[va->r];
			g = (ubyte)Gr_gamma_lookup[va->g];
			b = (ubyte)Gr_gamma_lookup[va->b];
		} else if (bNebula) {
			int pal = (va->b*(NEBULA_COLORS-1))/255;
			r = gr_palette[pal*3+0];
			g = gr_palette[pal*3+1];
			b = gr_palette[pal*3+2];
		} else if (bRamp) {
			r = g = b = (ubyte)Gr_gamma_lookup[va->b];
		}

		render_buffer[rb_offset].r = r;
		render_buffer[rb_offset].g = g;
		render_buffer[rb_offset].b = b;
		render_buffer[rb_offset].a = a;

		if (bFog) {
			float f_val;

			opengl_stuff_fog_value(va->z, &f_val);

			render_buffer[rb_offset].sr = gr_screen.current_fog_color.red;
			render_buffer[rb_offset].sg = gr_screen.current_fog_color.green;
			render_buffer[rb_offset].sb = gr_screen.current_fog_color.blue;
			render_buffer[rb_offset].sa = (ubyte)(f_val * 255.0f);
		}

		sx = (va->sx * 16.0f + ox) / 16.0f;
		sy = (va->sy * 16.0f + oy) / 16.0f;

		if (bTextured) {
			render_buffer[rb_offset].u = va->u;
			render_buffer[rb_offset].v = va->v;
		}

		render_buffer[rb_offset].x = sx * rhw;
		render_buffer[rb_offset].y = sy * rhw;
		render_buffer[rb_offset].z = -sz * rhw;
		render_buffer[rb_offset].w = rhw;

		++rb_offset;
	}

	sdr_prog_t program = PROG_COLOR;

	if (flags & TMAP_FLAG_TEXTURED) {
		program = PROG_TEX;
	}

	if (flags & TMAP_FLAG_PIXEL_FOG) {
		// fog versions of color/tex shaders should one higher than non-fog version
		program = (sdr_prog_t)((int)program + 1);
	}

	opengl2_shader_use(program);

	if (flags & TMAP_FLAG_TEXTURED) {
		glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].u);
		glEnableVertexAttribArray(SDRI_TEXCOORD);
	}

	if (flags & TMAP_FLAG_PIXEL_FOG) {
		glVertexAttribPointer(SDRI_SEC_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rb_t), &render_buffer[0].sr);
		glEnableVertexAttribArray(SDRI_SEC_COLOR);
	}

	glVertexAttribPointer(SDRI_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rb_t), &render_buffer[0].r);
	glEnableVertexAttribArray(SDRI_COLOR);

	glVertexAttribPointer(SDRI_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
	glEnableVertexAttribArray(SDRI_POSITION);

	glDrawArrays(GL_TRIANGLE_FAN, 0, rb_offset);

	glDisableVertexAttribArray(SDRI_COLOR);
	glDisableVertexAttribArray(SDRI_SEC_COLOR);
	glDisableVertexAttribArray(SDRI_POSITION);
	glDisableVertexAttribArray(SDRI_TEXCOORD);
}

void opengl2_rect_internal(int x, int y, int w, int h, int r, int g, int b, int a)
{
	int saved_zbuf = gr_zbuffer_get();

	// no zbuffering, no culling
	gr_zbuffer_set(GR_ZBUFF_NONE);
	gr_opengl_set_cull(0);

	opengl2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE);

	opengl_alloc_render_buffer(4);

	render_buffer[0].x = i2fl(x);
	render_buffer[0].y = i2fl(y);

	render_buffer[1].x = i2fl(x);
	render_buffer[1].y = i2fl(y + h);

	render_buffer[2].x = i2fl(x + w);
	render_buffer[2].y = i2fl(y);

	render_buffer[3].x = i2fl(x + w);
	render_buffer[3].y = i2fl(y + h);

	opengl2_shader_use(PROG_COLOR);

	r = Gr_gamma_lookup[r];
	g = Gr_gamma_lookup[g];
	b = Gr_gamma_lookup[b];

	glVertexAttrib4f(SDRI_COLOR, r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);

	glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
	glEnableVertexAttribArray(SDRI_POSITION);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(SDRI_POSITION);

	// restore zbuffer and culling
	gr_zbuffer_set(saved_zbuf);
	gr_opengl_set_cull(1);
}

void opengl2_aabitmap_ex_internal(int x, int y, int w, int h, int sx, int sy)
{
	if ( (w < 1) || (h < 1) ) {
		return;
	}

	if ( !gr_screen.current_color.is_alphacolor ) {
		return;
	}

	if ( !opengl2_tcache_set(gr_screen.current_bitmap, TCACHE_TYPE_AABITMAP) ) {
		// Couldn't set texture
		mprintf(( "WARNING: Error setting aabitmap texture!\n" ));
		return;
	}

	opengl2_set_state( TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE );

	float u0, u1, v0, v1;
	float x1, x2, y1, y2;
	int bw, bh;

	bm_get_info( gr_screen.current_bitmap, &bw, &bh );

	u0 = i2fl(sx)/i2fl(bw);
	v0 = i2fl(sy)/i2fl(bh);

	u1 = i2fl(sx+w)/i2fl(bw);
	v1 = i2fl(sy+h)/i2fl(bh);

	x1 = i2fl(x+gr_screen.offset_x);
	y1 = i2fl(y+gr_screen.offset_y);
	x2 = i2fl(x+w+gr_screen.offset_x);
	y2 = i2fl(y+h+gr_screen.offset_y);


	opengl_alloc_render_buffer(4);

	render_buffer[0].x = x1;
	render_buffer[0].y = y1;
	render_buffer[0].u = u0;
	render_buffer[0].v = v0;

	render_buffer[1].x = x1;
	render_buffer[1].y = y2;
	render_buffer[1].u = u0;
	render_buffer[1].v = v1;

	render_buffer[2].x = x2;
	render_buffer[2].y = y1;
	render_buffer[2].u = u1;
	render_buffer[2].v = v0;

	render_buffer[3].x = x2;
	render_buffer[3].y = y2;
	render_buffer[3].u = u1;
	render_buffer[3].v = v1;

	opengl2_shader_use(PROG_AABITMAP);

	float r, g, b, a;
	gr_get_colorf(&r, &g, &b, &a);
	glVertexAttrib4f(SDRI_COLOR, r, g, b, a);

	glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
	glEnableVertexAttribArray(SDRI_POSITION);

	glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].u);
	glEnableVertexAttribArray(SDRI_TEXCOORD);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(SDRI_POSITION);
	glDisableVertexAttribArray(SDRI_TEXCOORD);
}

void gr_opengl2_rect(int x, int y, int w, int h)
{
	opengl2_rect_internal(x, y, w, h, gr_screen.current_color.red,
			gr_screen.current_color.green, gr_screen.current_color.blue,
			gr_screen.current_color.alpha);
}

void gr_opengl2_shade(int x, int y, int w, int h)
{
	int r,g,b,a;

	float shade1 = 1.0f;
	float shade2 = 6.0f;

	r = fl2i(gr_screen.current_shader.r*255.0f*shade1);
	CAP(r, 0, 255);
	g = fl2i(gr_screen.current_shader.g*255.0f*shade1);
	CAP(g, 0, 255);
	b = fl2i(gr_screen.current_shader.b*255.0f*shade1);
	CAP(b, 0, 255);
	a = fl2i(gr_screen.current_shader.c*255.0f*shade2);
	CAP(a, 0, 255);

	opengl2_rect_internal(x, y, w, h, r, g, b, a);
}

void gr_opengl2_aabitmap_ex(int x, int y, int w, int h, int sx, int sy)
{
	if ( (x > gr_screen.clip_right ) || ((x+w-1) < gr_screen.clip_left) )
		return;

	if ( (y > gr_screen.clip_bottom ) || ((y+h-1) < gr_screen.clip_top) )
		return;

	opengl2_aabitmap_ex_internal(x, y, w, h, sx, sy);
}

void gr_opengl2_aabitmap(int x, int y)
{
	int w, h;

	bm_get_info( gr_screen.current_bitmap, &w, &h, NULL );

	gr_opengl2_aabitmap_ex(x, y, w, h, 0, 0);
}

void gr_opengl2_string(int sx, int sy, const char *s)
{
	int width, spacing, letter;
	int x, y;
	int rb_offset;
	float u0, u1, v0, v1;
	float x1, x2, y1, y2;
	int bw, bh;
	float fbw, fbh;

	if ( !Current_font )	{
		return;
	}

	gr_set_bitmap(Current_font->bitmap_id, GR_ALPHABLEND_NONE, GR_BITBLT_MODE_NORMAL, 1.0f, -1, -1);

	if ( !opengl2_tcache_set( gr_screen.current_bitmap, TCACHE_TYPE_AABITMAP ) ) {
		// Couldn't set texture
		mprintf(( "WARNING: Error setting aabitmap texture!\n" ));
		return;
	}

	bm_get_info( gr_screen.current_bitmap, &bw, &bh );

	fbw = 1.0f / i2fl(bw);
	fbh = 1.0f / i2fl(bh);

	opengl2_set_state( TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE );

	// don't want to create a super huge buffer size (i.e. credits text)
	const int alocsize = 320;	// 80 characters max per render call
	opengl_alloc_render_buffer(alocsize);

	opengl2_shader_use(PROG_AABITMAP);

	glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
	glEnableVertexAttribArray(SDRI_POSITION);

	float r, g, b, a;
	gr_get_colorf(&r, &g, &b, &a);
	glVertexAttrib4f(SDRI_COLOR, r, g, b, a);

	glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].u);
	glEnableVertexAttribArray(SDRI_TEXCOORD);


	y = sy;

	if (sx==0x8000) {			//centered
		x = get_centered_x(s);
	} else {
		x = sx;
	}

	spacing = 0;
	rb_offset = 0;

	while (*s)	{
		x += spacing;

		while (*s== '\n' )	{
			s++;
			y += Current_font->h;
			if (sx==0x8000) {			//centered
				x = get_centered_x(s);
			} else {
				x = sx;
			}
		}
		if (*s == 0 ) break;

		letter = get_char_width(s[0],s[1],&width,&spacing);
		s++;

		//not in font, draw as space
		if (letter<0)	{
			continue;
		}

		int xd, yd, xc, yc;
		int wc, hc;

		// Check if this character is totally clipped
		if ( x + width < gr_screen.clip_left ) continue;
		if ( y + Current_font->h < gr_screen.clip_top ) continue;
		if ( x > gr_screen.clip_right ) continue;
		if ( y > gr_screen.clip_bottom ) continue;

		xd = yd = 0;
		if ( x < gr_screen.clip_left ) xd = gr_screen.clip_left - x;
		if ( y < gr_screen.clip_top ) yd = gr_screen.clip_top - y;
		xc = x+xd;
		yc = y+yd;

		wc = width - xd; hc = Current_font->h - yd;
		if ( xc + wc > gr_screen.clip_right ) wc = gr_screen.clip_right - xc;
		if ( yc + hc > gr_screen.clip_bottom ) hc = gr_screen.clip_bottom - yc;

		if ( wc < 1 ) continue;
		if ( hc < 1 ) continue;

		float u = i2fl(Current_font->bm_u[letter] + xd);
		float v = i2fl(Current_font->bm_v[letter] + yd);

		x1 = i2fl(xc + gr_screen.offset_x);
		y1 = i2fl(yc + gr_screen.offset_y);
		x2 = x1 + i2fl(wc);
		y2 = y1 + i2fl(hc);

		u0 = u * fbw;
		v0 = v * fbh;

		u1 = (u+i2fl(wc)) * fbw;
		v1 = (v+i2fl(hc)) * fbh;

		// maybe go ahead and draw
		if (rb_offset == alocsize) {
			glDrawArrays(GL_TRIANGLE_STRIP, 0, rb_offset);
			rb_offset = 0;
		}

		render_buffer[rb_offset].x = x1;
		render_buffer[rb_offset].y = y1;
		render_buffer[rb_offset].u = u0;
		render_buffer[rb_offset].v = v0;
		++rb_offset;

		render_buffer[rb_offset].x = x1;
		render_buffer[rb_offset].y = y2;
		render_buffer[rb_offset].u = u0;
		render_buffer[rb_offset].v = v1;
		++rb_offset;

		render_buffer[rb_offset].x = x2;
		render_buffer[rb_offset].y = y1;
		render_buffer[rb_offset].u = u1;
		render_buffer[rb_offset].v = v0;
		++rb_offset;

		render_buffer[rb_offset].x = x2;
		render_buffer[rb_offset].y = y2;
		render_buffer[rb_offset].u = u1;
		render_buffer[rb_offset].v = v1;
		++rb_offset;
	}

	if (rb_offset) {
		glDrawArrays(GL_TRIANGLE_STRIP, 0, rb_offset);
	}

	glDisableVertexAttribArray(SDRI_POSITION);
	glDisableVertexAttribArray(SDRI_TEXCOORD);
}

void gr_opengl2_line(int x1, int y1, int x2, int y2)
{
	opengl2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE);

	INT_CLIPLINE(x1, y1, x2, y2, gr_screen.clip_left, gr_screen.clip_top,
		gr_screen.clip_right, gr_screen.clip_bottom, return, void(), void());

	float sx1, sy1;
	float sx2, sy2;

	sx1 = i2fl(x1 + gr_screen.offset_x) + 0.5f;
	sy1 = i2fl(y1 + gr_screen.offset_y) + 0.5f;
	sx2 = i2fl(x2 + gr_screen.offset_x) + 0.5f;
	sy2 = i2fl(y2 + gr_screen.offset_y) + 0.5f;

	opengl_alloc_render_buffer(2);

	opengl2_shader_use(PROG_COLOR);

	glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
	glEnableVertexAttribArray(SDRI_POSITION);

	float r, g, b, a;
	gr_get_colorf(&r, &g, &b, &a);
	glVertexAttrib4f(SDRI_COLOR, r, g, b, a);

	if ( (x1 == x2) && (y1 == y2) ) {
		render_buffer[0].x = sx1;
		render_buffer[0].y = sy1;
		render_buffer[0].z = -0.99f;

		glDrawArrays(GL_POINTS, 0, 1);

		glDisableVertexAttribArray(SDRI_POSITION);

		return;
	}

	if (x1 == x2) {
		if (sy1 < sy2) {
			sy2 += 0.5f;
		} else {
			sy1 += 0.5f;
		}
	} else if (y1 == y2) {
		if (sx1 < sx2) {
			sx2 += 0.5f;
		} else {
			sx1 += 0.5f;
		}
	}

	render_buffer[0].x = sx2;
	render_buffer[0].y = sy2;
	render_buffer[0].z = -0.99f;

	render_buffer[1].x = sx1;
	render_buffer[1].y = sy1;
	render_buffer[1].z = -0.99f;

	glDrawArrays(GL_LINES, 0, 2);

	glDisableVertexAttribArray(SDRI_POSITION);
}

void gr_opengl2_aaline(vertex *v1, vertex *v2)
{
	gr_opengl2_line( fl2i(v1->sx), fl2i(v1->sy), fl2i(v2->sx), fl2i(v2->sy) );
}

void gr_opengl2_gradient(int x1, int y1, int x2, int y2)
{
	int swapped = 0;

	if ( !gr_screen.current_color.is_alphacolor ) {
		gr_opengl2_line(x1, y1, x2, y2);
		return;
	}

	INT_CLIPLINE(x1, y1, x2, y2, gr_screen.clip_left, gr_screen.clip_top,
			gr_screen.clip_right, gr_screen.clip_bottom, return, void(), swapped=1);

	opengl2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE);

	ubyte aa = swapped ? 0 : gr_screen.current_color.alpha;
	ubyte ba = swapped ? gr_screen.current_color.alpha : 0;

	float sx1, sy1;
	float sx2, sy2;

	sx1 = i2fl(x1 + gr_screen.offset_x) + 0.5f;
	sy1 = i2fl(y1 + gr_screen.offset_y) + 0.5f;
	sx2 = i2fl(x2 + gr_screen.offset_x) + 0.5f;
	sy2 = i2fl(y2 + gr_screen.offset_y) + 0.5f;

	if (x1 == x2) {
		if (sy1 < sy2) {
			sy2 += 0.5f;
		} else {
			sy1 += 0.5f;
		}
	} else if (y1 == y2) {
		if (sx1 < sx2) {
			sx2 += 0.5f;
		} else {
			sx1 += 0.5f;
		}
	}

	opengl_alloc_render_buffer(2);

	render_buffer[0].r = gr_screen.current_color.red;
	render_buffer[0].g = gr_screen.current_color.green;
	render_buffer[0].b = gr_screen.current_color.blue;
	render_buffer[0].a = ba;
	render_buffer[0].x = sx2;
	render_buffer[0].y = sy2;
	render_buffer[0].z = -0.99f;

	render_buffer[1].r = gr_screen.current_color.red;
	render_buffer[1].g = gr_screen.current_color.green;
	render_buffer[1].b = gr_screen.current_color.blue;
	render_buffer[1].a = aa;
	render_buffer[1].x = sx1;
	render_buffer[1].y = sy1;
	render_buffer[1].z = -0.99f;

	opengl2_shader_use(PROG_COLOR);

	glVertexAttribPointer(SDRI_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rb_t), &render_buffer[0].r);
	glEnableVertexAttribArray(SDRI_COLOR);

	glVertexAttribPointer(SDRI_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
	glEnableVertexAttribArray(SDRI_POSITION);

	glDrawArrays(GL_LINES, 0, 2);

	glDisableVertexAttribArray(SDRI_COLOR);
	glDisableVertexAttribArray(SDRI_POSITION);
}

void gr_opengl2_circle(int xc, int yc, int d)
{
	int p,x, y, r;

	r = d/2;
	p=3-d;
	x=0;
	y=r;

	// Big clip
	if ( (xc+r) < gr_screen.clip_left ) return;
	if ( (xc-r) > gr_screen.clip_right ) return;
	if ( (yc+r) < gr_screen.clip_top ) return;
	if ( (yc-r) > gr_screen.clip_bottom ) return;

	while(x<y)	{
		// Draw the first octant
		gr_opengl2_line( xc-y, yc-x, xc+y, yc-x );
		gr_opengl2_line( xc-y, yc+x, xc+y, yc+x );

		if (p<0)
			p=p+(x<<2)+6;
		else	{
			// Draw the second octant
			gr_opengl2_line( xc-x, yc-y, xc+x, yc-y );
			gr_opengl2_line( xc-x, yc+y, xc+x, yc+y );

			p=p+((x-y)<<2)+10;
			y--;
		}
		x++;
	}
	if(x==y) {
		gr_opengl2_line( xc-x, yc-y, xc+x, yc-y );
		gr_opengl2_line( xc-x, yc+y, xc+x, yc+y );
	}
}

void gr_opengl2_pixel(int x, int y)
{
	gr_opengl2_line(x,y,x,y);
}

void gr_opengl2_cross_fade(int bmap1, int bmap2, int x1, int y1, int x2, int y2, float pct)
{
	gr_set_bitmap(bmap1, GR_ALPHABLEND_FILTER, GR_BITBLT_MODE_NORMAL, 1.0f - pct );
	gr_bitmap(x1, y1);

	gr_set_bitmap(bmap2, GR_ALPHABLEND_FILTER, GR_BITBLT_MODE_NORMAL, pct );
	gr_bitmap(x2, y2);
}

void gr_opengl2_flash(int r, int g, int b)
{
	CAP(r, 0, 255);
	CAP(g, 0, 255);
	CAP(b, 0, 255);

	if ( r || g || b ) {
		opengl2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_ALPHA_ADDITIVE, ZBUFFER_TYPE_NONE);

		float x1, x2, y1, y2;
		x1 = i2fl(gr_screen.clip_left+gr_screen.offset_x);
		y1 = i2fl(gr_screen.clip_top+gr_screen.offset_y);
		x2 = i2fl(gr_screen.clip_right+gr_screen.offset_x);
		y2 = i2fl(gr_screen.clip_bottom+gr_screen.offset_y);

		opengl_alloc_render_buffer(4);

		render_buffer[0].x = x1;
		render_buffer[0].y = y1;
		render_buffer[0].z = -0.99f;

		render_buffer[1].x = x1;
		render_buffer[1].y = y2;
		render_buffer[1].z = -0.99f;

		render_buffer[2].x = x2;
		render_buffer[2].y = y1;
		render_buffer[2].z = -0.99f;

		render_buffer[3].x = x2;
		render_buffer[3].y = y2;
		render_buffer[3].z = -0.99f;

		opengl2_shader_use(PROG_COLOR);

		glVertexAttrib4f(SDRI_COLOR, r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);

		glVertexAttribPointer(SDRI_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(rb_t), &render_buffer[0].x);
		glEnableVertexAttribArray(SDRI_POSITION);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableVertexAttribArray(SDRI_POSITION);
	}
}

void gr_opengl2_tmapper(int nverts, vertex **verts, uint flags)
{
	opengl2_tmapper_internal(nverts, verts, flags, 0);
}

#define FIND_SCALED_NUM(x,x0,x1,y0,y1) (((((x)-(x0))*((y1)-(y0)))/((x1)-(x0)))+(y0))

void gr_opengl2_scaler(vertex *va, vertex *vb)
{
	float x0, y0, x1, y1;
	float u0, v0, u1, v1;
	float clipped_x0, clipped_y0, clipped_x1, clipped_y1;
	float clipped_u0, clipped_v0, clipped_u1, clipped_v1;
	float xmin, xmax, ymin, ymax;
	int dx0, dy0, dx1, dy1;

	//============= CLIP IT =====================

	x0 = va->sx; y0 = va->sy;
	x1 = vb->sx; y1 = vb->sy;

	xmin = i2fl(gr_screen.clip_left); ymin = i2fl(gr_screen.clip_top);
	xmax = i2fl(gr_screen.clip_right); ymax = i2fl(gr_screen.clip_bottom);

	u0 = va->u; v0 = va->v;
	u1 = vb->u; v1 = vb->v;

	// Check for obviously offscreen bitmaps...
	if ( (y1<=y0) || (x1<=x0) ) return;
	if ( (x1<xmin ) || (x0>xmax) ) return;
	if ( (y1<ymin ) || (y0>ymax) ) return;

	clipped_u0 = u0; clipped_v0 = v0;
	clipped_u1 = u1; clipped_v1 = v1;

	clipped_x0 = x0; clipped_y0 = y0;
	clipped_x1 = x1; clipped_y1 = y1;

	// Clip the left, moving u0 right as necessary
	if ( x0 < xmin ) 	{
		clipped_u0 = FIND_SCALED_NUM(xmin,x0,x1,u0,u1);
		clipped_x0 = xmin;
	}

	// Clip the right, moving u1 left as necessary
	if ( x1 > xmax )	{
		clipped_u1 = FIND_SCALED_NUM(xmax,x0,x1,u0,u1);
		clipped_x1 = xmax;
	}

	// Clip the top, moving v0 down as necessary
	if ( y0 < ymin ) 	{
		clipped_v0 = FIND_SCALED_NUM(ymin,y0,y1,v0,v1);
		clipped_y0 = ymin;
	}

	// Clip the bottom, moving v1 up as necessary
	if ( y1 > ymax ) 	{
		clipped_v1 = FIND_SCALED_NUM(ymax,y0,y1,v0,v1);
		clipped_y1 = ymax;
	}

	dx0 = fl2i(clipped_x0); dx1 = fl2i(clipped_x1);
	dy0 = fl2i(clipped_y0); dy1 = fl2i(clipped_y1);

	if (dx1<=dx0) return;
	if (dy1<=dy0) return;

	//============= DRAW IT =====================

	vertex v[4];
	vertex *vl[4];

	vl[0] = &v[0];
	v[0].sx = clipped_x0;
	v[0].sy = clipped_y0;
	v[0].sw = va->sw;
	v[0].z = va->z;
	v[0].u = clipped_u0;
	v[0].v = clipped_v0;

	vl[1] = &v[1];
	v[1].sx = clipped_x1;
	v[1].sy = clipped_y0;
	v[1].sw = va->sw;
	v[1].z = va->z;
	v[1].u = clipped_u1;
	v[1].v = clipped_v0;

	vl[2] = &v[2];
	v[2].sx = clipped_x1;
	v[2].sy = clipped_y1;
	v[2].sw = va->sw;
	v[2].z = va->z;
	v[2].u = clipped_u1;
	v[2].v = clipped_v1;

	vl[3] = &v[3];
	v[3].sx = clipped_x0;
	v[3].sy = clipped_y1;
	v[3].sw = va->sw;
	v[3].z = va->z;
	v[3].u = clipped_u0;
	v[3].v = clipped_v1;

	opengl2_tmapper_internal( 4, vl, TMAP_FLAG_TEXTURED, 1 );
}
