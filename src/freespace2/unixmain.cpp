/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <sys/stat.h>

#include "pstypes.h"
#include "osregistry.h"
#include "osapi.h"

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

	for (i = 1; i < argc; i++) {
		len += strlen(argv[i]) + 1;
	}

	if (len > 0) {
		argptr = (char *)malloc(len+5);

		if (argptr == NULL) {
			fprintf(stderr, "ERROR: out of memory in main!\n");
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
		mprintf(("ERROR!! Exception caught in main()"));
		fprintf(stderr, "ERROR!! Exception caught in main()");
	}

	if (argptr) {
		free(argptr);
	}
		
	vm_dump();
	
	return retr;	
}
