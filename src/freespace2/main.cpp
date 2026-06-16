/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <exception>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "pstypes.h"
#include "launcher.h"
#include "version.h"
#include "osregistry.h"
#include "outwnd.h"
#include "cmdline.h"
#include "systemvars.h"


extern "C" int game_main();


#if !defined(SDL_PLATFORM_WINDOWS) && !defined(__EMSCRIPTEN__)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

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


int main(int argc, char *argv[])
{
	int retr = 0;
	auto sdl_ver = SDL_GetVersion();

	outwnd_init();

	SDL_SetAppMetadata(Osreg_title, version_get_string_full(), Osreg_app_id);

	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
							   "Copyright (C) Volition, Inc. 1999.  All rights reserved.");

	SDL_Log("Platform: %s", SDL_GetPlatform());
	SDL_Log("CPU: %d %s", SDL_GetNumLogicalCPUCores(), (SDL_GetNumLogicalCPUCores() == 1) ? "core" : "cores");
	SDL_Log("Memory: %d MiB", SDL_GetSystemRAM());
	SDL_Log("SDL version: %d.%d.%d (%d.%d.%d)", SDL_VERSIONNUM_MAJOR(sdl_ver),
			SDL_VERSIONNUM_MINOR(sdl_ver), SDL_VERSIONNUM_MICRO(sdl_ver),
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
	SDL_Log("Build: %d-bit, %s-endian", static_cast<int>(sizeof(void*)) * 8,
			(SDL_BYTEORDER == SDL_LIL_ENDIAN) ? "little" : "big");

#ifdef GIT_INFO
#ifdef GIT_TAG
	SDL_Log("Build ID: %s:%s", GIT_COMMIT_DATE, GIT_TAG);
#else
	SDL_Log("Build ID: %s~%s:%s", GIT_COMMIT_DATE, GIT_BRANCH, GIT_COMMIT_HASH);
#endif
#endif

	parse_cmdline(argc, argv);

	SDL_Log("");

#if !defined(SDL_PLATFORM_WINDOWS) && !defined(__EMSCRIPTEN__)
	// if we are standalone headless, daemonize
	if (Is_standalone && Cmdline_daemon) {
		daemonize();
	}

	// make sure we create files with user access only
	umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif

	try {
		if ( launcher_run() ) {
			retr = game_main();
		}
	} catch(const std::exception &e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", e.what(), NULL);
	} catch(...) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", "Exception caught in main()!", NULL);
	}

	return retr;
}
