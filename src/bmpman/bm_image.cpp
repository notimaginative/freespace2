/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "cfile.h"
#include "bmpman.h"

#define STB_IMAGE_IMPLEMENTATION

#define STBI_ASSERT(x)			SDL_assert(x)
#define STBI_MALLOC(sz)			malloc(sz)
#define STBI_REALLOC(p, newsz)	realloc(p, newsz)
#define STBI_FREE(p)			free(p)

// for simplicity we'll do:
//   only memory/io loading
//   only PNG and JPEG formats
#define STBI_NO_STDIO
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#include "ext/stb_image.h"


static int stbi__cfile_read(void *user, char *data, int size)
{
	return static_cast<int>(cfread(data, 1, size, reinterpret_cast<CFILE *>(user)));
}

static void stbi__cfile_skip(void *user, int n)
{
	cfseek(reinterpret_cast<CFILE *>(user), n, CF_SEEK_CUR);
}

static int stbi__cfile_eof(void *user)
{
	return cfeof(reinterpret_cast<CFILE *>(user)) ? 1 : 0;
}

static stbi_io_callbacks cfile_callbacks =
{
	stbi__cfile_read,
	stbi__cfile_skip,
	stbi__cfile_eof,
};


SDL_Surface *bm_image_to_surface(const char *filename, int dir_type, SDL_PixelFormat sformat)
{
	int x = 0, y = 0, bpp = 0;
	SDL_Surface *surface = nullptr;

	auto filep = cfopen(filename, "rb", dir_type);

	if ( !filep ) {
		return nullptr;
	}

	// prefer RGBA when it's for SDL surface usage (i.e., the "4")
	auto image = stbi_load_from_callbacks(&cfile_callbacks, filep,
										  &x, &y, &bpp, 4);

	cfclose(filep);

	if ( !image ) {
		return nullptr;
	}

	auto temp = SDL_CreateSurfaceFrom(x, y, SDL_PIXELFORMAT_RGBA32, image, y * bpp);

	// convert surface format if needed
	if (sformat != SDL_PIXELFORMAT_RGBA32) {
		surface = SDL_ConvertSurface(temp, sformat);

		SDL_DestroySurface(temp);
		temp = nullptr;
	} else {
		surface = temp;
	}

	free(image);

	return surface;
}

SDL_Surface *bm_image_to_surface(int bitmapnum, int bpp, int flags, SDL_PixelFormat sformat)
{
	SDL_Surface *surface = nullptr;

	SDL_assert(bpp == 16);	// only supporting 16-bit for now

	if (bitmapnum < 0) {
		return nullptr;
	}

	auto bmp = bm_lock(bitmapnum, static_cast<ubyte>(bpp), static_cast<ubyte>(flags));

	if ( !bmp ) {
		return nullptr;
	}

	// create temporary surface from 16-bit image data that bmpman uses
	auto temp = SDL_CreateSurfaceFrom(bmp->w, bmp->h, SDL_PIXELFORMAT_RGBA5551,
									  reinterpret_cast<void *>(bmp->data),
									  bmp->rowsize * (bmp->bpp >> 3));

	if (temp) {
		// convert to 32-bit for final surface
		surface = SDL_ConvertSurface(temp, sformat);

		SDL_DestroySurface(temp);
		temp = nullptr;
	}

	// must be done *after* format conversion
	bm_unlock(bitmapnum);
	bm_unload(bitmapnum);

	return surface;
}
