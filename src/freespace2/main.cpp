/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"

#ifdef PLAT_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

#undef malloc
#undef free

extern int game_main(const char *szCmdLine);

void vm_dump();


int main(int argc, char **argv)
{
	char *argptr = NULL;
	int i;
	int len = 0;
	int retr = 0;

#ifdef PLAT_UNIX
	// make sure we create files with user access only
	umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif

	for (i = 1; i < argc; i++) {
		len += strlen(argv[i]) + 1;
	}

	if (len > 0) {
		argptr = (char *)malloc(len+5);

		if (argptr == NULL) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", "Ran out of memory in main()!", NULL);
			exit(1);
		}

		memset(argptr, 0, len+5);

		for (i = 1; i < argc; i++) {
			strcat(argptr, argv[i]);
			strcat(argptr, " ");
		}
	}

	try {
		retr = game_main(argptr);
	} catch(...) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", "Exception caught in main()!", NULL);
	}

	if (argptr) {
		free(argptr);
	}
		
	vm_dump();
	
	return retr;	
}
