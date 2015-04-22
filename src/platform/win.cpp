/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef PLAT_UNIX

#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>

#include "SDL.h"


int platform_open_url(const char *url)
{
	char s_url[256];

	// make sure it's a valid www address
	if ( !SDL_strncasecmp(url, "http://", 7) || !SDL_strncasecmp(url, "https://", 8) ) {
		SDL_strlcpy(s_url, url, SDL_arraysize(s_url));
	} else {
		SDL_strlcpy(s_url, "http://", SDL_arraysize(s_url));
		SDL_strlcat(s_url, url, SDL_arraysize(s_url));
	}

	int rval = (int) ShellExecute(NULL, "open", s_url, NULL, NULL, SW_SHOW);

	if (rval < 32) {
		switch (rval) {
			case 0:
			case ERROR_BAD_FORMAT:
			case SE_ERR_ACCESSDENIED:
			case SE_ERR_ASSOCINCOMPLETE:
			case SE_ERR_DDEBUSY:
			case SE_ERR_DDEFAIL:
			case SE_ERR_DDETIMEOUT:
			case SE_ERR_DLLNOTFOUND:
			case SE_ERR_OOM:
			case SE_ERR_SHARE:
			case SE_ERR_NOASSOC:
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				return -1;
		}
	}

	return 0;
}

#endif
