// unix.h - duplicates some MS defines

#ifndef _UNIX_H
#define _UNIX_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "SDL.h"

#define DWORD int
#define _MAX_FNAME 255
#define _MAX_PATH 255
#define _MAX_DIR 256
#define MAX_FILENAME_LENGTH 64
#define _cdecl
#define __int64 long long
#define LARGE_INTEGER long long
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define _isnan isnan
#define HANDLE int
#define _getcwd getcwd
#define _chdir chdir
#define _strlwr strlwr
#define _unlink unlink
#define _mkdir mkdir
#define _hypot hypot
#define byte unsigned char

extern void strlwr (char *str);
extern int filelength (int fd);
extern int MulDiv (int, int, int);
#define CRITICAL_SECTION SDL_mutex*

#define STUB_FUNCTION fprintf(stderr,"STUB: %s at " __FILE__ ", line %d, thread %d\n",__FUNCTION__,__LINE__,getpid())
#endif

