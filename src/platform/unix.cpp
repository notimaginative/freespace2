#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "unix.h"

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
