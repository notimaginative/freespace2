#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "unix.h"

#define MAX_LINE_WIDTH 128

void strlwr (char * str)
{
	while (*str) {*str = tolower (*str); str++; }
}

int filelength (int fd)
{
	FILE *f = fdopen (dup(fd), "r");
	fseek (f, 0, SEEK_END);
	int len = ftell (f);
	fclose (f);
	return len;
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
int TotalRam = 0;

void vm_free(void* ptr, char*, int)
{
	free(ptr);
}

void *vm_malloc(int size, char*, int)
{
	return malloc(size);
}

char *vm_strdup(char const* str, char*, int)
{
	return strdup(str);
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
	fprintf (stderr, "%s: %s", id, tmp);
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
	STUB_FUNCTION;
}

void Error( char * filename, int line, char * format, ... )
{
	STUB_FUNCTION;
}

void WinAssert(char * text,char *filename, int line)
{
	STUB_FUNCTION;
}
