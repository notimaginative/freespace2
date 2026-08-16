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


// easy macros to disable compiler warnings (when absolutely required)
// (via: https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/)
#if defined(_MSC_VER)

#define DISABLE_WARNING_PUSH			__pragma(warning(push))
#define DISABLE_WARNING_POP				__pragma(warning(pop))
#define DISABLE_WARNING(warningNumber)	__pragma(warning(disable: warningNumber))

#define DISABLE_WARNING_SHADOW

#elif defined(__GNUC__) || defined(__clang__)

#define DO_PRAGMA(x) _Pragma(#x)
#define DISABLE_WARNING_PUSH			DO_PRAGMA(GCC diagnostic push)
#define DISABLE_WARNING_POP				DO_PRAGMA(GCC diagnostic pop)
#define DISABLE_WARNING(warningName)	DO_PRAGMA(GCC diagnostic ignored #warningName)

#define DISABLE_WARNING_SHADOW	DISABLE_WARNING(-Wshadow)

#else	// fallback for anything else

#define DISABLE_WARNING_PUSH
#define DISABLE_WARNING_POP
#define DISABLE_WARNING(warningName)

#define DISABLE_WARNING_SHADOW

#endif
// -- end compiler warning macros


void base_filename(const char *path, char *filename, const int max_fname);

int platform_open_url(const char *url);

#endif // PLATFORM_H
