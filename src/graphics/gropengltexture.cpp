/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef __EMSCRIPTEN__

#include <SDL3/SDL_opengl.h>

#include "pstypes.h"
#include "2d.h"
#include "gropengl.h"
#include "gropenglinternal.h"
#include "bmpman.h"
#include "grinternal.h"
#include "systemvars.h"
#include "osregistry.h"


static int vram_full = 0;

typedef struct tcache_slot_opengl {
	GLuint	texture_handle;
	float	u_scale, v_scale;
	int	bitmap_id;
	int	size;
	int	used_this_frame;
	int	time_created;
	ushort	w,h;

	// sections
	tcache_slot_opengl	*data_sections[MAX_BMAP_SECTIONS_X][MAX_BMAP_SECTIONS_Y];
	tcache_slot_opengl	*parent;

	gr_texture_source	texture_mode;
} tcache_slot_opengl;

static void *Texture_sections = NULL;
static tcache_slot_opengl *Textures = NULL;

static tcache_slot_opengl *GL_bound_texture;

static int GL_frame_count = 0;
static int GL_last_bitmap_id = -1;
static int GL_last_detail = -1;
static int GL_last_bitmap_type = -1;
static int GL_last_section_x = -1;
static int GL_last_section_y = -1;
static int GL_should_preload = 0;

extern int Gr_textures_in;

static gr_texture_source GL_current_texture_source = (gr_texture_source) -1;

static ubyte GL_xlat[256] = { 0 };

extern int bm_get_cache_slot( int bitmap_id, int separate_ani_frames );


void opengl_set_texture_state(gr_texture_source ts)
{
	if (ts == TEXTURE_SOURCE_NONE) {
		GL_bound_texture = NULL;

		glBindTexture(GL_TEXTURE_2D, 0);
		opengl_tcache_set(-1, -1, NULL, NULL, 0, -1, -1, 0 );
	} else if (GL_bound_texture &&
		GL_bound_texture->texture_mode != ts) {
		switch (ts) {
			case TEXTURE_SOURCE_DECAL:
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			case TEXTURE_SOURCE_NO_FILTERING:
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			default:
				break;
		}

		GL_bound_texture->texture_mode = ts;
	}

	GL_current_texture_source = ts;
}


void opengl_tcache_init()
{
	int i, idx, s_idx;

	if ( os_config_read_uint("Video", "PreloadTextures", 1) ) {
		GL_should_preload = 1;
	} else {
		GL_should_preload = 0;
	}

	Textures = (tcache_slot_opengl *)malloc(MAX_BITMAPS*sizeof(tcache_slot_opengl));
	if ( !Textures )        {
		exit(1);
	}

	if (gr_screen.use_sections) {
		Texture_sections = (tcache_slot_opengl*)malloc(MAX_BITMAPS * MAX_BMAP_SECTIONS_X * MAX_BMAP_SECTIONS_Y * sizeof(tcache_slot_opengl));
		if(!Texture_sections){
			exit(1);
		}
		memset(Texture_sections, 0, MAX_BITMAPS * MAX_BMAP_SECTIONS_X * MAX_BMAP_SECTIONS_Y * sizeof(tcache_slot_opengl));
	}

	// Init the texture structures
	int section_count = 0;
	for( i=0; i<MAX_BITMAPS; i++ )  {
		Textures[i].texture_handle = 0;

		Textures[i].bitmap_id = -1;
		Textures[i].size = 0;
		Textures[i].used_this_frame = 0;

		Textures[i].parent = NULL;

		// allocate sections
		if (gr_screen.use_sections) {
			for(idx=0; idx<MAX_BMAP_SECTIONS_X; idx++){
				for(s_idx=0; s_idx<MAX_BMAP_SECTIONS_Y; s_idx++){
					Textures[i].data_sections[idx][s_idx] = &((tcache_slot_opengl*)Texture_sections)[section_count++];
					Textures[i].data_sections[idx][s_idx]->parent = &Textures[i];
					Textures[i].data_sections[idx][s_idx]->texture_handle = 0;
					Textures[i].data_sections[idx][s_idx]->bitmap_id = -1;
					Textures[i].data_sections[idx][s_idx]->size = 0;
					Textures[i].data_sections[idx][s_idx]->used_this_frame = 0;
				}
			}
		} else {
			for(idx=0; idx<MAX_BMAP_SECTIONS_X; idx++){
				for(s_idx=0; s_idx<MAX_BMAP_SECTIONS_Y; s_idx++){
					Textures[i].data_sections[idx][s_idx] = NULL;
				}
			}
		}
	}

	GL_last_detail = Detail.hardware_textures;
	GL_last_bitmap_id = -1;
	GL_last_bitmap_type = -1;

	GL_last_section_x = -1;
	GL_last_section_y = -1;

	memset(GL_xlat, 0, sizeof(GL_xlat));
}

static int opengl_free_texture ( tcache_slot_opengl *t )
{
	int idx, s_idx;


	// Bitmap changed!!
	if ( t->bitmap_id > -1 )        {
		// if I, or any of my children have been used this frame, bail
		if(t->used_this_frame == GL_frame_count){
			return 0;
		}

		if (gr_screen.use_sections) {
			for(idx=0; idx<MAX_BMAP_SECTIONS_X; idx++){
				for(s_idx=0; s_idx<MAX_BMAP_SECTIONS_Y; s_idx++){
					if((t->data_sections[idx][s_idx] != NULL) && (t->data_sections[idx][s_idx]->used_this_frame == GL_frame_count)){
						return 0;
					}
				}
			}
		}

		// ok, now we know its legal to free everything safely
		t->texture_mode = (gr_texture_source) -1;
		glDeleteTextures (1, &t->texture_handle);
		t->texture_handle = 0;

		if ( GL_last_bitmap_id == t->bitmap_id )       {
			GL_last_bitmap_id = -1;
		}

		// if this guy has children, free them too, since the children
		// actually make up his size
		if (gr_screen.use_sections) {
			for(idx=0; idx<MAX_BMAP_SECTIONS_X; idx++){
				for(s_idx=0; s_idx<MAX_BMAP_SECTIONS_Y; s_idx++){
					if(t->data_sections[idx][s_idx] != NULL){
						opengl_free_texture(t->data_sections[idx][s_idx]);
					}
				}
			}
		}

		t->bitmap_id = -1;
		t->used_this_frame = 0;
		Gr_textures_in -= t->size;
		t->size = 0;
	}

	return 1;
}

void opengl_tcache_flush()
{
	int i;

	if (Textures == NULL) {
		return;
	}

	for( i=0; i<MAX_BITMAPS; i++ )  {
		opengl_free_texture ( &Textures[i] );
	}
	if (Gr_textures_in != 0) {
		mprintf(( "WARNING: VRAM is at %d instead of zero after flushing!\n", Gr_textures_in ));
		Gr_textures_in = 0;
	}

	GL_last_bitmap_id = -1;
	GL_last_section_x = -1;
	GL_last_section_y = -1;
}

void opengl_tcache_cleanup()
{
	opengl_tcache_flush ();

	if ( Textures ) {
		free(Textures);
		Textures = NULL;
	}

	if( Texture_sections != NULL ){
		free(Texture_sections);
		Texture_sections = NULL;
	}
}

void opengl_tcache_frame()
{
	GL_last_bitmap_id = -1;

	GL_frame_count++;

	/*
	int idx, s_idx;
	int i;
	for( i=0; i<MAX_BITMAPS; i++ )  {
		Textures[i].used_this_frame = 0;

		// data sections
		if(Textures[i].data_sections[0][0] != NULL){
			SDL_assert(GL_texture_sections);
			if(GL_texture_sections){
				for(idx=0; idx<MAX_BMAP_SECTIONS_X; idx++){
					for(s_idx=0; s_idx<MAX_BMAP_SECTIONS_Y; s_idx++){
						if(Textures[i].data_sections[idx][s_idx] != NULL){
							Textures[i].data_sections[idx][s_idx]->used_this_frame = 0;
						}
					}
				}
			}
		}
	}
	*/

	if ( vram_full )        {
		opengl_tcache_flush();
		vram_full = 0;
	}
}

static void opengl_tcache_get_adjusted_texture_size(int w_in, int h_in, int *w_out, int *h_out)
{
	int tex_w, tex_h;
	int i;

	// bogus
	if((w_out == NULL) ||  (h_out == NULL)){
		return;
	}

	// starting size
	tex_w = w_in;
	tex_h = h_in;

	// set height and width to a power of 2
	for (i=0; i<16; i++ )	{
		if ( (tex_w > (1<<i)) && (tex_w <= (1<<(i+1))) )	{
			tex_w = 1 << (i+1);
			break;
		}
	}

	for (i=0; i<16; i++ )	{
		if ( (tex_h > (1<<i)) && (tex_h <= (1<<(i+1))) )	{
			tex_h = 1 << (i+1);
			break;
		}
	}

	// try to keep an 8:1 size ratio
	if (tex_w/tex_h > 8)
		tex_h = tex_w/8;
	if (tex_h/tex_w > 8)
		tex_w = tex_h/8;

	if ( tex_w < GL_min_texture_width ) {
		tex_w = GL_min_texture_width;
	} else if ( tex_w > GL_max_texture_width )     {
		tex_w = GL_max_texture_width;
	}

	if ( tex_h < GL_min_texture_height ) {
		tex_h = GL_min_texture_height;
	} else if ( tex_h > GL_max_texture_height )    {
		tex_h = GL_max_texture_height;
	}

	// store the outgoing size
	*w_out = tex_w;
	*h_out = tex_h;
}

// bmp == bitmap structure with w, h, and data
// sx == x offset into bitmap
// sy == y offset into bitmap
// src_w == absolute width of section on source bitmap
// src_h == absolute height of section on source bitmap
// bmap_w == width of source bitmap
// bmap_h == height of source bitmap
// tex_w == width of final texture
// tex_h == height of final texture
static int opengl_create_texture_sub(int bitmap_handle, int bitmap_type, bitmap *bmp, tcache_slot_opengl *t, int sx, int sy, int src_w, int src_h, int tex_w, int tex_h, bool reload, bool resize, int fail_on_full)
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
		if ( !opengl_free_texture(t) ) {
			return 0;
		}

		glGenTextures(1, &t->texture_handle);

		if ( !t->texture_handle ) {
			nprintf(("Error", "!!DEBUG!! t->texture_handle == 0"));
			return 0;
		}
	}

	switch (bitmap_type) {
		case TCACHE_TYPE_AABITMAP:
			t->u_scale = (float)bmp->w / (float)tex_w;
			t->v_scale = (float)bmp->h / (float)tex_h;
			break;

		case TCACHE_TYPE_BITMAP_INTERFACE:
		case TCACHE_TYPE_BITMAP_SECTION:
			t->u_scale = (float)src_w / (float)tex_w;
			t->v_scale = (float)src_h / (float)tex_h;
			break;

		default:
			t->u_scale = 1.0f;
			t->v_scale = 1.0f;
			break;
	}

	t->texture_mode = TEXTURE_SOURCE_NO_FILTERING;

	glBindTexture(GL_TEXTURE_2D, t->texture_handle);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	ubyte *bmp_data = (ubyte*)bmp->data;
	ubyte *texmem = NULL, *texmemp;
	int i, j;
	int size = 0;

	switch (bitmap_type) {
		case TCACHE_TYPE_AABITMAP: {
			texmem = (ubyte *) malloc(tex_w * tex_h);
			texmemp = texmem;

			for (i = 0; i < tex_h; i++) {
				for (j = 0;j < tex_w; j++) {
					if ( (i < bmp->h) && (j < bmp->w) ) {
						*texmemp++ = GL_xlat[bmp_data[i*bmp->w+j]];
					} else {
						*texmemp++ = 0;
					}
				}
			}

			size = tex_w * tex_h;

			if (reload) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_ALPHA, GL_UNSIGNED_BYTE, texmem);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tex_w, tex_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texmem);
			}

			free (texmem);

			break;
		}

		case TCACHE_TYPE_BITMAP_INTERFACE:
		case TCACHE_TYPE_BITMAP_SECTION: {
			// if we aren't resizing in any way then we can just use bmp_data directly
			if (resize) {
				texmem = (ubyte *) malloc(tex_w * tex_h * 2);
				texmemp = texmem;

				for (i = 0;i < tex_h; i++) {
					for (j = 0; j < tex_w; j++) {
						if ( (i < src_h) && (j < src_w) ) {
							*texmemp++ = bmp_data[((i+sy)*bmp->w+(j+sx))*2+0];
							*texmemp++ = bmp_data[((i+sy)*bmp->w+(j+sx))*2+1];
						} else {
							*texmemp++ = 0;
							*texmemp++ = 0;
						}
					}
				}
			}

			size = tex_w * tex_h * 2;

			if (reload) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (resize) ? texmem : bmp_data);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (resize) ? texmem : bmp_data);
			}

			if (texmem) {
				free(texmem);
			}

			break;
		}

		default: {
			// if we aren't resizing then we can just use bmp_data directly
			if (resize) {
				texmem = (ubyte *) malloc (tex_w * tex_h * 2);
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

			size = tex_w * tex_h * 2;

			if (reload) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (resize) ? texmem : bmp_data);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (resize) ? texmem : bmp_data);
			}

			if (texmem) {
				free(texmem);
			}

			break;
		}
	}

	t->bitmap_id = bitmap_handle;
	t->time_created = GL_frame_count;
	t->used_this_frame = 0;
	t->size = size;
	t->w = (ushort)tex_w;
	t->h = (ushort)tex_h;

	if (!reload) {
		Gr_textures_in += t->size;
	}

	return 1;
}

static int opengl_create_texture(int bitmap_handle, int bitmap_type, tcache_slot_opengl *tslot, int fail_on_full)
{
	ubyte flags = 0;
	int final_w, final_h;
	ubyte bpp = 16;
	bool resize = false;
	bool cull_size = false;

	// setup texture/bitmap flags
	switch(bitmap_type){
		case TCACHE_TYPE_AABITMAP:
			flags |= BMP_AABITMAP;
			bpp = 8;
			break;
		case TCACHE_TYPE_NORMAL:
			flags |= BMP_TEX_OTHER;
			cull_size = true;
			break;
		case TCACHE_TYPE_NONDARKENING:
			flags |= BMP_TEX_NONDARK;
			cull_size = true;
			break;
		case TCACHE_TYPE_BITMAP_INTERFACE:
		case TCACHE_TYPE_XPARENT:
			flags |= BMP_TEX_XPARENT;
			break;
		default:
			Int3();
			return 0;
	}

	// lock the bitmap into the proper format
	bitmap *bmp = bm_lock(bitmap_handle, bpp, flags);
	if ( bmp == NULL ) {
		mprintf(("Couldn't lock bitmap %d.\n", bitmap_handle ));
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

	// get final texture size as it will be allocated as a DD surface
	opengl_tcache_get_adjusted_texture_size(max_w, max_h, &final_w, &final_h);

	if ( (final_w < 1) || (final_h < 1) ) {
		mprintf(("Bitmap is too small at %dx%d.\n", final_w, final_h));
		return 0;
	}

	// if we don't have to resize the image (to get power of 2, etc.) then skip that extra work
	if ( (max_w != final_w) || (max_h != final_h) ) {
		resize = true;
	}

	bool reload = false;

	// see if we can reuse this slot for a new bitmap
	if ( tslot->texture_handle && (tslot->bitmap_id != bitmap_handle) ) {
		if ( (final_w == tslot->w) && (final_h == tslot->h) ) {
			reload = true;
		}
	}

	// call the helper
	int ret_val = opengl_create_texture_sub(bitmap_handle, bitmap_type, bmp, tslot, 0, 0, bmp->w, bmp->h, final_w, final_h, reload, resize, fail_on_full);

	// unlock the bitmap
	bm_unlock(bitmap_handle);

	return ret_val;
}

static int opengl_create_texture_sectioned(int bitmap_handle, int bitmap_type, tcache_slot_opengl *tslot, int sx, int sy, int fail_on_full)
{
	int final_w, final_h;
	int section_x, section_y;
	bool resize = true;

	SDL_assert( gr_screen.use_sections );

	// setup texture/bitmap flags
	SDL_assert(bitmap_type == TCACHE_TYPE_BITMAP_SECTION);
	if(bitmap_type != TCACHE_TYPE_BITMAP_SECTION){
		bitmap_type = TCACHE_TYPE_BITMAP_SECTION;
	}

	// lock the bitmap in the proper format
	bitmap *bmp = bm_lock(bitmap_handle, 16, BMP_TEX_XPARENT);
	if ( bmp == NULL ) {
		mprintf(("Couldn't lock bitmap %d.\n", bitmap_handle ));
		return 0;
	}
	// determine the width and height of this section
	bm_get_section_size(bitmap_handle, sx, sy, &section_x, &section_y);

	// get final texture size as it will be allocated as an opengl texture
	opengl_tcache_get_adjusted_texture_size(section_x, section_y, &final_w, &final_h);

	if ( (final_w < 1) || (final_h < 1) ) {
		mprintf(("Bitmap is too small at %dx%d.\n", final_w, final_h));
		return 0;
	}

	// if we don't have to resize the image (to get power of 2, etc.) then skip that extra work
	if ( (bmp->sections.num_x == 1) && (bmp->sections.num_y == 1) && (section_x == final_w) && (section_y == final_h) ) {
		resize = false;
	}

	bool reload = false;

	// see if we can reuse this slot for a new bitmap
	if ( tslot->texture_handle && (tslot->bitmap_id != bitmap_handle) ) {
		if ( (final_w == tslot->w) && (final_h == tslot->h) ) {
			reload = true;
		}
	}

	// call the helper
	int ret_val = opengl_create_texture_sub(bitmap_handle, bitmap_type, bmp, tslot, bmp->sections.sx[sx], bmp->sections.sy[sy], section_x, section_y, final_w, final_h, reload, resize, fail_on_full);

	// unlock the bitmap
	bm_unlock(bitmap_handle);

	return ret_val;
}

int opengl_tcache_set(int bitmap_id, int bitmap_type, float *u_scale, float *v_scale, int fail_on_full, int sx, int sy, int force)
{
	bitmap *bmp = NULL;

	int idx, s_idx;
	int ret_val = 1;

	if (bitmap_id < 0)
	{
		GL_last_bitmap_id = -1;
		return 0;
	}

	if ( GL_last_detail != Detail.hardware_textures )      {
		GL_last_detail = Detail.hardware_textures;
		opengl_tcache_flush();
	}

	if (vram_full) {
		return 0;
	}

	int n = bm_get_cache_slot (bitmap_id, 1);
	tcache_slot_opengl *t = &Textures[n];

	if ( (GL_last_bitmap_id == bitmap_id) && (GL_last_bitmap_type==bitmap_type) && (t->bitmap_id == bitmap_id) && (GL_last_section_x == sx) && (GL_last_section_y == sy))       {
		t->used_this_frame = GL_frame_count;

		// mark all children as used
		if (gr_screen.use_sections) {
			for(idx=0; idx<MAX_BMAP_SECTIONS_X; idx++){
				for(s_idx=0; s_idx<MAX_BMAP_SECTIONS_Y; s_idx++){
					if(t->data_sections[idx][s_idx] != NULL){
						t->data_sections[idx][s_idx]->used_this_frame = GL_frame_count;
					}
				}
			}
		}

		// for second stage addition of nondark pixels (assumed to be switched to GL_TEXTURE1)
		if (force && (bitmap_type == TCACHE_TYPE_NONDARKENING)) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, t->texture_handle);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		}

		*u_scale = t->u_scale;
		*v_scale = t->v_scale;
		return 1;
	}

	if (bitmap_type == TCACHE_TYPE_BITMAP_SECTION){
		SDL_assert( gr_screen.use_sections );
		SDL_assert((sx >= 0) && (sy >= 0) && (sx < MAX_BMAP_SECTIONS_X) && (sy < MAX_BMAP_SECTIONS_Y));
		if(!((sx >= 0) && (sy >= 0) && (sx < MAX_BMAP_SECTIONS_X) && (sy < MAX_BMAP_SECTIONS_Y))){
			return 0;
		}

		ret_val = 1;

		// if the texture sections haven't been created yet
		if((t->bitmap_id < 0) || (t->bitmap_id != bitmap_id)){

			// lock the bitmap in the proper format
			bmp = bm_lock(bitmap_id, 16, BMP_TEX_XPARENT);
			bm_unlock(bitmap_id);

			// now lets do something for each texture

			for(idx=0; idx<bmp->sections.num_x; idx++){
				for(s_idx=0; s_idx<bmp->sections.num_y; s_idx++){
					// hmm. i'd rather we didn't have to do it this way...
					if(!opengl_create_texture_sectioned(bitmap_id, bitmap_type, t->data_sections[idx][s_idx], idx, s_idx, fail_on_full)){
						ret_val = 0;
					}

					// not used this frame
					t->data_sections[idx][s_idx]->used_this_frame = 0;
				}
			}

			// zero out pretty much everything in the parent struct since he's just the root
			t->bitmap_id = bitmap_id;
			t->texture_handle = 0;
			t->time_created = t->data_sections[sx][sy]->time_created;
			t->used_this_frame = 0;
		}

		// argh. we failed to upload. free anything we can
		if(!ret_val){
			opengl_free_texture(t);
		}
		// swap in the texture we want
		else {
			t = t->data_sections[sx][sy];
		}
	}
	// all other "normal" textures
	else if((bitmap_id < 0) || (bitmap_id != t->bitmap_id)){
		ret_val = opengl_create_texture( bitmap_id, bitmap_type, t, fail_on_full );
	}

	// everything went ok
	if(ret_val && (t->texture_handle) && !vram_full){
		*u_scale = t->u_scale;
		*v_scale = t->v_scale;

		GL_bound_texture = t;

		glBindTexture (GL_TEXTURE_2D, t->texture_handle );

		GL_last_bitmap_id = t->bitmap_id;
		GL_last_bitmap_type = bitmap_type;
		GL_last_section_x = sx;
		GL_last_section_y = sy;

		t->used_this_frame = GL_frame_count;
	}
	// gah
	else {
		GL_last_bitmap_id = -1;
		GL_last_bitmap_type = -1;

		GL_last_section_x = -1;
		GL_last_section_y = -1;

		GL_bound_texture = NULL;

		glBindTexture (GL_TEXTURE_2D, 0);	// test - DDOI
		return 0;
	}

	return 1;
}

void gr_opengl_preload_init()
{
	opengl_tcache_flush();
}

int gr_opengl_preload(int bitmap_num, int is_aabitmap)
{
	if ( !GL_should_preload )      {
		return 0;
	}

	float u_scale, v_scale;
	int retval;
	int bitmap_type = TCACHE_TYPE_NORMAL;

	if ( is_aabitmap )      {
		bitmap_type = TCACHE_TYPE_AABITMAP;
	}

	retval = opengl_tcache_set(bitmap_num, bitmap_type, &u_scale, &v_scale, 1, -1, -1, 0 );

	if ( !retval )  {
		mprintf(("Texture upload failed!\n" ));
	}

	return retval;
}

void gr_opengl_set_gamma(float)
{
	int i;

	// set the alpha gamma settings (for fonts)
	for (i = 0; i < 16; i++) {
		GL_xlat[i] = (ubyte)Gr_gamma_lookup[(i*255)/15];
	}

	GL_xlat[15] = GL_xlat[1];

	// Flush any existing textures
	opengl_tcache_flush();
}

void gr_opengl_release_texture(int handle)
{
	for(int i=0; i<MAX_BITMAPS; i++ )  {
		if (Textures[i].bitmap_id == handle) {
			Textures[i].used_this_frame = 0; // this bmp doesn't even exist any longer...
			opengl_free_texture( &Textures[i] );
		}
	}
}

#endif	// !__EMSCRIPTEN__
