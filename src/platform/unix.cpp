#include <ctype.h>
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
