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
#include "grgles2.h"
#include "grgles2internal.h"
#include "bmpman.h"
#include "grinternal.h"
#include "systemvars.h"
#include "osregistry.h"


static bool vram_full = false;

struct tcache_slot_opengl2 {
	GLuint texture_handle;
	int bitmap_id;
	int size;
	int used_this_frame;
	int time_created;
	int is_mipmaped;
	ushort w;
	ushort h;

	gr_texture_source texture_mode;
};

static tcache_slot_opengl2 *Textures = NULL;

static tcache_slot_opengl2 *GL_bound_texture;

static int GL_frame_count = 0;
static int GL_last_bitmap_id = -1;
static int GL_last_detail = -1;
static int GL_last_bitmap_type = -1;
static int GL_should_preload = 0;

static ubyte GL_xlat[256] = { 0 };

extern int Gr_textures_in;
extern int bm_get_cache_slot( int bitmap_id, int separate_ani_frames );


void gles2_set_texture_state(gr_texture_source ts)
{
	if (ts == TEXTURE_SOURCE_NONE) {
		GL_bound_texture = NULL;

		glBindTexture(GL_TEXTURE_2D, 0);
		gles2_tcache_set(-1, -1);
	} else if (GL_bound_texture && GL_bound_texture->texture_mode != ts) {
		switch (ts) {
			case TEXTURE_SOURCE_DECAL:
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;

			case TEXTURE_SOURCE_NO_FILTERING: {
				if (GL_bound_texture->is_mipmaped) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
				} else {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				break;
			}

			default:
				break;
		}

		GL_bound_texture->texture_mode = ts;
	}
}

void gles2_tcache_init()
{
	GL_should_preload = os_config_read_uint("Video", "PreloadTextures", 1);

	Textures = (tcache_slot_opengl2 *) malloc(MAX_BITMAPS * sizeof(tcache_slot_opengl2));

	if (Textures == NULL) {
		exit(1);
	}

	SDL_assert(gr_screen.use_sections == 0);

	memset(Textures, 0, MAX_BITMAPS * sizeof(tcache_slot_opengl2));

	for (int i = 0; i < MAX_BITMAPS; i++) {
		Textures[i].bitmap_id = -1;
	}

	GL_last_detail = Detail.hardware_textures;
	GL_last_bitmap_id = -1;
	GL_last_bitmap_type	= -1;

	SDL_zero(GL_xlat);
}

static int gles2_free_texture(tcache_slot_opengl2 *t)
{
	if (t->bitmap_id < 0) {
		return 1;
	}

	// if I have been used this frame, bail
	if (t->used_this_frame == GL_frame_count) {
		return 0;
	}

	glDeleteTextures(1, &t->texture_handle);

	if (GL_last_bitmap_id == t->bitmap_id) {
		GL_last_bitmap_id = -1;
	}

	Gr_textures_in -= t->size;

	t->texture_handle = 0;
	t->bitmap_id = -1;
	t->used_this_frame = 0;
	t->size = 0;
	t->is_mipmaped = 0;

	return 1;
}

void gles2_tcache_flush()
{
	if (Textures == NULL) {
		return;
	}

	for (int i = 0; i < MAX_BITMAPS; i++) {
		gles2_free_texture(&Textures[i]);
	}

	if (Gr_textures_in != 0) {
		mprintf(("WARNING: VRAM is at %d instead of zero after flushing!\n", Gr_textures_in));
		Gr_textures_in = 0;
	}

	GL_last_bitmap_id = -1;

	vram_full = false;
}

void gles2_tcache_cleanup()
{
	gles2_tcache_flush();

	if (Textures) {
		free(Textures);
		Textures = NULL;
	}
}

void gles2_tcache_frame()
{
	GL_last_bitmap_id = -1;
	GL_frame_count++;

	if (vram_full) {
		gles2_tcache_flush();
		vram_full = false;
	}
}

static int gles2_create_texture_sub(int bitmap_handle, int bitmap_type, bitmap *bmp, tcache_slot_opengl2 *t, int tex_w, int tex_h, bool reload, bool resize, int fail_on_full)
{
	// bogus
	if ( (bmp == NULL) || (t == NULL) ) {
		return 0;
	}

	if (t->used_this_frame == GL_frame_count) {
		mprintf(("ARGHH!!! Texture already used this frame! Cannot free it!\n"));
		return 0;
	}

	if ( !reload ) {
		if ( !gles2_free_texture(t) ) {
			return 0;
		}

		glGenTextures(1, &t->texture_handle);

		if ( !t->texture_handle ) {
			nprintf(("Error", "!!DEBUG!! t->texture_handle == 0"));
			return 0;
		}
	}

	t->texture_mode = TEXTURE_SOURCE_NO_FILTERING;

	glBindTexture(GL_TEXTURE_2D, t->texture_handle);

	GLint wrap_mode = GL_CLAMP_TO_EDGE;

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	ubyte *bmp_data = (ubyte*)bmp->data;
	ubyte *texmem = NULL, *texmemp;
	int i, j;
	int size = 0;

	switch (bitmap_type) {
		case TCACHE_TYPE_AABITMAP: {
			SDL_assert(tex_w == bmp->w);
			SDL_assert(tex_h == bmp->h);

			texmem = (ubyte *) malloc(tex_w * tex_h);
			texmemp = texmem;

			for (i = 0; i < tex_h; i++) {
				for (j = 0; j < tex_w; j++) {
					*texmemp++ = GL_xlat[bmp_data[i*bmp->w+j]];
				}
			}

			size = tex_w * tex_h;

			if (reload) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_ALPHA, GL_UNSIGNED_BYTE, texmem);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tex_w, tex_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texmem);
			}

			free(texmem);

			break;
		}

		case TCACHE_TYPE_BITMAP_INTERFACE:
		case TCACHE_TYPE_BITMAP_SECTION: {
			size = tex_w * tex_h * 2;

			if (reload) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, bmp_data);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, bmp_data);
			}

			break;
		}

		default: {
			wrap_mode = GL_REPEAT;

			if (resize) {
				texmem = (ubyte *) malloc(tex_w * tex_h * 2);
				texmemp = texmem;

				SDL_assert(texmem);

				fix u = 0, utmp, v = 0, du, dv;

				du = ((bmp->w - 1) * F1_0) / tex_w;
				dv = ((bmp->h - 1) * F1_0) / tex_h;

				for (j = 0; j < tex_h; j++) {
					utmp = u;

					for (i = 0; i < tex_w; i++) {
						*texmemp++ = bmp_data[(f2i(v)*bmp->w+f2i(utmp))*2+0];
						*texmemp++ = bmp_data[(f2i(v)*bmp->w+f2i(utmp))*2+1];

						utmp += du;
					}

					v += dv;
				}
			}

			if (reload) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (resize) ? texmem : bmp_data);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (resize) ? texmem : bmp_data);
			}

			if (texmem) {
				free(texmem);
			}

			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
			t->is_mipmaped = 1;

			size = fl2i(tex_w * tex_h * 2.0f * 1.3333333f);

			break;
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

	t->bitmap_id = bitmap_handle;
	t->time_created = GL_frame_count;
	t->used_this_frame = 0;
	t->size = size;
	t->w = (ushort)tex_w;
	t->h = (ushort)tex_h;

	if ( !reload ) {
		Gr_textures_in += t->size;
	}

	return 1;
}

static int gles2_create_texture(int bitmap_handle, int bitmap_type, tcache_slot_opengl2 *tslot, int fail_on_full)
{
	ubyte flags = 0;
	bool cull_size = false;
	bool resize = false;
	ubyte bpp = 16;

	switch (bitmap_type) {
		case TCACHE_TYPE_AABITMAP: {
			flags |= BMP_AABITMAP;
			bpp = 8;

			break;
		}

		case TCACHE_TYPE_NORMAL: {
			flags |= BMP_TEX_OTHER;
			cull_size = true;

			break;
		}

		case TCACHE_TYPE_BITMAP_INTERFACE:
		case TCACHE_TYPE_XPARENT: {
			flags |= BMP_TEX_XPARENT;

			break;
		}

		default:
			Int3();
			return 0;
	}

	bitmap *bmp = bm_lock(bitmap_handle, bpp, flags);

	if (bmp == NULL) {
		mprintf(("Couldn't lock bitmap %d\n", bitmap_handle));
		return 0;
	}

	int max_w = bmp->w;
	int max_h = bmp->h;

	if ( cull_size && (Detail.hardware_textures < 4) ) {
		// if we are going to cull the size then we need to force a resize
		resize = true;

		// Detail.hardware_textures goes form 0 to 4
		int val = 16 >> Detail.hardware_textures;

		max_w /= val;
		max_h /= val;
	}

	if ( (bitmap_type == TCACHE_TYPE_NORMAL) || (bitmap_type == TCACHE_TYPE_XPARENT) ) {
		if ( !is_pow2(max_w) || !is_pow2(max_h) ) {
			resize = true;

			max_w = next_pow2(max_w);
			max_h = next_pow2(max_h);
		}
	}

	if ( (max_w < 1) || (max_h < 1) ) {
		mprintf(("Bitmap %d is too small at %dx%d\n", bitmap_handle, max_w, max_h));
		return 0;
	}

	bool reload = false;

	// see if we can reuse this slot for a new bitmap
	if ( tslot->texture_handle && (tslot->bitmap_id != bitmap_handle) ) {
		if ( (max_w == tslot->w) && (max_h == tslot->h) ) {
			reload = true;
		}
	}

	// call the helper
	int ret_val = gles2_create_texture_sub(bitmap_handle, bitmap_type, bmp, tslot, max_w, max_h, reload, resize, fail_on_full);

	// unlock the bitmap
	bm_unlock(bitmap_handle);

	return ret_val;
}

int gles2_tcache_set(int bitmap_id, int bitmap_type, int fail_on_full)
{
	if (bitmap_id < 0) {
		GL_last_bitmap_id = -1;

		return 0;
	}

	if (GL_last_detail != Detail.hardware_textures) {
		gles2_tcache_flush();
		GL_last_detail = Detail.hardware_textures;
	}

	if (vram_full) {
		return 0;
	}

	int n = bm_get_cache_slot(bitmap_id, 1);
	tcache_slot_opengl2 *t = &Textures[n];

	if ( (GL_last_bitmap_id == bitmap_id) && (GL_last_bitmap_type == bitmap_type) && (t->bitmap_id == bitmap_id) ) {
		t->used_this_frame = GL_frame_count;

		return 1;
	}

	int ret_val = 1;

	if (bitmap_id != t->bitmap_id) {
		ret_val = gles2_create_texture(bitmap_id, bitmap_type, t, fail_on_full);
	}

	if (ret_val && t->texture_handle && !vram_full) {
		glBindTexture(GL_TEXTURE_2D, t->texture_handle);

		GL_bound_texture = t;

		GL_last_bitmap_id = t->bitmap_id;
		GL_last_bitmap_type = bitmap_type;

		t->used_this_frame = GL_frame_count;
	} else {
		GL_last_bitmap_id = -1;
		GL_last_bitmap_type = -1;

		GL_bound_texture = NULL;

		glBindTexture(GL_TEXTURE_2D, 0);

		return 0;
	}

	return 1;
}

void gr_gles2_preload_init()
{
	gles2_tcache_flush();
}

int gr_gles2_preload(int bitmap_num, int is_aabitmap)
{
	if ( !GL_should_preload ) {
		return 0;
	}

	int bitmap_type = (is_aabitmap) ? TCACHE_TYPE_AABITMAP : TCACHE_TYPE_NORMAL;

	int retval = gles2_tcache_set(bitmap_num, bitmap_type, 1);

	if ( !retval ) {
		mprintf(("Texture upload failed bit bitmap %d!\n", bitmap_num));
	}

	return retval;
}

void gr_gles2_set_gamma(float)
{
	// set the alpha gamma settings (for fonts)
	for (int i = 0; i < 16; i++) {
		GL_xlat[i] = (ubyte)Gr_gamma_lookup[(i*255)/15];
	}

	GL_xlat[15] = GL_xlat[1];

	// Flush any existing textures
	gles2_tcache_flush();
}

void gr_gles2_release_texture(int handle)
{
	for (int i = 0; i < MAX_BITMAPS; i++) {
		auto t = &Textures[i];

		if (t->bitmap_id == handle) {
			t->used_this_frame = 0;
			gles2_free_texture(t);

			break;
		}
	}
}
