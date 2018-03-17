/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef PLAT_UNIX

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "SDL.h"
#include "osapi.h"


int platform_open_url(const char *url)
{
	char s_url[256];

	// make sure it's a valid www address
	if ( !SDL_strncasecmp(url, "http://", 7) || !SDL_strncasecmp(url, "https://", 8) ) {
		SDL_strlcpy(s_url, url, SDL_arraysize(s_url));
	} else {
		SDL_snprintf(s_url, SDL_arraysize(s_url), "http://%s", url);
	}

	int rval = (int) ShellExecuteA(NULL, "open", s_url, NULL, NULL, SW_SHOWNORMAL);

	if (rval <= 32) {
		return -1;
	}

	return 0;
}

#endif
