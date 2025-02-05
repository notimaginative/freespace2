/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifdef PLAT_UNIX

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
		
	return buf.st_size;
}

int WSAGetLastError()
{
	return errno;
}

int platform_open_url(const char *url)
{
#ifdef __APPLE__
	const char *open_cmd = "open";
#else
	const char *open_cmd = "xdg-open";
#endif
	char s_url[256];
	int statval = 0;

	// make sure it's a valid www address
	if ( !SDL_strncasecmp(url, "http://", 7) || !SDL_strncasecmp(url, "https://", 8) ) {
		SDL_strlcpy(s_url, url, SDL_arraysize(s_url));
	} else {
		SDL_strlcpy(s_url, "http://", SDL_arraysize(s_url));
		SDL_strlcat(s_url, url, SDL_arraysize(s_url));
	}

	pid_t mpid = fork();

	if (mpid < 0) {
		// nothing, will return error
	} else if (mpid == 0) {
		int rv = 0;

		rv = execlp(open_cmd, open_cmd, s_url, (char *)0);

		exit(rv);
	} else {
		waitpid(mpid, &statval, 0);

		if ( WIFEXITED(statval) ) {
			if (WEXITSTATUS(statval) == 0) {
				return 0;
			} else {
				return -1;
			}
		}
	}

	return -1;
}

#endif
