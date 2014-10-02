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
#include <errno.h>
#include <sys/stat.h>

#include "pstypes.h"


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
