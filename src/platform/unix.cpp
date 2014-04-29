#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pstypes.h"

// use system versions of this stuff in here rather than the vm_* versions
#undef malloc
#undef free
#undef strdup


#define MAX_LINE_WIDTH 128


int filelength (int fd)
{
	struct stat buf;
	if (fstat (fd, &buf) == -1)
		return -1;
		
	return buf.st_size;
}

int WSAGetLastError()
{
	return errno;
}

void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
	if (path == NULL)
		return;

	/* fs2 only uses fname */
	if (fname != NULL) {
		const char *ls = strrchr(path, '/');
		if (ls != NULL) {
			ls++;		// move past '/'
		} else {
			ls = path;
		}
	
		const char *lp = strrchr(path, '.');
		if (lp == NULL) {
			lp = ls + strlen(ls);	// move to the end
		}
	
		int dist = lp-ls;
		if (dist > (_MAX_FNAME-1))
			dist = _MAX_FNAME-1;
		
		strncpy(fname, ls, dist);
		fname[dist] = 0;	// add null, just in case
	}
}

int TotalRam = 0;


int vm_init(int min_heap_size)
{
	return 1;
}

#if defined(__MACOSX__)
#define MALLOC_SIZE(x)		malloc_size(x)
#elif defined(__GNUC__)
#define MALLOC_SIZE(x)		malloc_usable_size(x)
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

/* error message debugging junk */
/*
int Log_debug_output_to_file = 0;

void load_filter_info(void)
{
//	STUB_FUNCTION;
}

void outwnd_printf(char* id, char* format, ...)
{
	char tmp[MAX_LINE_WIDTH*4];
	va_list args;

	va_start (args, format);
	vsprintf (tmp, format, args);
	va_end(args);
	fprintf (stderr, "%s: %s\n", id, tmp);
}

void outwnd_printf2(char* format, ...)
{
	char tmp[MAX_LINE_WIDTH*4];
	va_list args;

	va_start (args, format);
	vsprintf (tmp, format, args);
	va_end(args);
	fprintf (stderr, "General: %s", tmp);
}

void outwnd_close()
{
//	STUB_FUNCTION;
}
*/

extern void gr_force_windowed();

void Warning( const char * filename, int line, const char * format, ... )
{
	char tmp[MAX_LINE_WIDTH*4] = { 0 };
	char tmp2[MAX_LINE_WIDTH*4] = { 0 };
	va_list args;

	va_start(args, format);
	vsnprintf(tmp, sizeof(tmp), format, args);
	va_end(args);
//	fprintf (stderr, "Warning: (%s:%d): %s\n", filename, line, tmp);
	snprintf(tmp2, sizeof(tmp2), "Warning: %s\n\nFile:%s\nLine: %d", tmp, filename, line);

	gr_force_windowed();

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning!", tmp2, NULL);
}

void Error( const char * filename, int line, const char * format, ... )
{
	char tmp[MAX_LINE_WIDTH*4] = { 0 };
	char tmp2[MAX_LINE_WIDTH*4] = { 0 };
	va_list args;

	va_start (args, format);
	vsnprintf (tmp, sizeof(tmp), format, args);
	va_end(args);
//	fprintf (stderr, "Error: (%s:%d): %s\n", filename, line, tmp);
	snprintf(tmp2, sizeof(tmp2), "Error: %s\n\nFile:%s\nLine: %d", tmp, filename, line);

	gr_force_windowed();

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", tmp2, NULL);

	exit (1);
}
