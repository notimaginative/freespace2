/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <exception>

#include <SDL3/SDL_main.h>
#include "pstypes.h"

#ifdef PLAT_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#endif


extern "C" int game_main(const char *szCmdLine);


#if defined(PLAT_UNIX) && !defined(__EMSCRIPTEN__)
static void daemonize()
{
	pid_t pid = fork();

	if (pid == -1) {
		exit(EXIT_FAILURE);
	} else if (pid != 0) {
		_exit(0);
	}

	if (setsid() == -1) {
		exit(EXIT_FAILURE);
	}

	signal(SIGHUP, SIG_IGN);

	pid = fork();

	if (pid == -1) {
		exit(EXIT_FAILURE);
	} else if (pid != 0) {
		_exit(0);
	}

	if (chdir("/") == -1) {
		exit(EXIT_FAILURE);
	}

	umask(0);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_WRONLY);
	open("/dev/null", O_RDWR);
}
#endif


extern "C"
int main(int argc, char *argv[])
{
	char *argptr = NULL;
	int i;
	int len = 0;
	int retr = 0;

#if defined(PLAT_UNIX) && !defined(__EMSCRIPTEN__)
	// if we are standalone headless, daemonize
	bool daemon = false;
	bool standalone = false;

	for (i = 1; i < argc; i++) {
		if ( !daemon && SDL_strstr(argv[i], "-daemon") ) {
			daemon = true;
		}

		if ( !standalone ) {
			if ( SDL_strstr(argv[i], "-standalone") ) {
				standalone = true;
			}

			if ( !SDL_strcmp(argv[i], "-b") ) {
				standalone = true;
			}
		}
	}

	if (standalone && daemon) {
		daemonize();
	}

	// make sure we create files with user access only
	umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif

	for (i = 1; i < argc; i++) {
		len += strlen(argv[i]) + 1;
	}

	if (len > 0) {
		argptr = (char *)SDL_malloc(len+5);

		if (argptr == NULL) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", "Ran out of memory in main()!", NULL);
			exit(1);
		}

		memset(argptr, 0, len+5);

		for (i = 1; i < argc; i++) {
			SDL_strlcat(argptr, argv[i], len+5);
			SDL_strlcat(argptr, " ", len+5);
		}
	}

	try {
		retr = game_main(argptr);
	} catch(const std::exception &e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", e.what(), NULL);
	} catch(...) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", "Exception caught in main()!", NULL);
	}

	if (argptr) {
		SDL_free(argptr);
	}

	return retr;	
}
