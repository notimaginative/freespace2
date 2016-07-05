/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */


#include "pstypes.h"
#include "2d.h"


static void stub_void_void()
{
}

static void stub_void_int(int)
{
}

static void stub_void_int2(int, int)
{
}

static void stub_void_int3(int, int, int)
{
}

static void stub_void_int4(int, int, int, int)
{
}

static void stub_void_int6(int, int, int, int, int, int)
{
}

static void stub_void_vertex2(vertex *, vertex *)
{
}

static void stub_string(int, int, const char *)
{
}

static void stub_tmapper(int, vertex *[], uint)
{
}

static void stub_print_screen(const char *)
{
}

static int stub_save_screen()
{
	return -1;
}

static void stub_set_gamma(float)
{
}

static uint stub_lock()
{
	return 0;
}

static void stub_fog_set(int, int, int, int, float, float)
{
}

static void stub_get_region(int, int, int, ubyte *)
{
}

static void stub_cross_fade(int, int, int, int, int, int, float)
{
}

static int stub_preload(int, int)
{
	return 1;
}

static void stub_stream_frame(ubyte *)
{
}


void gr_stub_init()
{
	gr_screen.gf_flip = stub_void_void;
	gr_screen.gf_set_clip = stub_void_int4;
	gr_screen.gf_reset_clip = stub_void_void;

	gr_screen.gf_clear = stub_void_void;

	gr_screen.gf_aabitmap = stub_void_int2;
	gr_screen.gf_aabitmap_ex = stub_void_int6;

	gr_screen.gf_rect = stub_void_int4;
	gr_screen.gf_shade = stub_void_int4;
	gr_screen.gf_string = stub_string;
	gr_screen.gf_circle = stub_void_int3;

	gr_screen.gf_line = stub_void_int4;
	gr_screen.gf_aaline = stub_void_vertex2;
	gr_screen.gf_pixel = stub_void_int2;
	gr_screen.gf_scaler = stub_void_vertex2;
	gr_screen.gf_tmapper = stub_tmapper;

	gr_screen.gf_gradient = stub_void_int4;

	gr_screen.gf_print_screen = stub_print_screen;

	gr_screen.gf_fade_in = stub_void_int;
	gr_screen.gf_fade_out = stub_void_int;
	gr_screen.gf_flash = stub_void_int3;

	gr_screen.gf_zbuffer_clear = stub_void_int;

	gr_screen.gf_save_screen = stub_save_screen;
	gr_screen.gf_restore_screen = stub_void_int;
	gr_screen.gf_free_screen = stub_void_int;

	gr_screen.gf_dump_frame_start = stub_void_int2;
	gr_screen.gf_dump_frame_stop = stub_void_void;
	gr_screen.gf_dump_frame = stub_void_void;

	gr_screen.gf_stream_start = stub_void_int4;
	gr_screen.gf_stream_frame = stub_stream_frame;
	gr_screen.gf_stream_stop = stub_void_void;

	gr_screen.gf_set_gamma = stub_set_gamma;

	gr_screen.gf_lock = stub_lock;
	gr_screen.gf_unlock = stub_void_void;

	gr_screen.gf_fog_set = stub_fog_set;

	gr_screen.gf_get_region = stub_get_region;

	gr_screen.gf_set_cull = stub_void_int;

	gr_screen.gf_cross_fade = stub_cross_fade;

	gr_screen.gf_preload_init = stub_void_void;
	gr_screen.gf_preload = stub_preload;

	gr_screen.gf_zbias = stub_void_int;

	gr_screen.gf_set_viewport = stub_void_int2;

	gr_screen.gf_activate = stub_void_int;

	gr_screen.gf_release_texture = stub_void_int;
}
