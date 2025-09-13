/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef SDL_PLATFORM_WINDOWS

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <SDL3/SDL.h>


int filelength (int fd)
{
	struct stat buf;
	if (fstat (fd, &buf) == -1)
		return -1;
		
	return static_cast<int>(buf.st_size);
}

int WSAGetLastError()
{
	return errno;
}

#endif
