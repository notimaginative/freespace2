#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "unix.h"

#define MAX_LINE_WIDTH 128

void strlwr (char * str)
{
	while (*str) {*str = tolower (*str); str++; }
}

int filelength (int fd)
{
	struct stat buf;
	if (fstat (fd, &buf) == -1)
		return -1;
		
	return buf.st_size;
}

unsigned long _beginthread (void (*pfuncStart)(void *), unsigned unStackSize, void* pArgList)
{
	STUB_FUNCTION;
	
	return 0;
}

void Sleep (int mili)
{
	usleep (mili * 1000);
}

void OutputDebugString (const char *str)
{
	fprintf(stderr, "OutputDebugString: %s\n", str);
}

int WSAGetLastError()
{
	return errno;
}

int MulDiv(int a, int b, int c)
{
	/* slow long long version */
	__extension__ long long aa = a;
	__extension__ long long bb = b;
	__extension__ long long cc = c;
	
	__extension__ long long dd = aa * bb;
	__extension__ long long ee = dd / cc;
	
	int retr = (int) ee;
	
	return retr;
}

/* mem debug junk */
#ifndef NDEBUG
//#define WANT_DEBUG
#endif

int TotalRam = 0;

#ifdef WANT_DEBUG
typedef struct RAM {
	void *addr;
	int size;
	
	char *file;
	int line;
	
	RAM *next;
} RAM;

static RAM *RamTable;
#endif

void vm_free(void* ptr, char *file, int line)
{
#ifdef WANT_DEBUG
	fprintf(stderr, "FREE: %s:%d addr = %p\n", file, line, ptr);
	
	RAM *item = RamTable;
	RAM **mark = &RamTable;
	
	while (item != NULL) {
		if (item->addr == ptr) {
			RAM *tmp = item;
			
			*mark = item->next;
			
			free(tmp->addr);
			free(tmp);
			
			return;
		}
		
		mark = &(item->next);
		
		item = item->next;
	}
	
	fprintf(stderr, "ERROR: vm_free caught invalid free: addr = %p, file = %s/%d\n", ptr, file, line);
#else	
	free(ptr);
#endif
}

void *vm_malloc(int size, char *file, int line)
{
#ifdef WANT_DEBUG
	fprintf(stderr, "MALLOC: %s:%d %d bytes\n", file, line, size);
	
	RAM *next = (RAM *)malloc(sizeof(RAM));
	
	next->addr = malloc(size);
	next->size = size;
	next->file = file;
	next->line = line;
	
	next->next = RamTable;
	RamTable = next;
	
	return next->addr;
#else
	return malloc(size);
#endif	
}

char *vm_strdup(char const* str, char *file, int line)
{
#ifdef WANT_DEBUG
	fprintf(stderr, "STRDUP: %s:%d\n", file, line);
	
	RAM *next = (RAM *)malloc(sizeof(RAM));
	
	next->addr = strdup(str);
	next->size = strlen(str)+1;
	next->file = file;
	next->line = line;
	
	next->next = RamTable;
	RamTable = next;
	
	return (char *)next->addr;
#else
	return strdup(str);
#endif
}

void vm_dump()
{
#ifdef WANT_DEBUG
	int i = 0;
	int mem = 0;
	fprintf(stderr, "\nDumping allocated memory:\n");
	
	RAM *ptr = RamTable;
	while (ptr) {
		fprintf(stderr, "%d: file: %s:%d: addr:%p size:%d\n", i, ptr->file, ptr->line, ptr->addr, ptr->size);
		mem += ptr->size;
		ptr = ptr->next;
		i++;
	}
	
	fprintf(stderr, "\nTotal of %d left-over bytes from %d allocations\n", mem, i);
#endif	
}

void windebug_memwatch_init()
{
	TotalRam = 0;
}

/* error message debugging junk */
int Log_debug_output_to_file = 0;

void load_filter_info(void)
{
	STUB_FUNCTION;
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
	STUB_FUNCTION;
}

void Warning( char * filename, int line, char * format, ... )
{
	char tmp[MAX_LINE_WIDTH*4];
	va_list args;

	va_start (args, format);
	vsprintf (tmp, format, args);
	va_end(args);
	fprintf (stderr, "Warning: (%s:%d): %s\n", filename, line, tmp);
}

void Error( char * filename, int line, char * format, ... )
{
	char tmp[MAX_LINE_WIDTH*4];
	va_list args;

	va_start (args, format);
	vsprintf (tmp, format, args);
	va_end(args);
	fprintf (stderr, "Error: (%s:%d): %s\n", filename, line, tmp);
	exit (1);
}

void WinAssert(char * text,char *filename, int line)
{
	fprintf (stderr, "Assertion: (%s:%d) %s\n", filename, line, text);
}
