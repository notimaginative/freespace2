/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/GlobalIncs/version.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 *
 * $Log$
 * Revision 1.3  2002/06/09 04:41:17  relnev
 * added copyright header
 *
 * Revision 1.2  2002/05/07 03:16:45  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:09  root
 * Initial import.
 *
 * 
 * 4     8/09/99 2:21p Andsager
 * Fix patching from multiplayer direct to launcher update tab.
 * 
 * 3     8/06/99 3:32p Andsager
 * Handle update when no registry is set
 * 
 * 2     5/19/99 4:07p Dave
 * Moved versioning code into a nice isolated common place. Fixed up
 * updating code on the pxo screen. Fixed several stub problems.
 * 
 * 1     5/18/99 4:28p Dave
 * 
 * $NoKeywords: $
 */

#include <stdio.h>
#include <string.h>
#include "version.h"
#include "osregistry.h"
#include "pstypes.h"
#include "cfile.h"

// ----------------------------------------------------------------------------------------------------------------
// VERSION DEFINES/VARS
//

// Defines
#define VER(major, minor, build) (100*100*major+100*minor+build)
#define MAX_LINE_LENGTH 512


// ----------------------------------------------------------------------------------------------------------------
// VERSION FUNCTIONS
//

// compare version against the passed version file
// returns -1 on error 
// 0 if we are an earlier version
// 1 if same version
// 2 if higher version
// fills in user version and latest version values if non-NULL
int version_compare(const char *filename, int *u_major, int *u_minor, int *u_build, int *l_major, int *l_minor, int *l_build)
{	
	int usr_major, usr_minor, usr_build;
	int latest_major, latest_minor, latest_build;

	// open file and try backup, if needed
	CFILE *f = cfopen(filename, "rt", CF_TYPE_DATA);
	if (f == NULL) {
		return -1;		
	}

	// grab the last line in file which isn't empty and isn't a comment
	char buffer[MAX_LINE_LENGTH+1], verbuffer[MAX_LINE_LENGTH+1];

	SDL_strlcpy(verbuffer, "", SDL_arraysize(verbuffer));
	SDL_strlcpy(buffer, "", SDL_arraysize(buffer));
	while ( !cfeof(f) ) {
		// Read the line into a temporary buffer
		if ( cfgets(buffer, MAX_LINE_LENGTH, f) == NULL ) {
			break;
		}

		// take the \n off the end of it
		if (SDL_strlen(buffer)>0 && buffer[SDL_strlen(buffer) - 1] == '\n')
			buffer[SDL_strlen(buffer) - 1] = 0;

		// If the line is empty, go get another one
		if (SDL_strlen(buffer) == 0) continue;

		// If the line is a comment, go get another one
		if (buffer[0] == VERSION_FILE_COMMENT_CHAR) continue;

		// Line is a good one, so save it...
		SDL_strlcpy(verbuffer, buffer, SDL_arraysize(verbuffer));
	}
	cfclose(f);

	// Make sure a version line was found
	if (SDL_strlen(verbuffer) == 0) {
		// MessageBox(XSTR("Couldn't parse Version file!", 1205), XSTR("Error!", 1185), MB_OK|MB_ICONERROR);
		return -1;
	}

	// Get the most up to date Version number
	latest_major = 0;
	latest_minor = 0;
	latest_build = 0;

	if (sscanf(verbuffer, "%i %i %i", &latest_major, &latest_minor, &latest_build) != 3) {
		// MessageBox(XSTR("Couldn't parse Version file!", 1205), XSTR("Error!", 1185), MB_OK|MB_ICONERROR);
		return -1;
	}

	// retrieve the user's current version
	usr_major = os_config_read_uint("Version", "Major", 0);
	usr_minor = os_config_read_uint("Version", "Minor", 0);
	usr_build = os_config_read_uint("Version", "Build", 0);
	
	// Make sure the user's Version was found!
	if ( VER(usr_major, usr_minor, usr_build) == 0 ) {
		// MessageBox(XSTR("The Freespace 2 Auto-Update program could not find your current game Version in the system registry.\n\nThis should be corrected by starting up the game, exiting the game, and then running the Auto-Update program.", 1206), XSTR("Unable to Determine User's Version", 1207), MB_OK|MB_ICONERROR);
		return NO_VERSION_IN_REGISTRY;
	}	

	// stuff outgoing values
	if(u_major != NULL){
		*u_major = usr_major;
	}
	if(u_minor != NULL){
		*u_minor = usr_minor;
	}
	if(u_build != NULL){
		*u_build = usr_build;
	}
	if(l_major != NULL){
		*l_major = latest_major;
	}
	if(l_minor != NULL){
		*l_minor = latest_minor;
	}
	if(l_build != NULL){
		*l_build = latest_build;
	}

	// check to see if the user's version is up to date
	if (VER(usr_major, usr_minor, usr_build) < VER(latest_major, latest_minor, latest_build)) {		
		return 0;
	}

	// same version
	return 1;
}

/// Return short version string with major.minor number
const char *version_get_string(char *str, size_t str_len)
{
	static char version_string[20] = { 0 };

	if ( !version_string[0] ) {
		SDL_snprintf(version_string, SDL_arraysize(version_string), "%d.%02d", FS_VERSION_MAJOR, FS_VERSION_MINOR);
	}

	if (str && str_len) {
		SDL_strlcpy(str, version_string, str_len);
	}

	return version_string;
}

/// Return full version string formatted for in-game UI display, depending on build
const char *version_get_string_full(char *str, size_t str_len)
{
	static char version_string[100] = { 0 };

	if ( !version_string[0] ) {
#ifdef FS1_DEMO
		SDL_snprintf(version_string, SDL_arraysize(version_string), "Dv%d.%02d", FS_VERSION_MAJOR, FS_VERSION_MINOR);
#else
		if ( FS_VERSION_BUILD == 0 ) {
			SDL_snprintf(version_string, SDL_arraysize(version_string), "v%d.%02d", FS_VERSION_MAJOR, FS_VERSION_MINOR);
		} else {
			SDL_snprintf(version_string, SDL_arraysize(version_string), "v%d.%02d.%d", FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_VERSION_BUILD );
		}
#endif

#ifdef GIT_INFO
#ifdef GIT_TAG
		SDL_strlcat(version_string, " " GIT_COMMIT_DATE ":" GIT_TAG, SDL_arraysize(version_string));
#else
		SDL_strlcat(version_string, " " GIT_COMMIT_DATE "~" GIT_BRANCH ":" GIT_COMMIT_HASH,
					SDL_arraysize(version_string));
#endif
#endif

#if defined (FS2_DEMO)
		SDL_strlcat(version_string, " D", SDL_arraysize(version_string));
#elif defined (OEM_BUILD)
		SDL_strlcat(version_string, " (OEM)", SDL_arraysize(version_string));
#endif
	}

	if (str && str_len) {
		SDL_strlcpy(str, version_string, str_len);
	}

	return version_string;
}
