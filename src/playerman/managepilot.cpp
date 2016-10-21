/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Playerman/ManagePilot.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * ManagePilot.cpp has code to load and save pilot files, and to select and 
 * manage the pilot
 *
 * $Log$
 * Revision 1.6  2004/07/04 11:41:24  taylor
 * warning fixes, cross platform pilot compat with OSX-Linux-Windows
 *
 * Revision 1.5  2003/05/25 02:30:43  taylor
 * Freespace 1 support
 *
 * Revision 1.4  2002/06/09 04:41:25  relnev
 * added copyright header
 *
 * Revision 1.3  2002/06/09 03:16:05  relnev
 * added _splitpath.
 *
 * removed unneeded asm, old sdl 2d setup.
 *
 * fixed crash caused by opengl_get_region.
 *
 * Revision 1.2  2002/05/07 03:16:50  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:11  root
 * Initial import.
 *
 * 
 * 41    9/13/99 4:52p Dave
 * RESPAWN FIX
 * 
 * 40    9/01/99 10:09a Dave
 * Pirate bob.
 * 
 * 39    8/26/99 8:49p Jefff
 * old player file compatibility with new medal stuff
 * 
 * 38    8/22/99 5:53p Dave
 * Scoring fixes. Added self destruct key. Put callsigns in the logfile
 * instead of ship designations for multiplayer players.
 * 
 * 37    8/16/99 4:06p Dave
 * Big honking checkin.
 * 
 * 36    8/11/99 11:36a Jefff
 * added compatibility w/ fs2 demo plr version
 * 
 * 35    8/10/99 3:46p Jefff
 * changes for Intelligence section of new tech room
 * 
 * 34    8/04/99 11:38p Andsager
 * make new pilot detail level match registry info.
 * 
 * 33    8/02/99 9:55p Dave
 * Hardcode a nice hud color config for the demo.
 * 
 * 32    8/02/99 9:13p Dave
 * Added popup tips.
 * 
 * 31    8/01/99 12:39p Dave
 * Added HUD contrast control key (for nebula).
 * 
 * 30    7/30/99 10:31p Dave
 * Added comm menu to the configurable hud files.
 * 
 * 29    7/29/99 10:47p Dave
 * Standardized D3D fogging using vertex fog. Shook out Savage 4 bugs.
 * 
 * 28    7/29/99 12:05a Dave
 * Nebula speed optimizations.
 * 
 * 27    7/24/99 1:54p Dave
 * Hud text flash gauge. Reworked dead popup to use 4 buttons in red-alert
 * missions.
 * 
 * 26    6/22/99 7:03p Dave
 * New detail options screen.
 * 
 * 25    6/16/99 4:06p Dave
 * New pilot info popup. Added new draw-bitmap-as-poly function.
 * 
 * 24    6/11/99 11:13a Dave
 * last minute changes before press tour build.
 * 
 * 23    6/08/99 1:14a Dave
 * Multi colored hud test.
 * 
 * 22    5/03/99 8:33p Dave
 * New version of multi host options screen.
 * 
 * 21    3/24/99 4:05p Dave
 * Put in support for assigning the player to a specific squadron with a
 * specific logo. Preliminary work for doing pos/orient checksumming in
 * multiplayer to reduce bandwidth.
 * 
 * 20    1/30/99 1:29a Dave
 * Fixed nebula thumbnail problem. Full support for 1024x768 choose pilot
 * screen.  Fixed beam weapon death messages.
 * 
 * 19    1/29/99 2:08a Dave
 * Fixed beam weapon collisions with players. Reduced size of scoring
 * struct for multiplayer. Disabled PXO.
 * 
 * 18    1/21/99 2:06p Dave
 * Final checkin for multiplayer testing.
 * 
 * 17    1/15/99 2:49p Dave
 * Fixed creation of pilots.
 * 
 * 16    1/14/99 6:06p Dave
 * 100% full squad logo support for single player and multiplayer.
 * 
 * 15    1/12/99 3:15a Dave
 * Barracks screen support for selecting squad logos. We need real artwork
 * :)
 * 
 * 14    1/06/99 2:24p Dave
 * Stubs and release build fixes.
 * 
 * 13    12/14/98 12:13p Dave
 * Spiffed up xfer system a bit. Put in support for squad logo file xfer.
 * Need to test now.
 * 
 * 12    11/20/98 11:16a Dave
 * Fixed up IPX support a bit. Making sure that switching modes and
 * loading/saving pilot files maintains proper state.
 * 
 * 11    11/19/98 4:51p Dave
 * Ignore multiplayer protocol settings in the pilot file for now.
 * 
 * 10    11/19/98 4:19p Dave
 * Put IPX sockets back in psnet. Consolidated all multiplayer config
 * files into one.
 * 
 * 9     10/13/98 2:47p Andsager
 * Remove reference to Tech_shivan_species_avail
 * 
 * 8     10/13/98 9:29a Dave
 * Started neatening up freespace.h. Many variables renamed and
 * reorganized. Added AlphaColors.[h,cpp]
 * 
 * 7     10/12/98 9:30a Andsager
 * Clean up barracks.cpp.  Remove unneeded ".h" files from ManagePilot
 * 
 * 6     10/09/98 5:17p Andsager
 * move barracks screen into barracks.cpp
 * 
 * 5     10/09/98 2:57p Dave
 * Starting splitting up OS stuff.
 * 
 * 4     10/08/98 9:19a Andsager
 * Clean up pilot player read and write, starting with new version.
 * 
 * 3     10/07/98 6:27p Dave
 * Globalized mission and campaign file extensions. Removed Silent Threat
 * special code. Moved \cache \players and \multidata into the \data
 * directory.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:50a Dave
 * 
 * 279   9/21/98 10:02p Dave
 * Last minute changes to techroom weapon/ship/species stuff.
 * 
 * 278   9/08/98 12:10p Andsager
 * Fixed a bug with saving ship and weapon info techroom flags.
 * 
 * 277   9/01/98 4:25p Dave
 * Put in total (I think) backwards compatibility between mission disk
 * freespace and non mission disk freespace, including pilot files and
 * campaign savefiles.
 * 
 * 276   6/09/98 10:31a Hoffoss
 * Created index numbers for all xstr() references.  Any new xstr() stuff
 * added from here on out should be added to the end if the list.  The
 * current list count can be found in FreeSpace.cpp (search for
 * XSTR_SIZE).
 * 
 * 275   6/05/98 9:49a Lawrance
 * OEM changes
 * 
 * 274   6/01/98 11:43a John
 * JAS & MK:  Classified all strings for localization.
 * 
 * 273   5/26/98 11:53a Allender
 * fix multiplayer problems and sexpression crash
 * 
 * 272   5/24/98 2:46p Lawrance
 * Fix bug where skill level would be reset to default when switching
 * between pilots
 * 
 * 271   5/23/98 4:02p Allender
 * version change
 * 
 * 270   5/23/98 2:41p Mike
 * Make Easy the default skill level and prevent old pilot's skill level
 * from carrying into new pilot.
 *
*/

#include <errno.h>
#include "managepilot.h"
#include "2d.h"
#include "freespace.h"
#include "hudsquadmsg.h"
#include "sound.h"
#include "multi.h"
#include "eventmusic.h"
#include "audiostr.h"
#include "osregistry.h"
#include "font.h"
#include "playermenu.h"
#include "missionshipchoice.h"
#include "hudconfig.h"
#include "popup.h"
#include "redalert.h"
#include "techmenu.h"
#include "joy.h"
#include "mouse.h"
#include "cutscenes.h"
#include "bmpman.h"
#include "key.h"

// update this when altering data that is read/written to .PLR file
#if defined(FS2_DEMO)
	#define CURRENT_PLAYER_FILE_VERSION				136		// 1.10
	#define PREVIOUS_PLAYER_FILE_VERSION			135		// 1.00

	#define LOWEST_COMPATIBLE_PLAYER_FILE_VERSION	PREVIOUS_PLAYER_FILE_VERSION
#elif defined(FS1_DEMO)
	#define CURRENT_PLAYER_FILE_VERSION				94		// 1.20
	#define PREVIOUS_PLAYER_FILE_VERSION			86		// 1.00

	#define LOWEST_COMPATIBLE_PLAYER_FILE_VERSION	PREVIOUS_PLAYER_FILE_VERSION
#elif defined(MAKE_FS1)
	#define CURRENT_PLAYER_FILE_VERSION				100		// 1.04
	#define PREVIOUS_PLAYER_FILE_VERSION			99		// 1.00

	#define LOWEST_COMPATIBLE_PLAYER_FILE_VERSION	PREVIOUS_PLAYER_FILE_VERSION
#else
	#define CURRENT_PLAYER_FILE_VERSION				140		// 1.00

	#define LOWEST_COMPATIBLE_PLAYER_FILE_VERSION	CURRENT_PLAYER_FILE_VERSION			// demo plr files should work in final
#endif

// keep track of pilot file changes here 
// version 2	: Added squad logo filename
// version 3	: Changed size of scoring struct. use ushort instead of ints for storing alltime kills by ship type
// version 4/5 : Added squadron name field
// version 6	: changed max length on a multiplayer options field
// version 130	: changed size of hud config struct
// version 133 : misc changes. new hud gauge
// version 134 : added HUD contrast toggle key
// version 135 : added tips flag  (THIS IS THE DEMO VERSION - RETAIN COMPATIBILITY FROM HERE ON OUT)
// version 136 : added intelligence flags to tech room visibility data
// version 137 : 2 new HUD gauges. 
// version 138	: new multiplayer config
// version 139 : # medals increased - added compatibility with old plr file versions
// version 140 : ships table reordered. clear out old pilot files
// search for PLAYER INIT for new pilot initialization stuff. I _think_ its in the right spot for now
#define PLR_FILE_ID	0x46505346	// FPSF, unique signiture to identify a .PLR file (FreeSpace Player File)  // FPSF appears as FSPF in file.

#ifdef MAKE_FS1
#define PLR_MAX_SHIP_TYPES_OLD			75
#define PLR_MAX_WEAPON_TYPES_OLD		44

extern void hud_config_set_color(int color);
#endif

// Current content of a .PLR file
//

// Global variables
int Player_sel_mode;

// pilot pic image list stuff ( call pilot_load_pic_list() to make these valid )
char Pilot_images_arr[MAX_PILOT_IMAGES][MAX_FILENAME_LEN];
char *Pilot_image_names[MAX_PILOT_IMAGES];
int Num_pilot_images = 0;

// squad logo list stuff (call pilot_load_squad_pic_list() to make these valid )
char Pilot_squad_images_arr[MAX_PILOT_IMAGES][MAX_FILENAME_LEN];
char *Pilot_squad_image_names[MAX_PILOT_IMAGES];
int Num_pilot_squad_images = 0;

static uint Player_file_version;

// forward declarations
void read_detail_settings(CFILE *file, int Player_file_version);
void write_detail_settings(CFILE *file);
void read_stats_block(CFILE *file, int Player_file_version, scoring_struct *stats);
void write_stats_block(CFILE *file, scoring_struct *stats);
void read_multiplayer_options(player *p,CFILE *file);
void write_multiplayer_options(player *p,CFILE *file);

static SDL_Keycode keycode_translate_from(short keycode);
static short keycode_translate_to(SDL_Keycode keycode);

// internal function to delete a player file.  Called after a pilot is obsoleted, and when a pilot is deleted
// used in barracks and player_select
void delete_pilot_file( const char *pilot_name, int single )
{
	char filename[MAX_FILENAME_LEN];
	char basename[MAX_FILENAME_LEN];

	// get the player file.

	base_filename(pilot_name, basename, SDL_arraysize(basename));

	SDL_strlcpy( filename, basename, SDL_arraysize(filename) );
	SDL_strlcat(filename, NOX(".plr"), SDL_arraysize(filename) );
	if (Player_sel_mode == PLAYER_SELECT_MODE_SINGLE){
		cf_delete(filename, CF_TYPE_SINGLE_PLAYERS);
	} else {
		cf_delete(filename, CF_TYPE_MULTI_PLAYERS);
	}

	// we must try and delete the campaign save files for a pilot as well.
	mission_campaign_delete_all_savefiles( basename, !single );
}

// check if a pilot file is valid or not (i.e. is usable, not out of date, etc)
// used in barracks and player_select
int verify_pilot_file(const char *filename, int single, int *rank)
{
	CFILE	*file;
	uint id, file_version;
	int type;

	filename = cf_add_ext(filename, NOX(".plr"));
	
	if (single){
		file = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS);
	} else {
		file = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_MULTI_PLAYERS);
	}

	if (!file){
		return -1;
	}

	id = cfread_uint(file);
	if (id != PLR_FILE_ID) {
		nprintf(("Warning", "Player file has invalid signature\n"));
		cfclose(file);
		delete_pilot_file( filename, single );
		return -1;
	}

	// check for compatibility here
	file_version = cfread_uint(file);
/*	if (file_version < INITIAL_RELEASE_FILE_VERSION) { */
//	if (file_version != CURRENT_PLAYER_FILE_VERSION) {
	if (file_version < LOWEST_COMPATIBLE_PLAYER_FILE_VERSION) {
		nprintf(("Warning", "WARNING => Player file is outdated and not compatible...\n"));
		cfclose(file);
		delete_pilot_file( filename, single );
		return -1;
	}

	type = !cfread_ubyte(file);
	if (rank){
		*rank = 0;	
	}
		
	if (rank){
		*rank = cfread_int(file);
	} else {
		cfread_int(file);
	}

	cfclose(file);
	if (type != single){
		return -1;
	}

	return 0;
}

void pilot_write_techroom_data(CFILE *file)
{
	int idx;		
	ubyte out;

	// write the ship and weapon count
	cfwrite_int(Num_ship_types, file);
	cfwrite_int(Num_weapon_types, file);
#ifndef MAKE_FS1
	cfwrite_int(Intel_info_size, file);
#endif

	// write all ship flags out
	for (idx=0; idx<Num_ship_types; idx++) {
		out = (Ship_info[idx].flags & SIF_IN_TECH_DATABASE) ? (ubyte)1 : (ubyte)0;		
		cfwrite_ubyte(out, file);				
	}

	// write all weapon types out
	for (idx=0; idx<Num_weapon_types; idx++) {
		out = (Weapon_info[idx].wi_flags & WIF_IN_TECH_DATABASE) ? (ubyte)1 : (ubyte)0;
		cfwrite_ubyte(out, file);
	}	

#ifndef MAKE_FS1
	// write all intel entry flags out
	for (idx=0; idx<Intel_info_size; idx++) {
		cfwrite_ubyte((ubyte)Intel_info[idx].in_tech_db, file);
	}
#else
	// whether shivans are visible or not
	int shivans = Intel_info[2].in_tech_db ? 1 : 0;
	cfwrite_int(shivans, file);
#endif
}


#ifdef MAKE_FS1
void pilot_read_techroom_data(CFILE *file)
{
	int idx;
	int ship_count, weapon_count, shivans;
	ubyte in;

	if (Player_file_version < 100) {
		int vflags = 0;

		// first set of ships visible
		vflags = cfread_int(file);

		for (idx = 0; idx < 32; idx++) {
			if ( (vflags & (1<<idx)) && (idx < Num_ship_types) ) {
				Ship_info[idx].flags |= SIF_IN_TECH_DATABASE | SIF_IN_TECH_DATABASE_M;
			} else if (idx < Num_ship_types) {
				Ship_info[idx].flags &= ~SIF_IN_TECH_DATABASE;
			}
		}

		// second set of ships visible
		vflags = cfread_int(file);

		for (idx = 0; idx < 32; idx++) {
			if ( (vflags & (1<<idx)) && ((idx+32) < Num_ship_types) ) {
				Ship_info[idx+32].flags |= SIF_IN_TECH_DATABASE | SIF_IN_TECH_DATABASE_M;
			} else if ((idx+32) < Num_ship_types) {
				Ship_info[idx+32].flags &= ~SIF_IN_TECH_DATABASE;
			}
		}

		// last set of ships visible
		vflags = cfread_int(file);

		for (idx = 0; idx < 32; idx++) {
			if ( (vflags & (1<<idx)) && ((idx+64) < Num_ship_types) ) {
				Ship_info[idx+64].flags |= SIF_IN_TECH_DATABASE | SIF_IN_TECH_DATABASE_M;
			} else if ((idx+64) < Num_ship_types) {
				Ship_info[idx+64].flags &= ~SIF_IN_TECH_DATABASE;
			}
		}

		// first set of weapons visible
		vflags = cfread_int(file);

		for (idx = 0; idx < 32; idx++) {
			if ( (vflags & (1<<idx)) && (idx < Num_weapon_types) ) {
				Weapon_info[idx].wi_flags |= WIF_IN_TECH_DATABASE;
			} else if (idx < Num_ship_types) {
				Weapon_info[idx].wi_flags &= ~WIF_IN_TECH_DATABASE;
			}
		}

		// last set of weapons visible
		vflags = cfread_int(file);

		for (idx = 0; idx < 32; idx++) {
			if ( (vflags & (1<<idx)) && ((idx+32) < Num_weapon_types) ) {
				Weapon_info[idx+32].wi_flags |= WIF_IN_TECH_DATABASE;
			} else if ((idx+32) < Num_ship_types) {
				Weapon_info[idx+32].wi_flags &= ~WIF_IN_TECH_DATABASE;
			}
		}

		// shivans visible?
		shivans = cfread_int(file);

		if (shivans) {
			Intel_info[2].in_tech_db = 1;
		} else {
			Intel_info[2].in_tech_db = 0;
		}
	} else {
		// read in ship and weapon counts
		ship_count = cfread_int(file);
		weapon_count = cfread_int(file);
		SDL_assert(ship_count <= MAX_SHIP_TYPES);
		SDL_assert(weapon_count <= MAX_WEAPON_TYPES);

		// read all ships in
		for (idx=0; idx<ship_count; idx++) {
			in = cfread_ubyte(file);
			if (in) {
				Ship_info[idx].flags |= SIF_IN_TECH_DATABASE | SIF_IN_TECH_DATABASE_M;
			} else {
				Ship_info[idx].flags &= ~SIF_IN_TECH_DATABASE;
			}
		}

		// read all weapons in
		for (idx=0; idx<weapon_count; idx++) {
			in = cfread_ubyte(file);
			if (in) {
				Weapon_info[idx].wi_flags |= WIF_IN_TECH_DATABASE;
			} else {
				Weapon_info[idx].wi_flags &= ~WIF_IN_TECH_DATABASE;
			}
		}

		// shivans visible?
		shivans = cfread_int(file);

		if (shivans) {
			Intel_info[2].in_tech_db = 1;
		} else {
			Intel_info[2].in_tech_db = 0;
		}
	}
}
#else
void pilot_read_techroom_data(CFILE *file)
{
	int idx;
	int ship_count, weapon_count, intel_count;
	ubyte in;

	// read in ship and weapon counts
	ship_count = cfread_int(file);
	weapon_count = cfread_int(file);
	SDL_assert(ship_count <= MAX_SHIP_TYPES);
	SDL_assert(weapon_count <= MAX_WEAPON_TYPES);

	// maintain compatibility w/ demo version
	if (Player_file_version < 136) {
		// skip over all this data, because the lack of tech room in the demo
		// left this all hosed in the demo .plr files
		// this will all get initialized as if this fella was a new pilot
		for (idx=0; idx<ship_count+weapon_count; idx++) {
			cfread_ubyte(file);
		}

	} else {

		intel_count = cfread_int(file);
		SDL_assert(intel_count <= MAX_INTEL_ENTRIES);

		// read all ships in
		for (idx=0; idx<ship_count; idx++) {
			in = cfread_ubyte(file);
			if (in) {
				Ship_info[idx].flags |= SIF_IN_TECH_DATABASE | SIF_IN_TECH_DATABASE_M;
			} else {
				Ship_info[idx].flags &= ~SIF_IN_TECH_DATABASE;
			}
		}

		// read all weapons in
		for (idx=0; idx<weapon_count; idx++) {
			in = cfread_ubyte(file);
			if (in) {
				Weapon_info[idx].wi_flags |= WIF_IN_TECH_DATABASE;
			} else {
				Weapon_info[idx].wi_flags &= ~WIF_IN_TECH_DATABASE;
			}
		}

		// read all intel entries in
		for (idx=0; idx<intel_count; idx++) {
			in = cfread_ubyte(file);
			if (in) {
				Intel_info[idx].in_tech_db = 1;
			} else {
				Intel_info[idx].in_tech_db = 0;
			}
		}
	}
}
#endif

// write out the player ship selection
void pilot_write_loadout(CFILE *file)
{
	int i, j;
	wss_unit *slot;	

	cfwrite_string_len(Player_loadout.filename, file);
	cfwrite_string_len(Player_loadout.last_modified, file);

	// write ship and weapon counts
	cfwrite_int(Num_ship_types, file);
	cfwrite_int(Num_weapon_types, file);

	// write ship pool
	for ( i = 0; i < Num_ship_types; i++ ) {
		cfwrite_int(Player_loadout.ship_pool[i], file);
	}

	// write weapons pool
	for ( i = 0; i < Num_weapon_types; i++ ) {
		cfwrite_int(Player_loadout.weapon_pool[i], file);
	}

	// write ship loadouts
	for ( i = 0; i < MAX_WSS_SLOTS; i++ ) {
		slot = &Player_loadout.unit_data[i];
		cfwrite_int(slot->ship_class, file);
		for ( j = 0; j < MAX_WL_WEAPONS; j++ ) {
			cfwrite_int(slot->wep[j], file);
			cfwrite_int(slot->wep_count[j], file);
		}
	}
}

// read in the ship selection for the pilot
void pilot_read_loadout(CFILE *file)
{
	int i, j;
	wss_unit *slot;
	int ship_count, weapon_count;

	memset(Player_loadout.filename, 0, MAX_FILENAME_LEN);
	cfread_string_len(Player_loadout.filename, MAX_FILENAME_LEN, file);

	memset(Player_loadout.last_modified, 0, DATE_TIME_LENGTH);	
	cfread_string_len(Player_loadout.last_modified, DATE_TIME_LENGTH, file);	

	// read in ship and weapon counts
#ifndef MAKE_FS1
	ship_count = cfread_int(file);
	weapon_count = cfread_int(file);
#else
	if (Player_file_version < 100) {
		ship_count = PLR_MAX_SHIP_TYPES_OLD;
		weapon_count = PLR_MAX_WEAPON_TYPES_OLD;
	} else {
		ship_count = cfread_int(file);
		weapon_count = cfread_int(file);
	}
#endif

	SDL_assert(ship_count <= MAX_SHIP_TYPES);
	SDL_assert(weapon_count <= MAX_WEAPON_TYPES);

	// read in ship pool
	for ( i = 0; i < ship_count; i++ ) {
		Player_loadout.ship_pool[i] = cfread_int(file);
	}

	// read in weapons pool
	for ( i = 0; i < weapon_count; i++ ) {
		Player_loadout.weapon_pool[i] = cfread_int(file);
	}

	// read in loadout info
	for ( i = 0; i < MAX_WSS_SLOTS; i++ ) {
		slot = &Player_loadout.unit_data[i];
		slot->ship_class = cfread_int(file);
		for ( j = 0; j < MAX_WL_WEAPONS; j++ ) {
			slot->wep[j] = cfread_int(file);
			slot->wep_count[j] = cfread_int(file);
		}
	}
}

// read_pilot_file()
//
// returns 0 - file read in correctly
//        -1 - .PLR file doesn't exist or file not compatible
//        >0 - errno from fopen error
// if single == 1, look for players in the single players directory, otherwise look in the 
// multiplayers directory
int read_pilot_file(const char *callsign, int single, player *p)
{
	ubyte num_ctrls;
	ubyte is_multi = 0;
	char filename[MAX_FILENAME_LEN], ship_name[NAME_LENGTH];
	CFILE	*file;
	uint id;
	int i, key_value;

	if (!p) {
		SDL_assert((Player_num >= 0) && (Player_num < MAX_PLAYERS));
		p = &Players[Player_num];
	}

	//sprintf(filename, "%-.8s.plr",Players[Player_num].callsign);
	SDL_assert(strlen(callsign) < MAX_FILENAME_LEN - 4);  // ensure we won't overrun the buffer
	SDL_strlcpy( filename, callsign, SDL_arraysize(filename) );
	SDL_strlcat( filename, NOX(".plr"), SDL_arraysize(filename) );

	// if we're a standalone server in multiplayer, just fill in some bogus values since we don't have a pilot file
	if ((Game_mode & GM_MULTIPLAYER) && (Game_mode & GM_STANDALONE_SERVER)) {
		memset(Player, 0, sizeof(player));
		SDL_strlcpy(Player->callsign, NOX("Standalone"), SDL_arraysize(Player->callsign));
		SDL_strlcpy(Player->short_callsign, NOX("Standalone"), SDL_arraysize(Player->short_callsign));
		return 0;
	}
	
	// see comments at the beginning of function
	if (single) {
		file = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS);
	} else {
		file = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_MULTI_PLAYERS);
	}

	if (!file) {
		return errno;
	}

	id = cfread_uint(file);
	if (id != PLR_FILE_ID) {
		Warning(LOCATION, "Player file has invalid signature");
		cfclose(file);
		return -1;
	}

	// check for compatibility here
	Player_file_version = cfread_uint(file);
	cf_set_version( file, Player_file_version );

	if (Player_file_version < LOWEST_COMPATIBLE_PLAYER_FILE_VERSION) {
		nprintf(("Warning", "WARNING => Player file is outdated and not compatible...\n"));
		cfclose(file);
		delete_pilot_file( filename, single );
		return -1;
	}

	// read in whether he's a multiplayer or not
	is_multi = cfread_ubyte(file);
	if (is_multi){
		p->flags |= PLAYER_FLAGS_IS_MULTI;
	} else {
		p->flags &= ~PLAYER_FLAGS_IS_MULTI;	// this takes care of unsetting any leftover bits from a (possibly) previously selected pilot
	}

	// read in rank.
	cfread_int(file);  

	// get player location
	p->on_bastion = cfread_ubyte(file);

#ifndef MAKE_FS1
	// tips?
	p->tips = cfread_int(file);
#endif

	// write out the image file name
	cfread_string_len(p->image_filename, MAX_FILENAME_LEN - 1, file);

	// write out the image file name
	p->insignia_texture = -1;
#ifndef MAKE_FS1
	cfread_string_len(p->squad_name, NAME_LENGTH, file);
	cfread_string_len(p->squad_filename, MAX_FILENAME_LEN - 1, file);
	player_set_squad_bitmap(p, p->squad_filename);
#endif

	// deal with campaign stuff.  The way we store the information in the file is to first store the
	// name of the current campaign that the player is playing.  Next we store the info regarding the campaigns
	// that the player is currently playing
	memset(p->current_campaign, 0, MAX_FILENAME_LEN);
	cfread_string_len(p->current_campaign, MAX_FILENAME_LEN, file);

	// read in the ship name for last ship flown by the player
	memset(ship_name, 0, NAME_LENGTH);
	cfread_string_len(ship_name, NAME_LENGTH, file);
	p->last_ship_flown_si_index = ship_info_lookup(ship_name);
	if ( p->last_ship_flown_si_index < 0 ) {
		nprintf(("Warning","WARNING => Ship class %s not located in Ship_info[] in player file\n",ship_name));
		p->last_ship_flown_si_index = ship_info_lookup(default_player_ship);
	}

	// set all the entries in the control config arrays to -1 (undefined)
	control_config_clear();

	// ---------------------------------------------
	// read in the keyboard/joystick mapping
	// ---------------------------------------------
	num_ctrls = cfread_ubyte(file);
	for (i=0; i<num_ctrls; i++) {
		key_value = cfread_short(file);
		// NOTE: next two lines are only here for transitioning from 255 to -1 as undefined key items
		if (key_value == 255)
			key_value = -1;

		Control_config[i].key_id = keycode_translate_from((short)key_value);

		key_value = cfread_short(file);
		// NOTE: next two lines are only here for transitioning from 255 to -1 as undefined key items
		if (key_value == 255)
			key_value = -1;

		Control_config[i].joy_id = (short) key_value;
	}
	
	HUD_config.show_flags = cfread_int(file);	
	HUD_config.show_flags2 = cfread_int(file);

	HUD_config.popup_flags = cfread_int(file);
	HUD_config.popup_flags2 = cfread_int(file);

	HUD_config.num_msg_window_lines = cfread_ubyte(file);			
	HUD_config.rp_flags = cfread_int(file);
	HUD_config.rp_dist =	cfread_int(file);

#ifdef MAKE_FS1
	HUD_config.main_color = cfread_int(file);
	HUD_color_alpha = cfread_int(file);

	if ( HUD_color_alpha < HUD_COLOR_ALPHA_USER_MIN ) {
		HUD_color_alpha = HUD_COLOR_ALPHA_DEFAULT;
	}

	hud_config_set_color(HUD_config.main_color);
#elif defined(FS2_DEMO)
	for(i=0; i<NUM_HUD_GAUGES; i++){
		cfread(&HUD_config.clr[i], sizeof(color), 1, file);
	}
#else
	// added 2 gauges with version 137
	if(Player_file_version < 137){
		for(i=0; i<NUM_HUD_GAUGES-2; i++){
			cfread(&HUD_config.clr[i], sizeof(color), 1, file);
		}

		// set the 2 new gauges to be radar color
		memcpy(&HUD_config.clr[NUM_HUD_GAUGES-2], &HUD_config.clr[HUD_RADAR], sizeof(color));
		memcpy(&HUD_config.clr[NUM_HUD_GAUGES-1], &HUD_config.clr[HUD_RADAR], sizeof(color));
	} else {
		for(i=0; i<NUM_HUD_GAUGES; i++){
			cfread(&HUD_config.clr[i], sizeof(color), 1, file);
		}
	}
#endif

	// read in the cutscenes which have been viewed
	Cutscenes_viewable = cfread_int(file);

	Master_sound_volume = cfread_float(file);
	Master_event_music_volume = cfread_float(file);
	Master_voice_volume = cfread_float(file);

	audiostream_set_volume_all(Master_voice_volume, ASF_VOICE);
	audiostream_set_volume_all(Master_event_music_volume, ASF_EVENTMUSIC);

	if ( Master_event_music_volume > 0.0f )	{
		Event_music_enabled = 1;
	} else {
		Event_music_enabled = 0;
	}

	read_detail_settings(file, Player_file_version);

	// restore list of most recently played missions
	Num_recent_missions = cfread_int( file );
	SDL_assert(Num_recent_missions <= MAX_RECENT_MISSIONS);
	for ( i = 0; i < Num_recent_missions; i++ ) {
		char *c;

		cfread_string_len( Recent_missions[i], MAX_FILENAME_LEN, file);
		// Remove the extension
		c = SDL_strchr(Recent_missions[i], '.');
		if (c)
			*c = 0;
	}
	
	// use this block of stats from now on
	read_stats_block(file, Player_file_version, &p->stats);	

   Game_skill_level = cfread_int(file);

	for (i=0; i<NUM_JOY_AXIS_ACTIONS; i++) {
		Axis_map_to[i] = cfread_int(file);
		Invert_axis[i] = cfread_int(file);
	}

	// restore some player flags
	Player[Player_num].save_flags = cfread_int(file);

	// restore the most recent ship selection	
	pilot_read_loadout(file);	

	// read in multiplayer options
	read_multiplayer_options(p,file);

	p->readyroom_listing_mode = cfread_int(file);
	Briefing_voice_enabled = cfread_int(file);

	// restore the default netgame protocol mode
	int protocol_temp = cfread_int(file);
	switch(protocol_temp){
		// TCP & PXO
		case NET_VMT:
			Multi_options_g.pxo = 1;
			Multi_options_g.protocol = NET_TCP;
			break;

		// plain TCP
		case NET_TCP:
			Multi_options_g.pxo = 0;
			Multi_options_g.protocol = NET_TCP;
			break;

		// in case of IPX, which is deprecated
		default:
			Multi_options_g.pxo = 0;
			Multi_options_g.protocol = NET_TCP;
			break;
	}

	// restore wingman status used by red alert missions
	red_alert_read_wingman_status(file, Player_file_version);

	// read techroom data
	pilot_read_techroom_data(file);

	// restore auto-advance pref
	Player->auto_advance = cfread_int(file);

	Use_mouse_to_fly = cfread_int(file);
	Mouse_sensitivity = cfread_int(file);
	Joy_sensitivity = cfread_int(file);
	Dead_zone_size = cfread_int(file);

	if (cfclose(file))
		return errno;

	// restore the callsign into the Player structure
	SDL_strlcpy(p->callsign, callsign, SDL_arraysize(p->callsign));

	// restore the truncated callsign into Player structure
	pilot_set_short_callsign(p, SHORT_CALLSIGN_PIXEL_W);
	
	// when we store the LastPlayer key, we have to mark it as being single or multiplayer, so we know where to look for him
	// (since we could have a single and a multiplayer pilot with the same callsign)
	// we'll distinguish them by putting an M and the end of the multiplayer callsign and a P at the end of a single player
	char cat[35];

	SDL_strlcpy(cat, p->callsign, SDL_arraysize(cat));
	if (is_multi)
		SDL_strlcat(cat, NOX("M"), SDL_arraysize(cat));
	else
		SDL_strlcat(cat, NOX("S"), SDL_arraysize(cat));

	os_config_write_string( NULL, "LastPlayer", cat );
/*
	// if he's not a multiplayer pilot, then load in the campaign file at this point!
	if (!is_multi) {
		if (mission_campaign_load_by_name(campaign_fname)) {
			strcpy(campaign_fname, BUILTIN_CAMPAIGN);
			if (mission_campaign_load_by_name(campaign_fname))
				SDL_assert(0);
		}
	}
	//Campaign.current_mission = mission_num;*/

	hud_squadmsg_save_keys();			// when new pilot read in, must save info for squadmate messaging

	return 0;
}

void read_stats_block(CFILE *file, int file_version, scoring_struct *stats)
{
	int i, total;
   
	init_scoring_element(stats);
	stats->score = cfread_int(file);
	stats->rank = cfread_int(file);
	stats->assists = cfread_int(file);

#ifndef MAKE_FS1
	if (file_version < 139) {
		// support for FS2_DEMO pilots that still have FS1 medal info in the .plr files
		for (i=0; i < NUM_MEDALS_FS1; i++) {
			cfread_int(file);			// dummy read
		}
	} else {
		// read the usual way
		for (i=0; i < NUM_MEDALS; i++) {
			stats->medals[i] = cfread_int(file);
		}
	}
#else
	for (i = 0; i < NUM_MEDALS; i++) {
		stats->medals[i] = cfread_int(file);
	}
#endif

	total = cfread_int(file);
	if (total > MAX_SHIP_TYPES){
		Warning(LOCATION, "Some ship kill information will be lost due to MAX_SHIP_TYPES decrease");
	}

	for (i=0; i<total && i<MAX_SHIP_TYPES; i++){
#ifndef MAKE_FS1
		stats->kills[i] = cfread_ushort(file);
#else
		stats->kills[i] = cfread_int(file);
#endif
	}

	stats->kill_count = cfread_int(file);
	stats->kill_count_ok = cfread_int(file);
	
	stats->p_shots_fired = cfread_uint(file);
	stats->s_shots_fired = cfread_uint(file);
	stats->p_shots_hit = cfread_uint(file);
	stats->s_shots_hit = cfread_uint(file);
	
   stats->p_bonehead_hits = cfread_uint(file);
	stats->s_bonehead_hits = cfread_uint(file);
	stats->bonehead_kills = cfread_uint(file);
}

// grab the various detail settings
void read_detail_settings(CFILE *file, int pfile_version)
{
	detail_level_set(NUM_DEFAULT_DETAIL_LEVELS-1);


	Detail.setting = cfread_int(file);

	Detail.nebula_detail = cfread_int(file);
	Detail.detail_distance = cfread_int(file);
#ifdef MAKE_FS1
	Detail.weapon_detail = cfread_int(file);
#endif
	Detail.hardware_textures = cfread_int(file);
	Detail.num_small_debris = cfread_int(file);
	Detail.num_particles = cfread_int(file);
	Detail.num_stars = cfread_int(file);
	Detail.shield_effects = cfread_int(file);
	Detail.lighting = cfread_int(file);
#ifdef MAKE_FS1
	Detail.unknown_slider = cfread_int(file);
#endif

	// Booleans
	Detail.targetview_model = cfread_int(file);
	Detail.planets_suns = cfread_int(file);
#ifdef MAKE_FS1
	Detail.unknown_boolean1 = cfread_int(file);
	Detail.unknown_boolean2 = cfread_int(file);
	Detail.engine_glows = cfread_int(file);
	Detail.alpha_effects = cfread_int(file);
#else
	Detail.weapon_extras = cfread_int(file);
#endif
}

// Will write the pilot file in the most current format
//
// if single == 1, save into the single players directory, else save into the multiplayers directory
int write_pilot_file_core(player *p)
{
	char filename[MAX_FILENAME_LEN + 1];
   int i, si_index;
	ubyte is_multi;
	CFILE *file;

	// never save a pilot file for the standalone server in multiplayer
	if ((Game_mode & GM_MULTIPLAYER) && (Game_mode & GM_STANDALONE_SERVER)) {
		return 0;
	}

	if (!p) {
		SDL_assert((Player_num >= 0) && (Player_num < MAX_PLAYERS));
		p = &Players[Player_num];
	}

	i = strlen(p->callsign);
	if (i == 0)
		return 0;	//	This means there is no player, probably meaning he was deleted and game exited from same screen.

	SDL_assert((i > 0) && (i <= MAX_FILENAME_LEN - 4));  // ensure we won't overrun the buffer
	SDL_strlcpy( filename, p->callsign, SDL_arraysize(filename));
	SDL_strlcat( filename, NOX(".plr"), SDL_arraysize(filename) );

	// determine if this pilot is a multiplayer pilot or not
	if (p->flags & PLAYER_FLAGS_IS_MULTI){
		is_multi = 1;
	} else {
		is_multi = 0;
	}

	// see above
	if ( !is_multi ){
		file = cfopen(filename, "wb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS);
	} else {
		file = cfopen(filename, "wb", CFILE_NORMAL, CF_TYPE_MULTI_PLAYERS);
	}

	if (!file){
		return errno;
	}

	// Write out player's info
	cfwrite_uint(PLR_FILE_ID, file);
	cfwrite_uint(CURRENT_PLAYER_FILE_VERSION, file);

	cfwrite_ubyte(is_multi, file);
	cfwrite_int(p->stats.rank, file);
	cfwrite_ubyte((ubyte) p->on_bastion, file);

#ifndef MAKE_FS1
	cfwrite_int(p->tips, file);
#endif

	// write out the image file name
	cfwrite_string_len(p->image_filename, file);

#ifndef MAKE_FS1
	// write out the image file name
	cfwrite_string_len(p->squad_name, file);
	cfwrite_string_len(p->squad_filename, file);
#endif

	// write out the name of the player's active campaign.
	cfwrite_string_len(p->current_campaign, file);	

	// write the ship name for last ship flown by the player
	si_index = p->last_ship_flown_si_index;
	if((si_index < 0) || (si_index >= Num_ship_types)){
		si_index = 0;
	}

	cfwrite_string_len(Ship_info[si_index].name, file);

	// ---------------------------------------------
	// write the keyboard/joystick configuration
	// ---------------------------------------------
	short key_id = 0;
	cfwrite_ubyte( CCFG_MAX, file );
	for (i=0; i<CCFG_MAX; i++) {
		key_id = keycode_translate_to(Control_config[i].key_id);
		cfwrite_short( key_id, file );
		cfwrite_short( Control_config[i].joy_id, file );
	}		

	// if this hud is an observer, the player ended the last mission as an observer, so we should
	// restore his "real" ship HUD.
	HUD_CONFIG_TYPE hc_temp;
	hc_temp.show_flags = 0;
	int stored_observer = 0;
	if ( HUD_config.is_observer ){
		// if we're in mission, copy the HUD we're currently using
		if(Game_mode & GM_IN_MISSION){			
			memcpy(&hc_temp,&HUD_config,sizeof(HUD_CONFIG_TYPE));
			stored_observer = 1;
		}

		hud_config_restore();				
	}

	// write the hud configuration
	cfwrite_int(HUD_config.show_flags, file);	
	cfwrite_int(HUD_config.show_flags2, file);	
	cfwrite_int(HUD_config.popup_flags, file);	
	cfwrite_int(HUD_config.popup_flags2, file);	
	cfwrite_ubyte( (ubyte) HUD_config.num_msg_window_lines, file );
	cfwrite_int( HUD_config.rp_flags, file );
	cfwrite_int( HUD_config.rp_dist, file );

#ifdef MAKE_FS1
	cfwrite_int( HUD_config.main_color, file );
	cfwrite_int( HUD_color_alpha, file );
#else
	for(i=0; i<NUM_HUD_GAUGES; i++){
		cfwrite(&HUD_config.clr[i], sizeof(color), 1, file);
	}
#endif

	// restore the HUD we backed up
	if( (Game_mode & GM_IN_MISSION) && stored_observer ){		
		memcpy(&HUD_config,&hc_temp,sizeof(HUD_CONFIG_TYPE));
	}

	// write the cutscenes which have been viewed
	cfwrite_int(Cutscenes_viewable, file);

	// store the digital sound fx volume, and music volume
	cfwrite_float(Master_sound_volume, file);
	cfwrite_float(Master_event_music_volume, file);
	cfwrite_float(Master_voice_volume, file);


	//cfwrite( &Detail, sizeof(detail_levels), 1, file );
	write_detail_settings(file);

	// store list of most recently played missions
	cfwrite_int(Num_recent_missions, file);
	for (i=0; i<Num_recent_missions; i++) {
		cfwrite_string_len(Recent_missions[i], file);
	}

	// write the player stats
	write_stats_block(file, &p->stats);	
   cfwrite_int(Game_skill_level, file);

	for (i=0; i<NUM_JOY_AXIS_ACTIONS; i++) {
		cfwrite_int(Axis_map_to[i], file);
		cfwrite_int(Invert_axis[i], file);
	}

	// store some player flags
   cfwrite_int(Player->save_flags, file);

	// store ship selection for most recent mission
	pilot_write_loadout(file);

	// read in multiplayer options	
	write_multiplayer_options(p, file);

	cfwrite_int(p->readyroom_listing_mode, file);
   cfwrite_int(Briefing_voice_enabled, file);

	// store the default netgame protocol mode for this pilot
	SDL_assert(Multi_options_g.protocol == NET_TCP);
	if (Multi_options_g.pxo == 1) {
		cfwrite_int(NET_VMT, file);
	} else {
		cfwrite_int(NET_TCP, file);
	}

	red_alert_write_wingman_status(file);
	pilot_write_techroom_data(file);

	// store auto-advance pref
   cfwrite_int(Player->auto_advance, file);

	cfwrite_int(Use_mouse_to_fly, file);
	cfwrite_int(Mouse_sensitivity, file);
	cfwrite_int(Joy_sensitivity, file);
	cfwrite_int(Dead_zone_size, file);

	if (!cfclose(file))
		return 0;

	return errno;
}

int write_pilot_file(player *the_player)
{
	int pilot_write_rval;
	do {
		// write_pilot_file_core returns 0 if ok, non-zero for error
		pilot_write_rval = write_pilot_file_core(the_player);

		// check with user if write not successful
		if (pilot_write_rval) {
			int popup_rval = popup(PF_TITLE_RED | PF_TITLE_BIG, 3, XSTR( "&Retry", 41), XSTR( "&Ignore", 42), XSTR( "&Quit Game", 43),
				XSTR( "Warning\nFailed to save pilot file.  You may be out of disk space.  If so, you should press Alt-Tab, free up some disk space, then come back and choose retry.\n", 44) );

			// quit game popup return value (2)
			if (popup_rval == 2) {
				exit(1);
			}

			// _ignore_ popup return value (1) - don't save the file
			if (popup_rval) {
				return pilot_write_rval;
			}

			// retry popup return value (0) - try again 
		}

	} while (pilot_write_rval);

	// write successful
	return 0;
}

void write_stats_block(CFILE *file,scoring_struct *stats)
{
	int i;
	int total;

	cfwrite_int(stats->score, file);
	cfwrite_int(stats->rank, file);
	cfwrite_int(stats->assists, file);
	for (i=0; i<NUM_MEDALS; i++){
		cfwrite_int(stats->medals[i], file);
	}

	total = MAX_SHIP_TYPES;
	while (total && !stats->kills[total - 1]){  // find last used element
		total--;
	}

	cfwrite_int(total, file);
	for (i=0; i<total; i++){
#ifndef MAKE_FS1
		cfwrite_ushort(stats->kills[i], file);
#else
		cfwrite_int(stats->kills[i], file);
#endif
	}

	cfwrite_int(stats->kill_count,file);
	cfwrite_int(stats->kill_count_ok,file);

   cfwrite_uint(stats->p_shots_fired,file);
	cfwrite_uint(stats->s_shots_fired,file);
	cfwrite_uint(stats->p_shots_hit,file);
	cfwrite_uint(stats->s_shots_hit,file);
	cfwrite_uint(stats->p_bonehead_hits,file);
	cfwrite_uint(stats->s_bonehead_hits,file);
	cfwrite_uint(stats->bonehead_kills,file);
}

// write the various detail settings
void write_detail_settings(CFILE *file)
{
	cfwrite_int(Detail.setting, file);

	cfwrite_int(Detail.nebula_detail, file);
	cfwrite_int(Detail.detail_distance, file);
#ifdef MAKE_FS1
	cfwrite_int(Detail.weapon_detail, file);
#endif
	cfwrite_int(Detail.hardware_textures, file);
	cfwrite_int(Detail.num_small_debris, file);
	cfwrite_int(Detail.num_particles, file);
	cfwrite_int(Detail.num_stars, file);
	cfwrite_int(Detail.shield_effects, file);
	cfwrite_int(Detail.lighting, file);
#ifdef MAKE_FS1
	cfwrite_int(Detail.unknown_slider, file);
#endif

	cfwrite_int(Detail.targetview_model, file);
	cfwrite_int(Detail.planets_suns, file);
#ifdef MAKE_FS1
	cfwrite_int(Detail.unknown_boolean1, file);
	cfwrite_int(Detail.unknown_boolean2, file);
	cfwrite_int(Detail.engine_glows, file);
	cfwrite_int(Detail.alpha_effects, file);
#else
	cfwrite_int(Detail.weapon_extras, file);
#endif
}

// write multiplayer information
void write_multiplayer_options(player *p,CFILE *file)
{	
	// write the netgame options
	cfwrite_ubyte(p->m_server_options.squad_set,file);
	cfwrite_ubyte(p->m_server_options.endgame_set,file);	
	cfwrite_int(p->m_server_options.flags,file);
	cfwrite_uint(p->m_server_options.respawn,file);
	cfwrite_ubyte(p->m_server_options.max_observers,file);
	cfwrite_ubyte(p->m_server_options.skill_level,file);
	cfwrite_ubyte(p->m_server_options.voice_qos,file);
	cfwrite_int(p->m_server_options.voice_token_wait,file);
	cfwrite_int(p->m_server_options.voice_record_time,file);
	cfwrite(&p->m_server_options.mission_time_limit,sizeof(fix),1,file);
	cfwrite_int(p->m_server_options.kill_limit,file);

	// write the local options
	cfwrite_int(p->m_local_options.flags,file);
	cfwrite_int(p->m_local_options.obj_update_level,file);
}

// read multiplayer options
void read_multiplayer_options(player *p,CFILE *file)
{
	// read the netgame options
	p->m_server_options.squad_set = cfread_ubyte(file);
	p->m_server_options.endgame_set = cfread_ubyte(file);	
	p->m_server_options.flags = cfread_int(file);
	p->m_server_options.respawn = cfread_uint(file);
	p->m_server_options.max_observers = cfread_ubyte(file);
	p->m_server_options.skill_level = cfread_ubyte(file);
	p->m_server_options.voice_qos = cfread_ubyte(file);
	p->m_server_options.voice_token_wait = cfread_int(file);
	p->m_server_options.voice_record_time = cfread_int(file);
	cfread(&p->m_server_options.mission_time_limit,sizeof(fix),1,file);
	p->m_server_options.kill_limit = cfread_int(file);

	// read the local options
	p->m_local_options.flags = cfread_int(file);
	p->m_local_options.obj_update_level = cfread_int(file);
}

// run thorough an open file (at the beginning) and see if this pilot is a multiplayer pilot
int is_pilot_multi(CFILE *fp)
{
	uint id,file_version;
	ubyte is_multi;
	
	id = cfread_uint(fp);
	if (id != PLR_FILE_ID) {
		Warning(LOCATION, "Player file has invalid signature");
		cfclose(fp);
		return -1;
	}

	// check for compatibility here
	file_version = cfread_uint(fp);
	if (file_version < LOWEST_COMPATIBLE_PLAYER_FILE_VERSION) {
		nprintf(("Warning", "WARNING => Player file is outdated and not compatible...\n"));
		cfclose(fp);
		return -1;
	}

	// read in whether he's a multiplayer or not
	is_multi = cfread_ubyte(fp);
	if (is_multi)
		return 1;

	return 0;
}

int is_pilot_multi(player *p)
{
	return (p->flags & PLAYER_FLAGS_IS_MULTI) ? 1 : 0;
}

// this works on barracks and player_select interface screens
void init_new_pilot(player *p, int reset)
{
	int cur_speed;

	if (reset) {
		hud_set_default_hud_config(p);		// use a default hud config

#ifndef MAKE_FS1
		// in the demo, load up the hardcoded hcf file
#ifdef FS2_DEMO
		hud_config_color_load("hud_1.hcf");
#else
		hud_config_color_load("hud_3.hcf");
#endif
#else
		hud_config_set_color(HUD_COLOR_GREEN);
#endif

		control_config_reset_defaults();		// get a default keyboard config
		player_set_pilot_defaults(p);			// set up any player struct defaults

		cur_speed = os_config_read_uint(NULL, NOX("ComputerSpeed"), 2 );
		if ( cur_speed < 0 )	{
			cur_speed = 0;
		} else if ( cur_speed >= NUM_DEFAULT_DETAIL_LEVELS )	{
			cur_speed = NUM_DEFAULT_DETAIL_LEVELS-1;
		}	
		// always set to high
		// DKA: 8/4/99 USE speed from registry
		// cur_speed = NUM_DEFAULT_DETAIL_LEVELS-2;

#if NUM_DEFAULT_DETAIL_LEVELS != 4
#error Code in ManagePilot assumes NUM_DEFAULT_DETAIL_LEVELS = 4
#endif

		detail_level_set(cur_speed);

		Game_skill_level = game_get_default_skill_level();

		mprintf(( "Setting detail level to %d because of new pilot\n", cur_speed ));
		Use_mouse_to_fly = 0;
		Mouse_sensitivity = 4;
		Joy_sensitivity = 9;
		Dead_zone_size = 10;
	}

	// unassigned squadron
	SDL_strlcpy(p->squad_name, XSTR("Unassigned", 1255), SDL_arraysize(p->squad_name));
	SDL_strlcpy(p->squad_filename, "", SDL_arraysize(p->squad_filename));

	// set him to be a single player pilot by default (the actual creation routines will change this if necessary)
	p->flags &= ~PLAYER_FLAGS_IS_MULTI;

	// effectively sets the length return by strlen() to 0	
	Campaign.filename[0] = 0;
	p->on_bastion = 0;

	// pick a random pilot image for this guy
	if (reset){
		pilot_set_random_pic(p);
		p->insignia_texture = -1;
		pilot_set_random_squad_pic(p);
	}

	init_scoring_element(&p->stats);	
	
	p->stats.score = 0;
	p->stats.rank = RANK_ENSIGN;	

#ifndef MAKE_FS1
	p->tips = 1;
#else
	p->tips = 0;
#endif

	Multi_options_g.protocol = NET_TCP;	

	// initialize default multiplayer options
	multi_options_set_netgame_defaults(&p->m_server_options);
	multi_options_set_local_defaults(&p->m_local_options);

	Player_loadout.filename[0] = 0;

	// reset the cutscenes which can be viewed
	if ( reset ){
		Cutscenes_viewable = INTRO_CUTSCENE_FLAG;
	}
}

void pilot_set_short_callsign(player *p, int max_width)
{
	SDL_strlcpy(p->short_callsign, p->callsign, SDL_arraysize(p->short_callsign));
	gr_set_font(FONT1);
	gr_force_fit_string(p->short_callsign, CALLSIGN_LEN - 1, max_width);
	gr_get_string_size( &(p->short_callsign_width), NULL, p->short_callsign );
}

// pick a random image for the passed player
void pilot_set_random_pic(player *p)
{
	// if there are no available pilot pics, set the image filename to null
	if (Num_pilot_images <= 0) {
		SDL_strlcpy(p->image_filename, "", SDL_arraysize(p->image_filename));
	} else {
		// pick a random name from the list
		int random_index = rand() % Num_pilot_images;
		SDL_assert((random_index >= 0) && (random_index < Num_pilot_images));
		SDL_strlcpy(p->image_filename, Pilot_images_arr[random_index], SDL_arraysize(p->image_filename));
	}	
}

// pick a random image for the passed player
void pilot_set_random_squad_pic(player *p)
{	
	// if there are no available pilot pics, set the image filename to null
	if (Num_pilot_squad_images <= 0) {
		player_set_squad_bitmap(p, "");
		// strcpy(p->squad_filename, "");		
	} else {
		// pick a random name from the list
		int random_index = rand() % Num_pilot_squad_images;		
		SDL_assert((random_index >= 0) && (random_index < Num_pilot_squad_images));
		player_set_squad_bitmap(p, Pilot_squad_images_arr[random_index]); 
		// strcpy(p->squad_filename, Pilot_squad_images_arr[random_index]);
	}	
}

// format a pilot's callsign into a "personal" form - ie, adding a 's or just an ' as appropriate
void pilot_format_callsign_personal(const char *in_callsign, char *out_callsign, const int out_size)
{
	// don't do anything if we've got invalid strings
	if((in_callsign == NULL) || (out_callsign == NULL)){
		return;
	}

	// copy the original string
	SDL_strlcpy(out_callsign, in_callsign, out_size);

	// tack on the appropriate postfix
	if(in_callsign[strlen(in_callsign) - 1] == 's'){		
		SDL_strlcat(out_callsign,XSTR( "\'", 45), out_size);
	} else {
		SDL_strlcat(out_callsign,XSTR( "\'s", 46), out_size);
	}
}

// throw up a popup asking the user to verify the overwrite of an existing pilot name
// 1 == ok to overwrite, 0 == not ok
int pilot_verify_overwrite()
{
	return popup(PF_USE_AFFIRMATIVE_ICON | PF_USE_NEGATIVE_ICON | PF_TITLE_RED | PF_TITLE_BIG, 2, XSTR( "&No", 47), XSTR( "&Yes", 48), XSTR( "Warning\nA duplicate pilot exists\nOverwrite?", 49));
}

extern int Skip_packfile_search;  // located in CFileList.cpp


// load up the list of pilot image filenames (do this at game startup as well as barracks startup)
void pilot_load_pic_list()
{
	Num_pilot_images = 0;
	
	// load pilot images from the player images directory
	Num_pilot_images = cf_get_file_list_preallocated(MAX_PILOT_IMAGES, Pilot_images_arr, Pilot_image_names, CF_TYPE_PLAYER_IMAGES_MAIN, NOX("*.pcx"));

	// sort all filenames
	cf_sort_filenames(Num_pilot_images, Pilot_image_names, CF_SORT_NAME);
}

// load up the list of pilot squad filenames
void pilot_load_squad_pic_list()
{
	Num_pilot_squad_images = 0;
	
	// load pilot images from the player images directory
	Num_pilot_squad_images = cf_get_file_list_preallocated(MAX_PILOT_IMAGES, Pilot_squad_images_arr, Pilot_squad_image_names, CF_TYPE_SQUAD_IMAGES_MAIN, NOX("*.pcx"));

	// sort all filenames
	cf_sort_filenames(Num_pilot_squad_images, Pilot_squad_image_names, CF_SORT_NAME);
}

// will attempt to load an insignia bitmap and set it as active for the player
void player_set_squad_bitmap(player *p, const char *fname)
{
	// sanity check
	if(p == NULL){
		return;
	}

	// if he has another bitmap already - unload it
	if(p->insignia_texture >= 0){
		bm_unload(p->insignia_texture);
	}

	p->insignia_texture = -1;

	// try and set the new one
	if (fname != p->squad_filename) {
		SDL_strlcpy(p->squad_filename, fname, SDL_arraysize(p->squad_filename));
	}

	if(strlen(p->squad_filename) > 0){
		p->insignia_texture = bm_load_duplicate(fname);
		
		// lock is as a transparent texture
		if(p->insignia_texture != -1){
			bm_lock(p->insignia_texture, 16, BMP_TEX_XPARENT);
			bm_unlock(p->insignia_texture);
		}
	}

	/*
	flen = strlen(filename);
	elen = strlen(ext);
	SDL_assert(flen < MAX_PATH_LEN);
	strcpy(path, filename);
	if ((flen < 4) || SDL_strcasecmp(path + flen - elen, ext)) {
		SDL_assert(flen + elen < MAX_PATH_LEN);
		strcat(path, ext);
	}
	*/
}

// set squadron
void player_set_squad(player *p, const char *squad_name)
{
	// sanity check
	if(p == NULL){
		return;
	}

	SDL_strlcpy(p->squad_name, squad_name, SDL_arraysize(p->squad_name));
}

DCF(pilot,"Changes pilot stats. (Like reset campaign)" )
{
	if (Dc_command) {
		dc_get_arg(ARG_STRING);
		if (!strcmp(Dc_arg, NOX("reset"))) {
			if (strlen(Campaign.filename)) {
				mission_campaign_savefile_delete(Campaign.filename);
				mission_campaign_load(Campaign.filename);
			}

		} else {
			Dc_help = 1;  // print usage, not stats
		}
	}

	if (Dc_help) {
		dc_printf( "Usage: pilot keyword\nWhere keyword can be in the following forms:\n" );
		dc_printf( "pilot reset			Resets campaign stats.\n" );
		Dc_status = 0;	// don't print status if help is printed.  Too messy.
	}

	if (Dc_status) {
		// no stats
	}
}

struct fs_keycode_t {
	short fs_code;
	SDL_Keycode sdl_code;
};

static const fs_keycode_t keycode_lookup[] = {
	{ /* KEY_0 */           0x0B,	SDLK_0			},
	{ /* KEY_1 */           0x02,	SDLK_1			},
	{ /* KEY_2 */           0x03,	SDLK_2			},
	{ /* KEY_3 */           0x04,	SDLK_3			},
	{ /* KEY_4 */           0x05,	SDLK_4			},
	{ /* KEY_5 */           0x06,	SDLK_5			},
	{ /* KEY_6 */           0x07,	SDLK_6			},
	{ /* KEY_7 */           0x08,	SDLK_7			},
	{ /* KEY_8 */           0x09,	SDLK_8			},
	{ /* KEY_9 */           0x0A,	SDLK_9			},
	{ /* KEY_A */           0x1E,	SDLK_a			},
	{ /* KEY_B */           0x30,	SDLK_b			},
	{ /* KEY_C */           0x2E,	SDLK_c			},
	{ /* KEY_D */           0x20,	SDLK_d			},
	{ /* KEY_E */           0x12,	SDLK_e			},
	{ /* KEY_F */           0x21,	SDLK_f			},
	{ /* KEY_G */           0x22,	SDLK_g			},
	{ /* KEY_H */           0x23,	SDLK_h			},
	{ /* KEY_I */           0x17,	SDLK_i			},
	{ /* KEY_J */           0x24,	SDLK_j			},
	{ /* KEY_K */           0x25,	SDLK_k			},
	{ /* KEY_L */           0x26,	SDLK_l			},
	{ /* KEY_M */           0x32,	SDLK_m			},
	{ /* KEY_N */           0x31,	SDLK_n			},
	{ /* KEY_O */           0x18,	SDLK_o			},
	{ /* KEY_P */           0x19,	SDLK_p			},
	{ /* KEY_Q */           0x10,	SDLK_q			},
	{ /* KEY_R */           0x13,	SDLK_r			},
	{ /* KEY_S */           0x1F,	SDLK_s			},
	{ /* KEY_T */           0x14,	SDLK_t			},
	{ /* KEY_U */           0x16,	SDLK_u			},
	{ /* KEY_V */           0x2F,	SDLK_v			},
	{ /* KEY_W */           0x11,	SDLK_w			},
	{ /* KEY_X */           0x2D,	SDLK_x			},
	{ /* KEY_Y */           0x15,	SDLK_y			},
	{ /* KEY_Z */           0x2C,	SDLK_z			},
	{ /* KEY_MINUS */       0x0C,	SDLK_MINUS		},
	{ /* KEY_EQUAL */       0x0D,	SDLK_EQUALS		},
	{ /* KEY_DIVIDE */      0x35,	SDLK_SLASH		},
	{ /* KEY_SLASH */       0x2B,	SDLK_BACKSLASH	},
	{ /* KEY_SLASH_UK */    0x56,	SDLK_BACKSLASH	},
	{ /* KEY_COMMA */       0x33,	SDLK_COMMA		},
	{ /* KEY_PERIOD */      0x34,	SDLK_PERIOD		},
	{ /* KEY_SEMICOL */     0x27,	SDLK_SEMICOLON	},
	{ /* KEY_LBRACKET */    0x1A,	SDLK_LEFTBRACKET	},
	{ /* KEY_RBRACKET */    0x1B,	SDLK_RIGHTBRACKET	},
	{ /* KEY_RAPOSTRO */    0x28,	SDLK_QUOTE		},
	{ /* KEY_LAPOSTRO */    0x29,	SDLK_BACKQUOTE	},
	{ /* KEY_ESC */         0x01,	SDLK_ESCAPE		},
	{ /* KEY_ENTER */       0x1C,	SDLK_RETURN		},
	{ /* KEY_BACKSP */      0x0E,	SDLK_BACKSPACE	},
	{ /* KEY_TAB */         0x0F,	SDLK_TAB		},
	{ /* KEY_SPACEBAR */    0x39,	SDLK_SPACE		},
	{ /* KEY_NUMLOCK */     0x61,	SDLK_NUMLOCKCLEAR	},
	{ /* KEY_SCROLLOCK */   0x46,	SDLK_SCROLLLOCK	},
	{ /* KEY_CAPSLOCK */    0x3A,	SDLK_CAPSLOCK	},
	{ /* KEY_LSHIFT */      0x2A,	SDLK_LSHIFT		},
	{ /* KEY_RSHIFT */      0x36,	SDLK_RSHIFT		},
	{ /* KEY_LALT */        0x38,	SDLK_LALT		},
	{ /* KEY_RALT */        0xB8,	SDLK_RALT		},
	{ /* KEY_LCTRL */       0x1D,	SDLK_LCTRL		},
	{ /* KEY_RCTRL */       0x9D,	SDLK_RCTRL		},
	{ /* KEY_F1 */          0x3B,	SDLK_F1			},
	{ /* KEY_F2 */          0x3C,	SDLK_F2			},
	{ /* KEY_F3 */          0x3D,	SDLK_F3			},
	{ /* KEY_F4 */          0x3E,	SDLK_F4			},
	{ /* KEY_F5 */          0x3F,	SDLK_F5			},
	{ /* KEY_F6 */          0x40,	SDLK_F6			},
	{ /* KEY_F7 */          0x41,	SDLK_F7			},
	{ /* KEY_F8 */          0x42,	SDLK_F8			},
	{ /* KEY_F9 */          0x43,	SDLK_F9			},
	{ /* KEY_F10 */         0x44,	SDLK_F10		},
	{ /* KEY_F11 */         0x57,	SDLK_F11		},
	{ /* KEY_F12 */         0x58,	SDLK_F12		},
	{ /* KEY_PAD0 */        0x52,	SDLK_KP_0		},
	{ /* KEY_PAD1 */        0x4F,	SDLK_KP_1		},
	{ /* KEY_PAD2 */        0x50,	SDLK_KP_2		},
	{ /* KEY_PAD3 */        0x51,	SDLK_KP_3		},
	{ /* KEY_PAD4 */        0x4B,	SDLK_KP_4		},
	{ /* KEY_PAD5 */        0x4C,	SDLK_KP_5		},
	{ /* KEY_PAD6 */        0x4D,	SDLK_KP_6		},
	{ /* KEY_PAD7 */        0x47,	SDLK_KP_7		},
	{ /* KEY_PAD8 */        0x48,	SDLK_KP_8		},
	{ /* KEY_PAD9 */        0x49,	SDLK_KP_9		},
	{ /* KEY_PADMINUS */    0x4A,	SDLK_KP_MINUS	},
	{ /* KEY_PADPLUS */     0x4E,	SDLK_KP_PLUS	},
	{ /* KEY_PADPERIOD */   0x53,	SDLK_KP_PERIOD	},
	{ /* KEY_PADDIVIDE */   0xB5,	SDLK_KP_DIVIDE	},
	{ /* KEY_PADMULTIPLY */ 0x37,	SDLK_KP_MULTIPLY	},
	{ /* KEY_PADENTER */    0x9C,	SDLK_KP_ENTER	},
	{ /* KEY_INSERT */      0xD2,	SDLK_INSERT		},
	{ /* KEY_HOME */        0xC7,	SDLK_HOME		},
	{ /* KEY_PAGEUP */      0xC9,	SDLK_PAGEUP		},
	{ /* KEY_DELETE */      0xd3,	SDLK_DELETE		},
	{ /* KEY_END */         0xCF,	SDLK_END		},
	{ /* KEY_PAGEDOWN */    0xD1,	SDLK_PAGEDOWN	},
	{ /* KEY_UP */          0xC8,	SDLK_UP			},
	{ /* KEY_DOWN */        0xD0,	SDLK_DOWN		},
	{ /* KEY_LEFT */        0xCB,	SDLK_LEFT		},
	{ /* KEY_RIGHT */       0xCD,	SDLK_RIGHT		},
	{ /* KEY_PRINT_SCRN */  0xB7,	SDLK_PRINTSCREEN	},
	{ /* KEY_PAUSE */       0x45,	SDLK_PAUSE		},
	{ /* KEY_BREAK */       0xc6,	SDLK_PAUSE		}
};

static SDL_Keycode keycode_translate_from(short keycode)
{
	const int tbl_size = sizeof(keycode_lookup) / sizeof(fs_keycode_t);

	if (keycode < 0) {
		return -1;
	}

	int mods = keycode & 0xf900;
	keycode &= KEY_MASK;

	for (int i = 0; i < tbl_size; i++) {
		if (keycode_lookup[i].fs_code == keycode) {
			return (SDL_Keycode)(keycode_lookup[i].sdl_code | mods);
		}
	}

	return -1;
}

static short keycode_translate_to(SDL_Keycode keycode)
{
	const int tbl_size = sizeof(keycode_lookup) / sizeof(fs_keycode_t);

	if (keycode < 0) {
		return -1;
	}

	int mods = keycode & 0xf900;
	keycode &= KEY_MASK;

	for (int i = 0; i < tbl_size; i++) {
		if (keycode_lookup[i].sdl_code == keycode) {
			return (short)(keycode_lookup[i].fs_code | mods);
		}
	}

	return -1;
}
