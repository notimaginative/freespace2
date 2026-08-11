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


int main(int argc, char *argv[])
{
	int retr = 0;
	const auto sdl_ver = SDL_GetVersion();
	bool server = false;
	ushort port = DEFAULT_GAME_PORT;

	// figure out if we're running standalone to use special log file
	for (int i = 1; i < argc; ++i) {
		if (SDL_strstr(argv[i], "-standalone") || !SDL_strcmp(argv[i], "-d")) {
			server = true;
		} else if (SDL_strstr(argv[i], "-port") || !SDL_strcmp(argv[i], "-o")) {
			if (i < argc-1) {
				port = static_cast<ushort>(SDL_atoi(argv[i+1]));
			}
		}
	}

	outwnd_init(server, port);

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

	SDL_Log("\n");

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
