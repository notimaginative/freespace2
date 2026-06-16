/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Mission/MissionLoad.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * C source module for mission loading
 *
 * $Log$
 * Revision 1.3  2005/03/31 00:04:25  taylor
 * fix directory separator (thanks Pierre\!)
 *
 * Revision 1.2  2002/06/09 04:41:22  relnev
 * added copyright header
 *
 * Revision 1.1.1.1  2002/05/03 03:28:09  root
 * Initial import.
 *
 * 
 * 5     7/20/99 1:49p Dave
 * Peter Drake build. Fixed some release build warnings.
 * 
 * 4     10/13/98 9:28a Dave
 * Started neatening up freespace.h. Many variables renamed and
 * reorganized. Added AlphaColors.[h,cpp]
 * 
 * 3     10/07/98 6:27p Dave
 * Globalized mission and campaign file extensions. Removed Silent Threat
 * special code. Moved \cache \players and \multidata into the \data
 * directory.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 102   5/19/98 1:19p Allender
 * new low level reliable socket reading code.  Make all missions/campaign
 * load/save to data missions folder (i.e. we are rid of the player
 * missions folder)
 * 
 * 101   5/10/98 10:05p Allender
 * only show cutscenes which have been seen before.  Made Fred able to
 * write missions anywhere, defaulting to player misison folder, not data
 * mission folder.  Fix FreeSpace code to properly read missions from
 * correct locations
 * 
 * 100   4/30/98 4:53p John
 * Restructured and cleaned up cfile code.  Added capability to read off
 * of CD-ROM drive and out of multiple pack files.
 * 
 * 99    2/23/98 6:55p Lawrance
 * Rip out obsolete code.
 * 
 * 98    2/23/98 8:53a John
 * String externalization
 * 
 * 97    1/19/98 9:37p Allender
 * Great Compiler Warning Purge of Jan, 1998.  Used pragma's in a couple
 * of places since I was unsure of what to do with code.
 * 
 * 96    1/17/98 8:49p Hoffoss
 * Fixed mission_load() calls to handle failure correctly.
 * 
 * 95    12/28/97 1:34p John
 * Fixed yet another mission filename bug
 * 
 * 94    12/28/97 12:42p John
 * Put in support for reading archive files; Made missionload use the
 * cf_get_file_list function.   Moved demos directory out of data tree.
 * 
 * 93    12/27/97 2:39p John
 * Took out the outdated ui_getfilelist functions.  Made the mission load
 * screen use cf_get_filelist instead.  Fixed a bug in mission load that
 * crashed the program if there are no missions available.
 * 
 * 92    12/23/97 12:00p Allender
 * change write_pilot_file to *not* take is_single as a default parameter.
 * causing multiplayer pilots to get written to the single player folder
 * 
 * 91    11/11/97 4:57p Dave
 * Put in support for single vs. multiplayer pilots. Began work on
 * multiplayer campaign saving. Put in initial player select screen
 * 
 * 90    10/31/97 11:27a John
 * appended path for j:\tmp missions
 * 
 * 89    10/31/97 11:19a John
 * added filter catagory for j:\tmp\*.fsm
 * 
 * 88    10/12/97 5:22p Lawrance
 * have ESC back out of mission load screen
 * 
 * 87    9/18/97 10:19p Lawrance
 * Add a mission campaign filter to the load screen
 * 
 * 86    9/16/97 2:41p Allender
 * beginning of code to change way player starts are handled.  Reordered
 * some code when missions loading since player ship is now created at
 * mission load time instead of before misison load
 * 
 * 85    8/25/97 5:47p Mike
 * Increase number of missions supported in mission load list (outside
 * campaign) to 256 and SDL_assert() if there are more than 256.
 * 
 * 84    8/20/97 5:19p Hoffoss
 * Fixed bug where creating a new pilot causes the mission load mission
 * list box to be empty.
 * 
 * 83    7/28/97 10:53a Lawrance
 * initialize a timestamp
 * 
 * 82    7/17/97 4:25p John
 * First, broken, stage of changing config stuff
 * 
 * 81    7/05/97 1:47p Lawrance
 * write pilot file when a mission is loaded
 * 
 * 80    6/26/97 5:53p Lawrance
 * save recently played missions, allow player to choose from list
 * 
 * 79    6/12/97 12:39p John
 * made ui use freespace colors
 * 
 * 78    6/12/97 11:35a John
 * more menu backgrounds
 * 
 * 77    5/14/97 1:33p Allender
 * remmoved extern declaration
 * 
 * 76    5/12/97 4:59p Allender
 * move the rest of the mission initialization functions into
 * game_level_init().  All mission loading now going through this
 * fucntion.
 * 
 * 75    5/12/97 3:21p Allender
 * re-ordered mission load code into single function in Freespace.
 * Simulation part now runs as seperate thread
 * 
 * 74    5/12/97 12:27p John
 * Restructured Graphics Library to add support for multiple renderers.
 * 
 * 73    4/28/97 5:43p Lawrance
 * allow hotkey assignment screen to work from ship selection
 * 
 * 72    4/25/97 11:31a Allender
 * Campaign state now saved in campaign save file in player directory.
 * Made some global variables follow naming convention.  Solidified
 * continuing campaigns based on new structure
 * 
 * 71    4/23/97 4:46p Allender
 * remove unused code
 * 
 * 70    4/23/97 3:21p Allender
 * more campaign stuff -- mission branching through campaign file now
 * works!!!!
 * 
 * 69    4/22/97 10:44a Allender
 * more campaign stuff.  Info about multiple campaigns now stored in
 * player file -- not saving some player information in save games.
 * 
 * 68    4/18/97 9:59a Allender
 * more campaign stuff.  All campaign related varaibles now stored in
 * campaign structure
 * 
 * 67    4/17/97 9:02p Allender
 * new campaign stuff.  all campaign related material stored in external
 * file.  Continuing campaign won't work at this time
 * 
 * 66    4/15/97 4:37p Lawrance
 * removed unused variables
 *
*/

#include "missionload.h"
#include "missiongoals.h"
#include "missionparse.h"    
#include "missionshipchoice.h"
#include "missionlog.h"
#include "missionmessage.h"
#include "cfile.h"
#include "osapi.h"
#include "vecmat.h"
#include "player.h"
#include "object.h"
#include "ship.h"
#include "ailocal.h"
#include "managepilot.h"
#include "hud.h"
#include "freespace.h"
#include "key.h"
#include "2d.h"
#include "line.h"
#include "timer.h"
#include "math.h"
#include "linklist.h"
#include "mouse.h"
#include "weapon.h"
#include "gamesequence.h"
#include "ui.h"
#include "sexp.h"
#include "missionhotkey.h"
#include "missioncampaign.h"
#include "cfilesystem.h"
#include "alphacolors.h"


extern mission The_mission;  // need to send this info to the briefing

// -----------------------------------------------
// For recording most recent missions played
// -----------------------------------------------
char	Recent_missions[MAX_RECENT_MISSIONS][MAX_FILENAME_LEN];
int	Num_recent_missions;


// -----------------------------------------------------
// ml_update_recent_missions()
//
//	Update the Recent_missions[][] array
//
void ml_update_recent_missions(char *filename)
{
	char	tmp[MAX_RECENT_MISSIONS][MAX_FILENAME_LEN], *p;
	int	i,j;
	

	for ( i = 0; i < Num_recent_missions; i++ ) {
		SDL_strlcpy( tmp[i], Recent_missions[i], SDL_arraysize(tmp[0]) );
	}

	// get a pointer to just the basename of the filename (including extension)
	p = strrchr(filename, DIR_SEPARATOR_CHAR);
	if ( p == NULL ) {
		p = filename;
	} else {
		p++;
	}

	SDL_assert(SDL_strlen(p) < MAX_FILENAME_LEN);
	SDL_strlcpy( Recent_missions[0], p, SDL_arraysize(Recent_missions[0]) );

	j = 1;
	for ( i = 0; i < Num_recent_missions; i++ ) {
		if ( SDL_strcasecmp(Recent_missions[0], tmp[i]) ) {
			SDL_strlcpy(Recent_missions[j++], tmp[i], SDL_arraysize(Recent_missions[0]));
			if ( j >= MAX_RECENT_MISSIONS ) {
				break;
			}
		}
	}

	Num_recent_missions = j;
	SDL_assert(Num_recent_missions <= MAX_RECENT_MISSIONS);
}

// Mission_load takes no parameters.
// It expects the following global variables to be set correctly:
//   Game_current_mission_filename

// returns -1 if failed, 0 if successful
int mission_load()
{
	char filename[128], *ext;	

	SDL_Log("MISSION LOAD: '%s'", Game_current_mission_filename);

	SDL_strlcpy(filename, Game_current_mission_filename, SDL_arraysize(filename));
	ext = SDL_strchr(filename, '.');
	if (ext) {
		mprintf(( "Hmmm... Extension passed to mission_load...\n" ));
		*ext = 0;				// remove any extension!
	}

	SDL_strlcat(filename, FS_MISSION_FILE_EXT, SDL_arraysize(filename));

	// does the magical mission parsing
	// creates all objects, except for the player object
	// save the player object later since the player may get
	// to choose the type of ship that he is to fly
	// return value of 0 indicates success, other is failure.

	if ( parse_main(filename) )
		return -1;

	if (Select_default_ship) {
		if ( create_default_player_ship() ) {
			Int3();
		}
	}

	ml_update_recent_missions(Game_current_mission_filename);  // update recently played missions list
	write_pilot_file();
	return 0;
}
