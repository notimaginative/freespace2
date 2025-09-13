#ifndef PLATFORM_H
#define PLATFORM_H


#include <cstdio>	// For NULL, etc
#include <cstdlib>
#include <memory.h>
#include <cstring>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifndef SDL_PLATFORM_WINDOWS
#include "unix.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#else
#include "win.h"
#endif


void base_filename(const char *path, char *filename, const int max_fname);

int platform_open_url(const char *url);

#endif // PLATFORM_H
