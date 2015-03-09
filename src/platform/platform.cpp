/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "pstypes.h"

// use system versions of this stuff in here rather than the vm_* versions
#undef malloc
#undef free
#undef strdup


#define MAX_LINE_WIDTH 128


int TotalRam = 0;


int vm_init(int min_heap_size)
{
	return 1;
}

#if defined(__MACOSX__)
#define MALLOC_SIZE(x)		malloc_size(x)
#elif defined(__GNUC__)
#define MALLOC_SIZE(x)		malloc_usable_size(x)
#elif defined(__WIN32__)
#define MALLOC_SIZE(x)		_msize(x)
#else
#define MALLOC_SIZE(x)		0
#endif

int Watch_malloc = 0;

DCF_BOOL(watch_malloc, Watch_malloc)

static const char *clean_filename(const char *name)
{
	const char *p = name+strlen(name)-1;
	// Move p to point to first letter of EXE filename
	while( (*p!='\\') && (*p!='/') && (*p!=':') )
		p--;
	p++;

	return p;
}

#ifndef NDEBUG
void vm_free(void* ptr, const char *file, int line)
#else
void vm_free(void* ptr)
#endif
{
	if ( !ptr ) {
#ifndef NDEBUG
		mprintf(("Why are you trying to free a NULL pointer?  [%s(%d)]\n", clean_filename(file), line));
#endif
		return;
	}

#ifndef NDEBUG
	size_t actual_size = MALLOC_SIZE(ptr);

	if (Watch_malloc) {
		mprintf(( "Free %d bytes [%s(%d)]\n", actual_size, clean_filename(file), line ));
	}

	TotalRam -= actual_size;
#endif

	free(ptr);
}

#ifndef NDEBUG
void *vm_malloc(int size, const char *file, int line)
#else
void *vm_malloc(int size)
#endif
{
	void *ptr = malloc(size);

	if ( !ptr )	{
		mprintf(( "Malloc failed!!!!!!!!!!!!!!!!!!!\n" ));

		Error(LOCATION, "Out of memory.  Try closing down other applications, increasing your\n"
				"virtual memory size, or installing more physical RAM.\n");

		return NULL;
	}

#ifndef NDEBUG
	size_t actual_size = MALLOC_SIZE(ptr);

	if ( Watch_malloc )	{
		mprintf(( "Malloc %d bytes [%s(%d)]\n", actual_size, clean_filename(file), line ));
	}

	TotalRam += actual_size;
#endif

	return ptr;
}

#ifndef NDEBUG
char *vm_strdup(char const* str, const char *file, int line)
#else
char *vm_strdup(char const* str)
#endif
{
	char *ptr = strdup(str);

	if ( !ptr )	{
		mprintf(( "Strdup failed!!!!!!!!!!!!!!!!!!!\n" ));

		Error(LOCATION, "Out of memory.  Try closing down other applications, increasing your\n"
				"virtual memory size, or installing more physical RAM.\n");
	}

#ifndef NDEBUG
	size_t actual_size = MALLOC_SIZE(ptr);

	if ( Watch_malloc )	{
		mprintf(( "Strdup %d bytes [%s(%d)]\n", actual_size, clean_filename(file), line ));
	}

	TotalRam += actual_size;
#endif

	return ptr;
}

void windebug_memwatch_init()
{
	TotalRam = 0;
}


extern void gr_force_windowed();

void Warning( const char * filename, int line, const char * format, ... )
{
#ifndef NDEBUG
	char tmp[MAX_LINE_WIDTH*4] = { 0 };
	char tmp2[MAX_LINE_WIDTH*4] = { 0 };
	va_list args;

	va_start(args, format);
	SDL_vsnprintf(tmp, sizeof(tmp), format, args);
	va_end(args);

	SDL_snprintf(tmp2, sizeof(tmp2), "Warning: %s\n\nFile:%s\nLine: %d", tmp, filename, line);

	gr_force_windowed();

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning!", tmp2, NULL);
#endif
}

void Error( const char * filename, int line, const char * format, ... )
{
	char tmp[MAX_LINE_WIDTH*4] = { 0 };
	char tmp2[MAX_LINE_WIDTH*4] = { 0 };
	va_list args;

	va_start (args, format);
	SDL_vsnprintf (tmp, sizeof(tmp), format, args);
	va_end(args);

	SDL_snprintf(tmp2, sizeof(tmp2), "Error: %s\n\nFile:%s\nLine: %d", tmp, filename, line);

	gr_force_windowed();

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", tmp2, NULL);

	exit (1);
}

void base_filename(const char *path, char *filename, const int max_fname)
{
	if ( (filename == NULL) || (max_fname <= 0) ) {
		return;
	}

	if (path == NULL) {
		filename[0] = '\0';
		return;
	}

	const char *sep = SDL_strrchr(path, DIR_SEPARATOR_CHAR);

	if (sep) {
		sep++;	// move past separator
	} else {
		sep = path;
	}

	const char *ext = SDL_strrchr(path, '.');

	if (ext == NULL) {
		ext = sep + SDL_strlen(sep);	// to end
	}

	// NOTE: 'size' must include NULL terminator
	int size = min((int)(ext - sep + 1), max_fname);

	if (size <= 0) {
		filename[0] = '\0';
	} else {
		SDL_strlcpy(filename, sep, size);
	}
}
