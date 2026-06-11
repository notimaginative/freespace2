/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "launcher_internal.h"
#include "cfile.h"
#include "cfilesystem.h"
#include "osregistry.h"

#include <string>

#include <vdf_parser.hpp>

#ifdef SDL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static void fix_dir_seps(std::string &path)
{
#ifdef SDL_PLATFORM_WINDOWS
	std::replace(path.begin(), path.end(), '/', '\\');
#endif
}

//
// Steam helper to locate game installation path, if it exists
//

static std::string steam_get_root()
{
#if defined(SDL_PLATFORM_WINDOWS)
	HKEY hKey;
	char path[1024] = {};
	DWORD pathSize = static_cast<DWORD>(sizeof(path));

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		auto rval = RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr,
									 reinterpret_cast<LPBYTE>(path), &pathSize);
		RegCloseKey(hKey);

		if (rval == ERROR_SUCCESS) {
			return std::string(path);
		}
	}
#else
	const char *locations[] = {
		"Library/Application Support/Steam",
		".local/share/Steam",
		".steam/steam"
	};

	auto home = SDL_GetUserFolder(SDL_FOLDER_HOME);

	if ( !home ) {
		return "";
	}

	std::string path;

	for (size_t idx = 0; idx < SDL_arraysize(locations); ++idx) {
		path = home;
		path += locations[idx];

		// verify path exists
		if (SDL_GetPathInfo(path.c_str(), nullptr)) {
			return path;
		}
	}
#endif

	return "";
}

static bool steam_get_game_path(std::string &location, const uint32_t app_id)
{
	auto steamPath = steam_get_root();

	if (steamPath.empty()) {
		return false;
	}

	// first, locate possible steam library locations
	auto libraryVDF = steamPath + "/steamapps/libraryfolders.vdf";
	fix_dir_seps(libraryVDF);

	if ( !SDL_GetPathInfo(libraryVDF.c_str(), nullptr) ) {
		return false;
	}

	try {
		std::ifstream file(libraryVDF);
		auto root = tyti::vdf::read(file);

		if (root.name != "libraryfolders") {
			return false;
		}

		// next, check each library for the app manifest of the game
		const std::string manifestPath = "/steamapps/appmanifest_" + std::to_string(app_id) + ".acf";

		for (auto &child : root.childs) {
			try {
				std::string libraryPath = child.second->attribs.at("path");

				std::string manifest = libraryPath + manifestPath;
				fix_dir_seps(manifest);

				std::ifstream app(manifest);
				auto appRoot = tyti::vdf::read(app);

				// if we have a valid manifest then grab the install directory
				// and construct a full path
				if (appRoot.name == "AppState") {
					std::string installdir = appRoot.attribs.at("installdir");

					std::string full_path = libraryPath + "/steamapps/common/" + installdir;
					fix_dir_seps(full_path);

					// verify path exists
					if ( !SDL_GetPathInfo(full_path.c_str(), nullptr) ) {
						return false;
					}

					location = std::move(full_path);
					return true;
				}
			} catch(...) {}
		}
	} catch(...) {}

	return false;
}

static bool steam_locator(std::string &location)
{
#ifdef MAKE_FS1
	const uint32_t app_id = 273600;
#else
	const uint32_t app_id = 273620;
#endif

	return steam_get_game_path(location, app_id);
}


//
// GOG helper to locate game installation path, if it exists
//

static bool gog_get_game_path(std::string &location)
{
#if defined(SDL_PLATFORM_WINDOWS)
	HKEY hKey;
	char path[1024] = {};
	DWORD pathSize = static_cast<DWORD>(sizeof(path));

#ifdef MAKE_FS1
	const char *subkey = "SOFTWARE\\WOW6432Node\\GOG.com\\GOGFREESPACE";
#else
	const char *subkey = "SOFTWARE\\WOW6432Node\\GOG.com\\Games\\5";
#endif

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		auto rval = RegQueryValueExA(hKey, "PATH", nullptr, nullptr,
									 reinterpret_cast<LPBYTE>(path), &pathSize);
		RegCloseKey(hKey);

		if (rval == ERROR_SUCCESS) {
			// verify path exists
			if ( !SDL_GetPathInfo(path, nullptr) ) {
				return false;
			}

			location = path;
			return true;
		}
	}
#endif

	return false;
}

static bool gog_locator(std::string &location)
{
	return gog_get_game_path(location);
}


//
// General launcher helpers
//

static bool data_locator()
{
	std::string location;

	if ( !steam_locator(location) && !gog_locator(location) ) {
		return false;
	}

	// just to be extra safe
	if (location.empty()) {
		return false;
	}

	// set installation path to what we found
	os_config_write_string(nullptr, "ExtrasPath", location.c_str());

	return true;
}

void launcher_data_missing_error()
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game data not found!",
							 "A working game installation was not found. Please "
							 "set the Extras Path in Setup to the location "
							 "where your game is installed.",
							 launcher_get_window());

	launcher_setup_open(LauncherSetupTab::Misc, true);
}

bool launcher_ready_to_play(const bool run_locator)
{
	bool ready = false;

	// check for two different types of files that are in all versions of the game
	bool base_game = cf_find_file_location("ships.tbl", CF_TYPE_TABLES) &&
	cf_find_file_location("ChoosePilot.pcx", CF_TYPE_INTERFACE);

	if (base_game) {
		// mainhall.tbl is only in FS2, so use that to differentiate between FS1 and FS2
#ifdef MAKE_FS1
		ready = cf_find_file_location("mainhall.tbl", CF_TYPE_TABLES) == false;
#else
		ready = cf_find_file_location("mainhall.tbl", CF_TYPE_TABLES);
#endif
	}

	if ( !ready && run_locator ) {
		// if there is no installation path set then try to find one, otherwise
		// we should leave it to the user to sort out the proper path
		auto extras = os_config_read_string(nullptr, "ExtrasPath");

		if ( !extras || !SDL_strlen(extras) ) {
			if (data_locator()) {
				cfile_refresh();

				return launcher_ready_to_play(false);
			}
		}
	}

	return ready;
}
