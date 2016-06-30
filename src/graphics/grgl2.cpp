/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "SDL_opengles2.h"

#include "gropengl.h"
#include "gropenglinternal.h"
#include "grgl2.h"
#include "2d.h"
#include "mouse.h"
#include "pstypes.h"
#include "bmpman.h"
#include "grinternal.h"
#include "osregistry.h"


int GL_two_inited = 0;

bool Use_mipmaps = false;

static GLuint FB_texture = 0;
static GLuint FB_id = 0;
static GLuint FB_rb_id = 0;

static GLuint GL_saved_screen_tex = 0;
static GLuint GL_stream_tex = 0;


static gr_alpha_blend GL_current_alpha_blend = (gr_alpha_blend) -1;
static gr_zbuffer_type GL_current_zbuffer_type = (gr_zbuffer_type) -1;

void opengl2_set_state(gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt)
{
	opengl2_set_texture_state(ts);

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
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
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

static void opengl2_init_func_pointers()
{
	gr_screen.gf_flip = gr_opengl2_flip;
	gr_screen.gf_set_clip = gr_opengl2_set_clip;
	gr_screen.gf_reset_clip = gr_opengl_reset_clip;

	gr_screen.gf_clear = gr_opengl_clear;

	gr_screen.gf_aabitmap = gr_opengl2_aabitmap;
	gr_screen.gf_aabitmap_ex = gr_opengl2_aabitmap_ex;

	gr_screen.gf_rect = gr_opengl2_rect;
	gr_screen.gf_shade = gr_opengl2_shade;
	gr_screen.gf_string = gr_opengl2_string;
	gr_screen.gf_circle = gr_opengl2_circle;

	gr_screen.gf_line = gr_opengl2_line;
	gr_screen.gf_aaline = gr_opengl2_aaline;
	gr_screen.gf_pixel = gr_opengl2_pixel;
	gr_screen.gf_scaler = gr_opengl2_scaler;
	gr_screen.gf_tmapper = gr_opengl2_tmapper;

	gr_screen.gf_gradient = gr_opengl2_gradient;

	gr_screen.gf_print_screen = gr_opengl_print_screen;

	gr_screen.gf_fade_in = gr_opengl2_fade_in;
	gr_screen.gf_fade_out = gr_opengl2_fade_out;
	gr_screen.gf_flash = gr_opengl2_flash;

	gr_screen.gf_zbuffer_clear = gr_opengl2_zbuffer_clear;

	gr_screen.gf_save_screen = gr_opengl2_save_screen;
	gr_screen.gf_restore_screen = gr_opengl2_restore_screen;
	gr_screen.gf_free_screen = gr_opengl2_free_screen;

	gr_screen.gf_dump_frame_start = gr_opengl2_dump_frame_start;
	gr_screen.gf_dump_frame_stop = gr_opengl2_dump_frame_stop;
	gr_screen.gf_dump_frame = gr_opengl2_dump_frame;

	gr_screen.gf_stream_start = gr_opengl2_stream_start;
	gr_screen.gf_stream_frame = gr_opengl2_stream_frame;
	gr_screen.gf_stream_stop = gr_opengl2_stream_stop;

	gr_screen.gf_set_gamma = gr_opengl2_set_gamma;

	gr_screen.gf_lock = gr_opengl_lock;
	gr_screen.gf_unlock = gr_opengl_unlock;

	gr_screen.gf_fog_set = gr_opengl2_fog_set;

	gr_screen.gf_get_region = gr_opengl2_get_region;

	gr_screen.gf_set_cull = gr_opengl_set_cull;

	gr_screen.gf_cross_fade = gr_opengl2_cross_fade;

	gr_screen.gf_preload_init = gr_opengl2_preload_init;
	gr_screen.gf_preload = gr_opengl2_preload;

	gr_screen.gf_zbias = gr_opengl_zbias;

	gr_screen.gf_set_viewport = gr_opengl2_set_viewport;

	gr_screen.gf_activate = gr_opengl_activate;

	gr_screen.gf_release_texture = gr_opengl2_release_texture;
}

static int opengl2_create_framebuffer()
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
	glGenRenderbuffers(1, &FB_rb_id);
	glBindRenderbuffer(GL_RENDERBUFFER, FB_rb_id);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, gr_screen.max_w, gr_screen.max_h);

	// create framebuffer
	glGenFramebuffers(1, &FB_id);
	glBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// attach texture and renderbuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FB_texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FB_rb_id);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		if (FB_texture) {
			glDeleteTextures(1, &FB_texture);
			FB_texture = 0;
		}

		if (FB_rb_id) {
			glDeleteRenderbuffers(1, &FB_rb_id);
			FB_rb_id = 0;
		}

		if (FB_id) {
			glDeleteFramebuffers(1, &FB_id);
			FB_id = 0;
		}

		return 0;
	}

	return 1;
}

void opengl2_cleanup()
{
	if ( !GL_two_inited ) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (FB_texture) {
		glDeleteTextures(1, &FB_texture);
		FB_texture = 0;
	}

	if (FB_rb_id) {
		glDeleteRenderbuffers(1, &FB_rb_id);
		FB_rb_id = 0;
	}

	if (FB_id) {
		glDeleteFramebuffers(1, &FB_id);
		FB_id = 0;
	}

	opengl2_tcache_cleanup();
	opengl2_shader_cleanup();

	if (GL_context) {
		SDL_GL_DeleteContext(GL_context);
		GL_context = NULL;
	}

	GL_two_inited = 0;
}

int opengl2_init()
{
	if (GL_two_inited) {
		return 1;
	}

	GL_two_inited = 1;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	GL_context = SDL_GL_CreateContext(GL_window);

	if ( !GL_context ) {
		opengl2_cleanup();
		return 0;
	}

	mprintf(("  Vendor   : %s\n", glGetString(GL_VENDOR)));
	mprintf(("  Renderer : %s\n", glGetString(GL_RENDERER)));
	mprintf(("  Version  : %s\n", glGetString(GL_VERSION)));

	// initial viewport setup
	gr_opengl2_set_viewport(gr_screen.max_w, gr_screen.max_h);

	// set up generic variables
	opengl_set_variables();

	opengl2_init_func_pointers();
	opengl2_tcache_init();

	if ( !opengl2_shader_init() ) {
		opengl2_cleanup();
		return 0;
	}

	if ( !opengl2_create_framebuffer() ) {
		opengl2_cleanup();
		return 0;
	}

	glEnable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	glDepthRangef(0.0f, 1.0f);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if ( SDL_GL_ExtensionSupported("GL_OES_texture_npot") ) {
		Use_mipmaps	= true;
	}

	mprintf(("  Mipmaps  : %s\n", Use_mipmaps ? "Enabled" : "Disabled"));

	glFlush();

	gr_opengl_clear();
	gr_opengl_set_cull(1);

	return 1;
}

void gr_opengl2_flip()
{
	if ( !GL_two_inited ) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gr_opengl_reset_clip();

	// set viewport to window size
	glViewport(GL_viewport_x, GL_viewport_y, GL_viewport_w, GL_viewport_h);

	glClear(GL_COLOR_BUFFER_BIT);

	{
		float x = 0.0f;
		float y = 0.0f;
		float w = i2fl(GL_viewport_w);
		float h = i2fl(GL_viewport_h);

		const float tex_coord[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };
		const float ver_coord[] = { x, y, x, h, w, y, w, h };

		opengl2_shader_use(PROG_WINDOW);

		glEnableVertexAttribArray(SDRI_POSITION);
		glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

		glEnableVertexAttribArray(SDRI_TEXCOORD);
		glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

		glBindTexture(GL_TEXTURE_2D, FB_texture);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, 0);

		glDisableVertexAttribArray(SDRI_TEXCOORD);
		glDisableVertexAttribArray(SDRI_POSITION);
	}

	mouse_eval_deltas();

	if ( mouse_is_visible() ) {
		int mx, my;

		mouse_get_pos(&mx, &my);

		if ( opengl2_tcache_set(Gr_cursor, TCACHE_TYPE_BITMAP_INTERFACE) ) {
			opengl2_set_state(TEXTURE_SOURCE_DECAL, ALPHA_BLEND_ALPHA_BLEND_ALPHA, ZBUFFER_TYPE_NONE);

			int bw, bh;
			bm_get_info(Gr_cursor, &bw, &bh);

			float x = i2fl(mx) * GL_viewport_scale_w;
			float y = i2fl(my) * GL_viewport_scale_h;
			float w = x + (bw * GL_viewport_scale_w);
			float h = y + (bh * GL_viewport_scale_h);

			const float tex_coord[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
			const float ver_coord[] = { x, y, x, h, w, y, w, h };

			opengl2_shader_use(PROG_WINDOW);

			glEnableVertexAttribArray(SDRI_POSITION);
			glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

			glEnableVertexAttribArray(SDRI_TEXCOORD);
			glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glDisableVertexAttribArray(SDRI_TEXCOORD);
			glDisableVertexAttribArray(SDRI_POSITION);
		}
#ifndef NDEBUG
		else {
			gr_set_color(255,255,255);
			gr_opengl2_line(mx, my, mx+7, my + 7);
			gr_opengl2_line(mx, my, mx+5, my );
			gr_opengl2_line(mx, my, mx, my+5);
		}
#endif
	}

#ifndef NDEBUG
	GLenum error = glGetError();

	if (error != GL_NO_ERROR) {
		mprintf(("!!DEBUG!! OpenGL Error: %d\n", error));
	}
#endif

	SDL_GL_SwapWindow(GL_window);

	opengl2_tcache_frame();

	glBindFramebuffer(GL_FRAMEBUFFER, FB_id);

	// set viewport to game screen size
	glViewport(0, 0, gr_screen.max_w, gr_screen.max_h);
}

void gr_opengl2_set_clip(int x, int y, int w, int h)
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

void gr_opengl2_fog_set(int fog_mode, int r, int g, int b, float fog_near, float fog_far)
{
	gr_screen.current_fog_mode = fog_mode;

	if (fog_mode == GR_FOGMODE_NONE) {
		return;
	}

	gr_screen.fog_near = fog_near;
	gr_screen.fog_far = fog_far;

	gr_init_color(&gr_screen.current_fog_color, r, g, b);
}

void gr_opengl2_zbuffer_clear(int mode)
{
	if (mode) {
		Gr_zbuffering = 1;
		Gr_zbuffering_mode = GR_ZBUFF_FULL;
		Gr_global_zbuffering = 1;

		opengl2_set_state(TEXTURE_SOURCE_NONE, ALPHA_BLEND_NONE, ZBUFFER_TYPE_FULL);
		glClear(GL_DEPTH_BUFFER_BIT);
	} else {
		Gr_zbuffering = 0;
		Gr_zbuffering_mode = GR_ZBUFF_NONE;
		Gr_global_zbuffering = 0;
	}
}

void gr_opengl2_fade_in(int instantaneous)
{

}

void gr_opengl2_fade_out(int instantaneous)
{

}

void gr_opengl2_get_region(int front, int w, int h, ubyte *data)
{
	opengl2_set_state(TEXTURE_SOURCE_NO_FILTERING, ALPHA_BLEND_NONE, ZBUFFER_TYPE_NONE);

	GLenum pxtype = GL_UNSIGNED_SHORT_5_5_5_1;

	if (gr_screen.bytes_per_pixel == 4) {
		pxtype = GL_UNSIGNED_BYTE;
	}

	glReadPixels(0, gr_screen.max_h-h-1, w, h, GL_RGBA, pxtype, data);
}

int gr_opengl2_save_screen()
{
	gr_opengl_reset_clip();

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

void gr_opengl2_restore_screen(int)
{
	gr_opengl_reset_clip();

	if ( !GL_saved_screen_tex ) {
		gr_opengl_clear();
		return;
	}

	float x = 0.0f;
	float y = 0.0f;
	float w = i2fl(gr_screen.max_w);
	float h = i2fl(gr_screen.max_h);

	const float tex_coord[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };	// y-flipped
	const float ver_coord[] = { x, y, x, h, w, y, w, h };

	opengl2_shader_use(PROG_TEX);

	glVertexAttrib4f(SDRI_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

	glEnableVertexAttribArray(SDRI_POSITION);
	glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &ver_coord);

	glEnableVertexAttribArray(SDRI_TEXCOORD);
	glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, &tex_coord);

	glBindTexture(GL_TEXTURE_2D, GL_saved_screen_tex);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableVertexAttribArray(SDRI_TEXCOORD);
	glDisableVertexAttribArray(SDRI_POSITION);
}

void gr_opengl2_free_screen(int)
{
	if (GL_saved_screen_tex) {
		glDeleteTextures(1, &GL_saved_screen_tex);
		GL_saved_screen_tex = 0;
	}
}

void gr_opengl2_dump_frame_start(int first_frame, int frames_between_dumps)
{

}

void gr_opengl2_dump_frame_stop()
{

}

void gr_opengl2_dump_frame()
{

}

static int GL_stream_w = 0;
static int GL_stream_h = 0;

static rb_t GL_stream[4];

void gr_opengl2_stream_start(int x, int y, int w, int h)
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

void gr_opengl2_stream_frame(ubyte *frame)
{
	if ( !GL_stream_tex ) {
		return;
	}

	opengl2_shader_use(PROG_TEX);

	glVertexAttrib4f(SDRI_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

	glEnableVertexAttribArray(SDRI_POSITION);
	glVertexAttribPointer(SDRI_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &GL_stream[0].x);

	glEnableVertexAttribArray(SDRI_TEXCOORD);
	glVertexAttribPointer(SDRI_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(rb_t), &GL_stream[0].u);

	glBindTexture(GL_TEXTURE_2D, GL_stream_tex);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GL_stream_w, GL_stream_h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, frame);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableVertexAttribArray(SDRI_TEXCOORD);
	glDisableVertexAttribArray(SDRI_POSITION);
}

void gr_opengl2_stream_stop()
{
	if (GL_stream_tex) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &GL_stream_tex);
		GL_stream_tex = 0;

		glEnable(GL_DEPTH_TEST);
	}
}

void gr_opengl2_set_viewport(int width, int height)
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

	GL_viewport_x = x;
	GL_viewport_y = y;
	GL_viewport_w = w;
	GL_viewport_h = h;
	GL_viewport_scale_w = w / i2fl(gr_screen.max_w);
	GL_viewport_scale_h = h / i2fl(gr_screen.max_h);

	opengl2_shader_update();
}
