/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/CFile/cfile.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Utilities for operating on files
 *
 * $Log$
 * Revision 1.14  2006/04/26 19:36:57  taylor
 * address an error handling bug in cfwrite()
 *
 * Revision 1.13  2005/10/01 22:04:58  taylor
 * fix FS1 (de)briefing voices, the directory names are different in FS1
 * hard code the table values so that the fs1.vp file isn't needed
 * hard code a mission fix for sm2-08a since a have no idea how to fix it otherwise
 * generally cleanup some FS1 code
 * fix volume sliders in the options screen that never went all the way up
 *
 * Revision 1.12  2005/08/12 08:50:09  taylor
 * recursively create directories (hurt more on OSX) and update all _mkdir() calls accordingly
 *
 * Revision 1.11  2004/07/04 11:27:29  taylor
 * cleanup CFILE code a little, warning fixes, remove redundant dir checks, amd64 support
 *
 * Revision 1.10  2004/06/11 00:28:39  tigital
 * byte-swapping changes for bigendian systems
 *
 * Revision 1.9  2003/05/25 02:30:42  taylor
 * Freespace 1 support
 *
 * Revision 1.8  2003/02/20 17:41:07  theoddone33
 * Userdir patch from Taylor Richards
 *
 * Revision 1.7  2002/12/30 03:23:29  relnev
 * disable root dir check
 *
 * Revision 1.6  2002/06/17 06:33:08  relnev
 * ryan's struct patch for gcc 2.95
 *
 * Revision 1.5  2002/06/09 04:41:15  relnev
 * added copyright header
 *
 * Revision 1.4  2002/06/05 08:05:28  relnev
 * stub/warning removal.
 *
 * reworked the sound code.
 *
 * Revision 1.3  2002/05/28 08:52:03  relnev
 * implemented two assembly stubs.
 *
 * cleaned up a few warnings.
 *
 * added a little demo hackery to make it progress a little farther.
 *
 * Revision 1.2  2002/05/28 06:28:20  theoddone33
 * Filesystem mods, actually reads some data files now
 *
 * Revision 1.1.1.1  2002/05/03 03:28:08  root
 * Initial import.
 *
 * 
 * 20    9/08/99 10:01p Dave
 * Make sure game won't run in a drive's root directory. Make sure
 * standalone routes suqad war messages properly to the host.
 * 
 * 19    9/08/99 12:03a Dave
 * Make squad logos render properly in D3D all the time. Added intel anim
 * directory.
 * 
 * 18    9/06/99 2:35p Dave
 * Rename briefing and debriefing voice direcrory paths properly.
 * 
 * 17    9/03/99 1:31a Dave
 * CD checking by act. Added support to play 2 cutscenes in a row
 * seamlessly. Fixed super low level cfile bug related to files in the
 * root directory of a CD. Added cheat code to set campaign mission # in
 * main hall.
 * 
 * 16    8/31/99 9:46a Dave
 * Support for new cfile cbanims directory.
 * 
 * 15    8/06/99 11:20a Johns
 * Don't call game_cd_changed() in a demo or multiplayer beta build.
 * 
 * 14    6/08/99 2:33p Dave
 * Fixed release build warning. Put in hud config stuff.
 * 
 * 13    5/19/99 4:07p Dave
 * Moved versioning code into a nice isolated common place. Fixed up
 * updating code on the pxo screen. Fixed several stub problems.
 * 
 * 12    4/30/99 12:18p Dave
 * Several minor bug fixes.
 * 
 * 11    4/07/99 5:54p Dave
 * Fixes for encryption in updatelauncher.
 * 
 * 10    3/28/99 5:58p Dave
 * Added early demo code. Make objects move. Nice and framerate
 * independant, but not much else. Don't use yet unless you're me :)
 * 
 * 9     2/22/99 10:31p Andsager
 * Get rid of unneeded includes.
 * 
 * 8     1/12/99 3:15a Dave
 * Barracks screen support for selecting squad logos. We need real artwork
 * :)
 * 
 * 7     11/30/98 1:07p Dave
 * 16 bit conversion, first run.
 * 
 * 6     10/29/98 10:41a Dave
 * Change the way cfile initializes exe directory.
 * 
 * 5     10/13/98 9:19a Andsager
 * Add localization support to cfile.  Optional parameter with cfopen that
 * looks for localized files.
 * 
 * 4     10/12/98 9:54a Dave
 * Fixed a few file organization things.
 * 
 * 3     10/07/98 6:27p Dave
 * Globalized mission and campaign file extensions. Removed Silent Threat
 * special code. Moved \cache \players and \multidata into the \data
 * directory.
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:48a Dave
 * 
 * 110   9/17/98 10:31a Hoffoss
 * Changed code to use location of executable as root rather than relying
 * on the cwd being set correctly.  This is most helpful in the case of
 * Fred launching due to the user double-clicking on an .fsm file in
 * explorer or something (where the cwd is probably the location of the
 * .fsm file).
 * 
 * 109   9/11/98 4:14p Dave
 * Fixed file checksumming of < file_size. Put in more verbose kicking and
 * PXO stats store reporting.
 * 
 * 108   9/09/98 5:53p Dave
 * Put in new tracker packets in API. Change cfile to be able to checksum
 * portions of a file.
 * 
 * 107   9/04/98 3:51p Dave
 * Put in validated mission updating and application during stats
 * updating.
 * 
 * 106   8/31/98 2:06p Dave
 * Make cfile sort the ordering or vp files. Added support/checks for
 * recognizing "mission disk" players.
 * 
 * 105   8/20/98 5:30p Dave
 * Put in handy multiplayer logfile system. Now need to put in useful
 * applications of it all over the code.
 * 
 * 104   8/12/98 4:53p Dave
 * Put in 32 bit checksumming for PXO missions. No validation on the
 * actual tracker yet, though.
 * 
 * 103   6/17/98 9:30a Allender
 * fixed red alert replay stats clearing problem
 * 
 * 102   6/05/98 9:46a Lawrance
 * check for CD change on cfopen()
 * 
 * 101   5/19/98 1:19p Allender
 * new low level reliable socket reading code.  Make all missions/campaign
 * load/save to data missions folder (i.e. we are rid of the player
 * missions folder)
 * 
 * 100   5/13/98 10:22p John
 * Added cfile functions to read/write rle compressed blocks of data.
 * Made palman use it for .clr files.  Made alphacolors calculate on the
 * fly rather than caching to/from disk.
 * 
 * 99    5/11/98 10:59a John
 * Moved the low-level file reading code into cfilearchive.cpp.
 * 
 * 98    5/06/98 5:30p John
 * Removed unused cfilearchiver.  Removed/replaced some unused/little used
 * graphics functions, namely gradient_h and _v and pixel_sp.   Put in new
 * DirectX header files and libs that fixed the Direct3D alpha blending
 * problems.
 * 
 * 97    5/03/98 11:53a John
 * Fixed filename case mangling.
 * 
 * 96    5/03/98 8:33a John
 * Made sure there weren't any more broken function lurking around like
 * the one Allender fixed.  
 * 
 * 95    5/02/98 11:05p Allender
 * fix cfilelength for pack files
 * 
 *
 * $NoKeywords: $
 */
#define _CFILE_INTERNAL 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#ifndef PLAT_UNIX
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include "pstypes.h"
#include "cfile.h"
#include "encrypt.h"
//#include "outwnd.h"
//#include "vecmat.h"
//#include "timer.h"
#include "cfilesystem.h"
#include "cfilearchive.h"
#include "osapi.h"
#include "osregistry.h"

char Cfile_root_dir[CFILE_ROOT_DIRECTORY_LEN] = "";
char Cfile_user_dir[CFILE_ROOT_DIRECTORY_LEN] = "";

// During cfile_init, verify that Pathtypes[n].index == n for each item
// Each path must have a valid parent that can be tracable all the way back to the root 
// so that we can create directories when we need to.
//
cf_pathtype Pathtypes[CF_MAX_PATH_TYPES]  = {
	// What type this is          Path                             Extensions              Parent type
	{ CF_TYPE_INVALID,				NULL,										NULL,							CF_TYPE_INVALID },
	// Root must be index 1!!	
	{ CF_TYPE_ROOT,					"",										".mve",							CF_TYPE_ROOT	},
	{ CF_TYPE_DATA,					"Data",									".cfg .log .txt",			CF_TYPE_ROOT	},
	{ CF_TYPE_MAPS,					"Data" DIR_SEPARATOR_STR "Maps",							".pcx .ani .tga",			CF_TYPE_DATA	},
	{ CF_TYPE_TEXT,					"Data" DIR_SEPARATOR_STR "Text",							".txt .net",				CF_TYPE_DATA	},
#ifdef MAKE_FS1
	{ CF_TYPE_MISSIONS,				"Data" DIR_SEPARATOR_STR "Missions",						".fsm .fsc .ntl .ssv",	CF_TYPE_DATA	},
#else
	{ CF_TYPE_MISSIONS,				"Data" DIR_SEPARATOR_STR "Missions",						".fs2 .fc2 .ntl .ssv",	CF_TYPE_DATA	},
#endif
	{ CF_TYPE_MODELS,					"Data" DIR_SEPARATOR_STR "Models",						".pof",						CF_TYPE_DATA	},
	{ CF_TYPE_TABLES,					"Data" DIR_SEPARATOR_STR "Tables",						".tbl",						CF_TYPE_DATA	},
	{ CF_TYPE_SOUNDS,					"Data" DIR_SEPARATOR_STR "Sounds",						".wav",						CF_TYPE_DATA	},
	{ CF_TYPE_SOUNDS_8B22K,			"Data" DIR_SEPARATOR_STR "Sounds" DIR_SEPARATOR_STR "8b22k",				".wav",						CF_TYPE_SOUNDS	},
	{ CF_TYPE_SOUNDS_16B11K,		"Data" DIR_SEPARATOR_STR "Sounds" DIR_SEPARATOR_STR "16b11k",				".wav",						CF_TYPE_SOUNDS	},
	{ CF_TYPE_VOICE,					"Data" DIR_SEPARATOR_STR "Voice",							"",							CF_TYPE_DATA	},
#ifdef MAKE_FS1
	{ CF_TYPE_VOICE_BRIEFINGS,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Briefings",			".wav",						CF_TYPE_VOICE	},
#else
	{ CF_TYPE_VOICE_BRIEFINGS,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Briefing",			".wav",						CF_TYPE_VOICE	},
#endif
	{ CF_TYPE_VOICE_CMD_BRIEF,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Command_briefings",".wav",						CF_TYPE_VOICE	},
#ifdef MAKE_FS1
	{ CF_TYPE_VOICE_DEBRIEFINGS,	"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Debriefings",			".wav",						CF_TYPE_VOICE	},
#else
	{ CF_TYPE_VOICE_DEBRIEFINGS,	"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Debriefing",			".wav",						CF_TYPE_VOICE	},
#endif
	{ CF_TYPE_VOICE_PERSONAS,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Personas",			".wav",						CF_TYPE_VOICE	},
	{ CF_TYPE_VOICE_SPECIAL,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Special",				".wav",						CF_TYPE_VOICE	},
	{ CF_TYPE_VOICE_TRAINING,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Training",			".wav",						CF_TYPE_VOICE	},
	{ CF_TYPE_MUSIC,					"Data" DIR_SEPARATOR_STR "Music",							".wav",						CF_TYPE_VOICE	},
	{ CF_TYPE_MOVIES,					"Data" DIR_SEPARATOR_STR "Movies",						".mve .msb",				CF_TYPE_DATA	},
	{ CF_TYPE_INTERFACE,				"Data" DIR_SEPARATOR_STR "Interface",					".pcx .ani .tga",			CF_TYPE_DATA	},
	{ CF_TYPE_FONT,					"Data" DIR_SEPARATOR_STR "Fonts",							".vf",						CF_TYPE_DATA	},
	{ CF_TYPE_EFFECTS,				"Data" DIR_SEPARATOR_STR "Effects",						".ani .pcx .neb .tga",	CF_TYPE_DATA	},
	{ CF_TYPE_HUD,						"Data" DIR_SEPARATOR_STR "Hud",							".ani .pcx .tga",			CF_TYPE_DATA	},
	{ CF_TYPE_PLAYER_MAIN,			"Data" DIR_SEPARATOR_STR "Players",						"",							CF_TYPE_DATA	},
	{ CF_TYPE_PLAYER_IMAGES_MAIN,	"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Images",			".pcx",						CF_TYPE_PLAYER_MAIN	},
#ifdef MAKE_FS1
	{ CF_TYPE_CACHE,					"Cache",							".clr .tmp",				CF_TYPE_DATA	}, 	//clr=cached color
	{ CF_TYPE_PLAYERS,				"Players",						".hcf",						CF_TYPE_ROOT	},
	{ CF_TYPE_SINGLE_PLAYERS,		"Players" DIR_SEPARATOR_STR "Single",			".plr .csg .css",			CF_TYPE_PLAYERS	},
	{ CF_TYPE_MULTI_PLAYERS,		"Players" DIR_SEPARATOR_STR "Multi",				".plr",						CF_TYPE_PLAYERS	},
#else
	{ CF_TYPE_CACHE,					"Data" DIR_SEPARATOR_STR "Cache",							".clr .tmp",				CF_TYPE_DATA	}, 	//clr=cached color
	{ CF_TYPE_PLAYERS,				"Data" DIR_SEPARATOR_STR "Players",						".hcf",						CF_TYPE_DATA	},
	{ CF_TYPE_SINGLE_PLAYERS,		"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Single",			".plr .csg .css",			CF_TYPE_PLAYERS	},
	{ CF_TYPE_MULTI_PLAYERS,		"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Multi",				".plr",						CF_TYPE_DATA	},
#endif
	{ CF_TYPE_MULTI_CACHE,			"Data" DIR_SEPARATOR_STR "MultiData",					".pcx .fs2",				CF_TYPE_DATA	},
	{ CF_TYPE_CONFIG,					"Data" DIR_SEPARATOR_STR "Config",						".cfg",						CF_TYPE_DATA	},
	{ CF_TYPE_SQUAD_IMAGES_MAIN,	"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Squads",			".pcx",						CF_TYPE_DATA	},
	{ CF_TYPE_DEMOS,					"Data" DIR_SEPARATOR_STR "Demos",							".fsd",						CF_TYPE_DATA	},
	{ CF_TYPE_CBANIMS,				"Data" DIR_SEPARATOR_STR "CBAnims",						".ani",						CF_TYPE_DATA	},
	{ CF_TYPE_INTEL_ANIMS,			"Data" DIR_SEPARATOR_STR "IntelAnims",					".ani",						CF_TYPE_DATA	},
};


#define CFILE_STACK_MAX	8

int cfile_inited = 0;

Cfile_block Cfile_block_list[MAX_CFILE_BLOCKS];
CFILE Cfile_list[MAX_CFILE_BLOCKS];

//
// Function prototypes for internally-called functions
//
int cfget_cfile_block();
CFILE *cf_open_fill_cfblock(FILE * fp, int type);
CFILE *cf_open_packed_cfblock(FILE *fp, int type, int offset, int size);
void cf_chksum_long_init();

void cfile_close()
{
	cf_free_secondary_filelist();
}

// determine if the given path is in a root directory (c:\  or  c:\freespace2.exe  or  c:\fred2.exe   etc)
int cfile_in_root_dir(char *exe_path)
{
	int token_count = 0;
	char path_copy[2048] = "";
	char *tok;

	// bogus
	if(exe_path == NULL){
		return 1;
	}

	// copy the path
	memset(path_copy, 0, 2048);
	SDL_strlcpy(path_copy, exe_path, SDL_arraysize(path_copy));

	// count how many slashes there are in the path
	tok = strtok(path_copy, DIR_SEPARATOR_STR);
	if(tok == NULL){
		return 1;
	}	
	do {
		token_count++;
		tok = strtok(NULL, DIR_SEPARATOR_STR);
	} while(tok != NULL);
		
	// root directory if we have <= 1 slash
	if(token_count <= 2){
		return 1;
	}

	// not-root directory
	return 0;
}

// cfile_init() initializes the cfile system.  Called once at application start.
//
//	returns:  success ==> 0
//           error   ==> non-zero
//
int cfile_init()
{
	int i;

	// initialize encryption
	encrypt_init();	

	if ( !cfile_inited ) {
		// initialize root and user paths (may have been done already)
		if ( cfile_init_paths() ) {
			return 1;
		}

		cfile_inited = 1;

		for ( i = 0; i < MAX_CFILE_BLOCKS; i++ ) {
			Cfile_block_list[i].type = CFILE_BLOCK_UNUSED;
		}

		const char *extras_dir = os_config_read_string(NULL, "ExtrasPath", NULL);

		if ( extras_dir && (strlen(extras_dir) >= MAX_PATH_LEN) ) {
			extras_dir = NULL;
		}

		cf_build_secondary_filelist(extras_dir);

		// 32 bit CRC table init
		cf_chksum_long_init();



		atexit( cfile_close );
	}

	return 0;
}


// flush (delete all files in) the passed directory (by type), return the # of files deleted
// NOTE : WILL NOT DELETE READ-ONLY FILES
int cfile_flush_dir(int dir_type)
{
	char filespec[MAX_PATH_LEN];
	int del_count;
	SDL_PathInfo pinfo;

	SDL_assert( CF_TYPE_SPECIFIED(dir_type) );

	cf_create_default_path_string(filespec, dir_type);

	// proceed to delete the files
	del_count = 0;

	auto results = SDL_GlobDirectory(filespec, "*", 0, nullptr);

	if (results) {
		for (int i = 0; results[i]; ++i) {
			char fn[MAX_PATH_LEN];

			SDL_snprintf(fn, SDL_arraysize(fn), "%s%s\n", filespec, results[i]);

			if ( !SDL_GetPathInfo(fn, &pinfo) ) {
				continue;
			}

			if (pinfo.type != SDL_PATHTYPE_FILE) {
				continue;
			}

			// delete the file
			cf_delete(results[i], dir_type);

			// increment the deleted count
			++del_count;
		}

		SDL_free(results);
	}

	// return the # of files deleted
	return del_count;
}


// add the given extention to a filename (or filepath) if it doesn't already have this
// extension.
//    filename = name of filename or filepath to process
//    ext = extension to add.  Must start with the period
//    Returns: new filename or filepath with extension.
char *cf_add_ext(const char *filename, const char *ext)
{
	int flen, elen;
	static char path[MAX_PATH_LEN];

	flen = strlen(filename);
	elen = strlen(ext);
	SDL_assert(flen < MAX_PATH_LEN);
	SDL_strlcpy(path, filename, SDL_arraysize(path));
	if ((flen < 4) || SDL_strcasecmp(path + flen - elen, ext)) {
		SDL_assert(flen + elen < MAX_PATH_LEN);
		SDL_strlcat(path, ext, SDL_arraysize(path));
	}

	return path;
}

// Deletes a file.
void cf_delete( const char *filename, int dir_type )
{
	char longname[MAX_PATH_LEN];

	SDL_assert( CF_TYPE_SPECIFIED(dir_type) );

	cf_create_default_path_string( longname, dir_type, filename );

	FILE *fp = fopen(longname, "rb");
	if (fp) {
		// delete the file
		fclose(fp);
		unlink(longname);
	}

}


// Same as _access function to read a file's access bits
int cf_access( const char *filename, int dir_type, int mode )
{
	char longname[MAX_PATH_LEN];

	SDL_assert( CF_TYPE_SPECIFIED(dir_type) );

	cf_create_default_path_string( longname, dir_type, filename );

	return access(longname,mode);
}


// Returns 1 if file exists, 0 if not.
int cf_exist( const char *filename, int dir_type )
{
	char longname[MAX_PATH_LEN];

	SDL_assert( CF_TYPE_SPECIFIED(dir_type) );

	cf_create_default_path_string( longname, dir_type, filename );

	FILE *fp = fopen(longname, "rb");
	if (fp) {
		fclose(fp);
		return 1;
	}

	return 0;
}

int cf_rename(const char *old_name, const char *name, int dir_type)
{
	SDL_assert( CF_TYPE_SPECIFIED(dir_type) );

	int ret_code;
	char old_longname[MAX_PATH_LEN];
	char new_longname[MAX_PATH_LEN];
	
	cf_create_default_path_string( old_longname, dir_type, old_name );
	cf_create_default_path_string( new_longname, dir_type, name );

	ret_code = rename(old_longname, new_longname );		
	if(ret_code != 0){
		switch(errno){
		case EACCES :
			return CF_RENAME_FAIL_ACCESS;
		case ENOENT :
		default:
			return CF_RENAME_FAIL_EXIST;
		}
	}

	return CF_RENAME_SUCCESS;
	

}

// Creates the directory path if it doesn't exist. Even creates all its
// parent paths.
void cf_create_directory( int dir_type )
{
	int num_dirs = 0;
	int dir_tree[CF_MAX_PATH_TYPES];
	char longname[MAX_PATH_LEN];

	SDL_assert( CF_TYPE_SPECIFIED(dir_type) );

	int current_dir = dir_type;

	do {
		SDL_assert( num_dirs < CF_MAX_PATH_TYPES );		// Invalid Pathtypes data?

		dir_tree[num_dirs++] = current_dir;
		current_dir = Pathtypes[current_dir].parent_index;

	} while( current_dir != CF_TYPE_ROOT );

	
	int i;

	for (i=num_dirs-1; i>=0; i-- )	{
		cf_create_default_path_string( longname, dir_tree[i], NULL );

		if ( mkdir(longname, 0700) == 0 )	{
			mprintf(( "CFILE: Created new directory '%s'\n", longname ));
		}
	}


}


// cfopen()
//
// parameters:  *filepath ==> name of file to open (may be path+name)
//              *mode     ==> specifies how file should be opened (eg "rb" for read binary)
//                            passing NULL to mode deletes the file if it exists and returns NULL
//               type     ==> CFILE_NORMAL
//					  dir_type	=>	override extension check, value is one of CF_TYPE* #defines
//
//               NOTE: type parameter is an optional parameter.  The default value is CFILE_NORMAL
//
//
// returns:		success ==> address of CFILE structure
//					error   ==> NULL
//

CFILE *cfopen(const char *file_path, const char *mode, int type, int dir_type, bool localize)
{
	char longname[MAX_PATH_LEN];

//	nprintf(("CFILE", "CFILE -- trying to open %s\n", file_path ));
// #if !defined(MULTIPLAYER_BETA_BUILD) && !defined(FS2_DEMO)

	//================================================
	// Check that all the parameters make sense
	SDL_assert(file_path && strlen(file_path));
	SDL_assert( mode != NULL );

	//===========================================================
	// If in write mode, just try to open the file straight off
	// the harddisk.  No fancy packfile stuff here!
	
	if ( SDL_strchr(mode,'w') )	{
#ifdef PLAT_UNIX
		const char *toks = "/";
#else
		const char *toks = "/\\:";
#endif

		// For write-only files, require a full path or a path type
		if ( strpbrk(file_path, toks) ) {
			// Full path given?
			SDL_strlcpy(longname, file_path, SDL_arraysize(longname));
		} else {
			// Path type given?
			SDL_assert( dir_type != CF_TYPE_ANY );

			// Create the directory if necessary
			cf_create_directory( dir_type );

			cf_create_default_path_string( longname, dir_type, file_path );
		}

		// JOHN: TODO, you should create the path if it doesn't exist.
				
		FILE *fp = fopen(longname, mode);
		if (fp)	{
			return cf_open_fill_cfblock(fp, dir_type);
 		}
		return NULL;
	} 


	//================================================
	// Search for file on disk, on cdrom, or in a packfile

	int offset, size;
	char copy_file_path[MAX_PATH_LEN];  // FIX change in memory from cf_find_file_location
	SDL_strlcpy(copy_file_path, file_path, SDL_arraysize(copy_file_path));


	if ( cf_find_file_location( copy_file_path, dir_type, longname, &size, &offset, localize ) )	{
		// Fount it, now create a cfile out of it
		FILE *fp = fopen( longname, "rb" );

		if ( fp )	{
			if ( offset )	{
				// Found it in a pack file
				return cf_open_packed_cfblock(fp, dir_type, offset, size );
			} else {
				// Found it in a normal file
				return cf_open_fill_cfblock(fp, dir_type);
			}
		}
	}

	return NULL;
}


// ------------------------------------------------------------------------
// ctmpfile() 
//
// Open up a temporary file.  A unique name is automatically generated.  The
// file will be automatically deleted when file is closed.
//
// return:		NULL					=>		tmp file could not be opened
//					pointer to CFILE	=>		tmp file successfully opened
//
CFILE *ctmpfile()
{
	FILE	*fp;
	fp = tmpfile();
	if ( fp )
		return cf_open_fill_cfblock(fp, 0);
	else
		return NULL;
}



// cfget_cfile_block() will try to find an empty Cfile_block structure in the
//	Cfile_block_list[] array and return the index.
//
// returns:   success ==> index in Cfile_block_list[] array
//            failure ==> -1
//
int cfget_cfile_block()
{	
	int i;
	Cfile_block *cb;

	for ( i = 0; i < MAX_CFILE_BLOCKS; i++ ) {
		cb = &Cfile_block_list[i];
		if ( cb->type == CFILE_BLOCK_UNUSED ) {
			cb->fp = NULL;
			cb->type = CFILE_BLOCK_USED;
			return i;
		}
	}

	// If we've reached this point, a free Cfile_block could not be found
	nprintf(("Warning","A free Cfile_block could not be found.\n"));
	SDL_assert(0);	// out of free cfile blocks
	return -1;			
}


// cfclose() closes the file
//
// returns:   success ==> 0
//				  failure ==> EOF
//
int cfclose( CFILE * cfile )
{
	int result;

	SDL_assert(cfile != NULL);
	Cfile_block *cb;
	SDL_assert(cfile->id >= 0 && cfile->id < MAX_CFILE_BLOCKS);
	cb = &Cfile_block_list[cfile->id];	

	result = 0;

	if (cb->fp != NULL) {
		SDL_assert(cb->fp != NULL);
		result = fclose(cb->fp);
	} else {
		// VP  do nothing
	}

	cb->type = CFILE_BLOCK_UNUSED;
	return result;
}




// cf_open_fill_cfblock() will fill up a Cfile_block element in the Cfile_block_list[] array
// for the case of a file being opened by cf_open();
//
// returns:   success ==> ptr to CFILE structure.  
//            error   ==> NULL
//
CFILE *cf_open_fill_cfblock(FILE *fp, int type)
{
	int cfile_block_index;

	cfile_block_index = cfget_cfile_block();
	if ( cfile_block_index == -1 ) {
		return NULL;
	} else {
		CFILE *cfp;
		Cfile_block *cfbp;
		cfbp = &Cfile_block_list[cfile_block_index];
		cfp = &Cfile_list[cfile_block_index];;
		cfp->id = cfile_block_index;
		cfp->version = 0;
		cfbp->fp = fp;
		cfbp->dir_type = type;
		
		cf_init_lowlevel_read_code(cfp,0,filelength(fileno(fp)) );

		return cfp;
	}
}


// cf_open_packed_cfblock() will fill up a Cfile_block element in the Cfile_block_list[] array
// for the case of a file being opened by cf_open();
//
// returns:   success ==> ptr to CFILE structure.  
//            error   ==> NULL
//
CFILE *cf_open_packed_cfblock(FILE *fp, int type, int offset, int size)
{
	// Found it in a pack file
	int cfile_block_index;
	
	cfile_block_index = cfget_cfile_block();
	if ( cfile_block_index == -1 ) {
		return NULL;
	} else {
		CFILE *cfp;
		Cfile_block *cfbp;
		cfbp = &Cfile_block_list[cfile_block_index];
	
		cfp = &Cfile_list[cfile_block_index];
		cfp->id = cfile_block_index;
		cfp->version = 0;
		cfbp->fp = fp;
		cfbp->dir_type = type;

		cf_init_lowlevel_read_code(cfp,offset, size );

		return cfp;
	}

}


int cf_get_dir_type(CFILE *cfile)
{
	return Cfile_block_list[cfile->id].dir_type;
}


// version number of opened file.  Will be 0 unless you put something else here after you
// open a file.  Once set, you can use minimum version numbers with the read functions.
void cf_set_version( CFILE * cfile, int version )
{
	SDL_assert(cfile != NULL);

	cfile->version = version;
}

// routines to read basic data types from CFILE's.  Put here to
// simplify mac/pc reading from cfiles.

float cfread_float(CFILE *file, int ver, float deflt)
{
	float f;

	if (file->version < ver)
		return deflt;

	if (cfread( &f, sizeof(f), 1, file) != 1)
		return deflt;

    f = INTEL_FLOAT(f);
	return f;
}

int cfread_int(CFILE *file, int ver, int deflt)
{
	int i;

	if (file->version < ver)
		return deflt;

	if (cfread( &i, sizeof(i), 1, file) != 1)
		return deflt;

	i = INTEL_INT(i);
	return i;
}

uint cfread_uint(CFILE *file, int ver, uint deflt)
{
	uint i;

	if (file->version < ver)
		return deflt;

	if (cfread( &i, sizeof(i), 1, file) != 1)
		return deflt;

	i = INTEL_INT(i);
	return i;
}

short cfread_short(CFILE *file, int ver, short deflt)
{
	short s;

	if (file->version < ver)
		return deflt;

	if (cfread( &s, sizeof(s), 1, file) != 1)
		return deflt;

	s = INTEL_SHORT(s);
	return s;
}

ushort cfread_ushort(CFILE *file, int ver, ushort deflt)
{
	ushort s;

	if (file->version < ver)
		return deflt;

	if (cfread( &s, sizeof(s), 1, file) != 1)
		return deflt;

	s = INTEL_SHORT(s);
	return s;
}

ubyte cfread_ubyte(CFILE *file, int ver, ubyte deflt)
{
	ubyte b;

	if (file->version < ver)
		return deflt;

	if (cfread( &b, sizeof(b), 1, file) != 1)
		return deflt;

	return b;
}

void cfread_vector(vector *vec, CFILE *file, int ver, vector *deflt)
{
	if (file->version < ver) {
		if (deflt)
			*vec = *deflt;
		else
			vec->xyz.x = vec->xyz.y = vec->xyz.z = 0.0f;

		return;
	}

	vec->xyz.x = cfread_float(file, ver, deflt ? deflt->xyz.x : 0.0f);
	vec->xyz.y = cfread_float(file, ver, deflt ? deflt->xyz.y : 0.0f);
	vec->xyz.z = cfread_float(file, ver, deflt ? deflt->xyz.z : 0.0f);
}
	
void cfread_angles(angles *ang, CFILE *file, int ver, angles *deflt)
{
	if (file->version < ver) {
		if (deflt)
			*ang = *deflt;
		else
			ang->p = ang->b = ang->h = 0.0f;

		return;
	}

	ang->p = cfread_float(file, ver, deflt ? deflt->p : 0.0f);
	ang->b = cfread_float(file, ver, deflt ? deflt->b : 0.0f);
	ang->h = cfread_float(file, ver, deflt ? deflt->h : 0.0f);
}

char cfread_char(CFILE *file, int ver, char deflt)
{
	char b;

	if (file->version < ver)
		return deflt;

	if (cfread( &b, sizeof(b), 1, file) != 1)
		return deflt;

	return b;
}

void cfread_string(char *buf, int n, CFILE *file)
{
	char c;

	do {
		c = cfread_char(file);
		if ( n > 0 )	{
			*buf++ = c;
			n--;
		}
	} while (c != 0 );
}

void cfread_string_len(char *buf,int n, CFILE *file)
{
	int len;
	len = cfread_int(file);
	SDL_assert( len < n );
	if (len)
		cfread(buf, len, 1, file);

	buf[len] = 0;
}

// equivalent write functions of above read functions follow

int cfwrite_float(float f, CFILE *file)
{
    f = INTEL_FLOAT(f);
	return cfwrite(&f, sizeof(f), 1, file);
}

int cfwrite_int(int i, CFILE *file)
{
	i = INTEL_INT(i);
	return cfwrite(&i, sizeof(i), 1, file);
}

int cfwrite_uint(uint i, CFILE *file)
{
	i = INTEL_INT(i);
	return cfwrite(&i, sizeof(i), 1, file);
}

int cfwrite_short(short s, CFILE *file)
{
	s = INTEL_SHORT(s);
	return cfwrite(&s, sizeof(s), 1, file);
}

int cfwrite_ushort(ushort s, CFILE *file)
{
	s = INTEL_SHORT(s);
	return cfwrite(&s, sizeof(s), 1, file);
}

int cfwrite_ubyte(ubyte b, CFILE *file)
{
	return cfwrite(&b, sizeof(b), 1, file);
}

int cfwrite_vector(vector *vec, CFILE *file)
{
	if(!cfwrite_float(vec->xyz.x, file)){
		return 0;
	}
	if(!cfwrite_float(vec->xyz.y, file)){
		return 0;
	}
	return cfwrite_float(vec->xyz.z, file);
}

int cfwrite_angles(angles *ang, CFILE *file)
{
	if(!cfwrite_float(ang->p, file)){
		return 0;
	}
	if(!cfwrite_float(ang->b, file)){
		return 0;
	}
	return cfwrite_float(ang->h, file);
}

int cfwrite_char(char b, CFILE *file)
{
	return cfwrite( &b, sizeof(b), 1, file);
}

int cfwrite_string(const char *buf, CFILE *file)
{
	if ( (!buf) || (buf && !buf[0]) ) {
		return cfwrite_char(0, file);
	} 
	int len = strlen(buf);
	if(!cfwrite(buf, len, 1, file)){
		return 0;
	}
	return cfwrite_char(0, file);			// write out NULL termination			
}

int cfwrite_string_len(const char *buf, CFILE *file)
{
	int len = strlen(buf);

	if(!cfwrite_int(len, file)){
		return 0;
	}
	if (len){
		return cfwrite(buf,len,1,file);
	} 

	return 1;
}

// Get the filelength
int cfilelength( CFILE * cfile )
{
	SDL_assert(cfile != NULL);
	Cfile_block *cb;
	SDL_assert(cfile->id >= 0 && cfile->id < MAX_CFILE_BLOCKS);
	cb = &Cfile_block_list[cfile->id];	

	SDL_assert(cb->fp != NULL);

	// cb->size gets set at cfopen
	return cb->size;
}

// cfwrite() writes to the file
//
// returns:   number of full elements actually written
//            
//
int cfwrite(const void *buf, int elsize, int nelem, CFILE *cfile)
{
	SDL_assert(cfile != NULL);
	SDL_assert(buf != NULL);
	SDL_assert(elsize > 0);
	SDL_assert(nelem > 0);

	Cfile_block *cb;
	SDL_assert(cfile->id >= 0 && cfile->id < MAX_CFILE_BLOCKS);
	cb = &Cfile_block_list[cfile->id];	

	int size = elsize * nelem;

	SDL_assert(cb->fp != NULL);
	SDL_assert(cb->lib_offset == 0 );
	int bytes_written = fwrite( buf, 1, size, cb->fp );

	if (bytes_written > 0) {
		cb->raw_position += bytes_written;
	}

	#if defined(CHECK_POSITION) && !defined(NDEBUG)
		int tmp_offset = ftell(cb->fp) - cb->lib_offset;
		SDL_assert(tmp_offset == cb->raw_position);
	#endif

	return bytes_written / elsize;
}


// cfputc() writes a character to a file
//
// returns:   success ==> returns character written
//				  error   ==> EOF
//
int cfputc(int c, CFILE *cfile)
{
	int result = 0;

	SDL_assert(cfile != NULL);
	Cfile_block *cb;
	SDL_assert(cfile->id >= 0 && cfile->id < MAX_CFILE_BLOCKS);
	cb = &Cfile_block_list[cfile->id];	

	SDL_assert(cb->fp != NULL);
	result = fputc(c, cb->fp);

	return result;	
}


// cfgetc() reads a character from a file
//
// returns:   success ==> returns character read
//				  error   ==> EOF
//
int cfgetc(CFILE *cfile)
{
	SDL_assert(cfile != NULL);
	
	char tmp;

	int result = cfread(&tmp, 1, 1, cfile );
	if ( result == 1 )	{
		result = char(tmp);
	} else {
		result = CF_EOF;
	}

	return result;	
}





// cfgets() reads a string from a file
//
// returns:   success ==> returns pointer to string
//				  error   ==> NULL
//
char *cfgets(char *buf, int n, CFILE *cfile)
{
	SDL_assert(cfile != NULL);
	SDL_assert(buf != NULL);
	SDL_assert(n > 0 );

	char * t = buf;
	int i, c;

	for (i=0; i<n-1; i++ )	{
		do {
			char tmp_c;

			int ret = cfread( &tmp_c, 1, 1, cfile );
			if ( ret != 1 )	{
				*buf = 0;
				if ( buf > t )	{		
					return t;
				} else {
					return NULL;
				}
			}
			c = int(tmp_c);
		} while ( c == 13 );
		*buf++ = char(c);
		if ( c=='\n' ) break;
	}
	*buf++ = 0;

	return  t;
}

// cfputs() writes a string to a file
//
// returns:   success ==> non-negative value
//				  error   ==> EOF
//
int cfputs(const char *str, CFILE *cfile)
{
	SDL_assert(cfile != NULL);
	SDL_assert(str != NULL);

	Cfile_block *cb;
	SDL_assert(cfile->id >= 0 && cfile->id < MAX_CFILE_BLOCKS);
	cb = &Cfile_block_list[cfile->id];	

	int result = 0;

	// cfputs() not supported for memory-mapped files
	SDL_assert(cb->fp != NULL);
	result = fputs(str, cb->fp);

	return result;	
}


// 16 and 32 bit checksum stuff ----------------------------------------------------------

// CRC code for mission validation.  given to us by Kevin Bentley on 7/20/98.   Some sort of
// checksumming code that he wrote a while ago.  
#define CRC32_POLYNOMIAL					0xEDB88320L
uint CRCTable[256];

#define CF_CHKSUM_SAMPLE_SIZE				512

// update cur_chksum with the chksum of the new_data of size new_data_size
ushort cf_add_chksum_short(ushort seed, const char *buffer, int size)
{
	const ubyte * ptr = (const ubyte *)buffer;
	uint sum1,sum2;

	sum1 = sum2 = (uint)(seed);

	while(size--)	{
		sum1 += *ptr++;
		if (sum1 >= 255 ) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;
	
	return (ushort)((sum1<<8)+ sum2);
}

// update cur_chksum with the chksum of the new_data of size new_data_size
uint cf_add_chksum_long(uint seed, const char *buffer, int size)
{
	uint crc;
	unsigned const char *p;
	uint temp1;
	uint temp2;

	p = (unsigned const char*)buffer;
	crc = seed;	

	while (size--!=0){
	  temp1 = (crc>>8)&0x00FFFFFFL;
	  temp2 = CRCTable[((int)crc^*p++)&0xff];
	  crc = temp1^temp2;
	}	
	
	return crc;
}

void cf_chksum_long_init()
{
	int i,j;
	uint crc;

	for( i=0;i<=255;i++) {
		crc=i;
		for(j=8;j>0;j--) {
			if(crc&1)
				 crc=(crc>>1)^CRC32_POLYNOMIAL;
			else
				 crc>>=1;
		}
		CRCTable[i]=crc;
	}	
}

// single function convenient to use for both short and long checksums
// NOTE : only one of chk_short or chk_long must be non-NULL (indicating which checksum to perform)
int cf_chksum_do(CFILE *cfile, ushort *chk_short, uint *chk_long, int max_size)
{
	char cf_buffer[CF_CHKSUM_SAMPLE_SIZE];
	int is_long;
	int cf_len = 0;
	int cf_total;
	int read_size;

	// determine whether we're doing a short or long checksum
	is_long = 0;
	if(chk_short){
		SDL_assert(!chk_long);		
		*chk_short = 0;
	} else {
		SDL_assert(chk_long);
		is_long = 1;
		*chk_long = 0;
	}

	// if max_size is -1, set it to be the size of the file
	if(max_size < 0){
		cfseek(cfile, 0, SEEK_SET);
		max_size = cfilelength(cfile);
	}
	
	cf_total = 0;
	do {
		// determine how much we want to read
		if((max_size - cf_total) >= CF_CHKSUM_SAMPLE_SIZE){
			read_size = CF_CHKSUM_SAMPLE_SIZE;
		} else {
			read_size = max_size - cf_total;
		}

		// read in some buffer
		cf_len = cfread(cf_buffer, 1, read_size, cfile);

		// total we've read so far
		cf_total += cf_len;

		// add the checksum
		if(cf_len > 0){
			// do the proper short or long checksum
			if(is_long){
				*chk_long = cf_add_chksum_long(*chk_long, cf_buffer, cf_len);
			} else {
				*chk_short = cf_add_chksum_short(*chk_short, cf_buffer, cf_len);
			}
		}
	} while((cf_len > 0) && (cf_total < max_size));

	return 1;
}

// get the 2 byte checksum of the passed filename - return 0 if operation failed, 1 if succeeded
int cf_chksum_short(const char *filename, ushort *chksum, int max_size, int cf_type)
{
	int ret_val;
	CFILE *cfile = NULL;		
	
	// zero the checksum
	*chksum = 0;

	// attempt to open the file
	cfile = cfopen(filename,"rt",CFILE_NORMAL,cf_type);
	if(cfile == NULL){		
		return 0;
	}
	
	// call the overloaded cf_chksum function()
	ret_val = cf_chksum_do(cfile, chksum, NULL, max_size);

	// close the file down
	cfclose(cfile);
	cfile = NULL;

	// return the result
	return ret_val;
}

// get the 2 byte checksum of the passed file - return 0 if operation failed, 1 if succeeded
// NOTE : preserves current file position
int cf_chksum_short(CFILE *file, ushort *chksum, int max_size)
{
	int ret_code;
	int start_pos;
	
	// Returns current position of file.
	start_pos = cftell(file);
	if(start_pos == -1){
		return 0;
	}
	
	// move to the beginning of the file
	if(cfseek(file, 0, CF_SEEK_SET)){
		return 0;
	}
	ret_code = cf_chksum_do(file, chksum, NULL, max_size);
	// move back to the start position
	cfseek(file, start_pos, CF_SEEK_SET);

	return ret_code;
}

// get the 32 bit CRC checksum of the passed filename - return 0 if operation failed, 1 if succeeded
int cf_chksum_long(const char *filename, uint *chksum, int max_size, int cf_type)
{
	int ret_val;
	CFILE *cfile = NULL;		
	
	// zero the checksum
	*chksum = 0;

	// attempt to open the file
	cfile = cfopen(filename,"rt",CFILE_NORMAL,cf_type);
	if(cfile == NULL){		
		return 0;
	}
	
	// call the overloaded cf_chksum function()
	ret_val = cf_chksum_do(cfile, NULL, chksum, max_size);

	// close the file down
	cfclose(cfile);
	cfile = NULL;

	// return the result
	return ret_val;	
}

// get the 32 bit CRC checksum of the passed file - return 0 if operation failed, 1 if succeeded
// NOTE : preserves current file position
int cf_chksum_long(CFILE *file, uint *chksum, int max_size)
{
	int ret_code;
	int start_pos;
	
	// Returns current position of file.
	start_pos = cftell(file);
	if(start_pos == -1){
		return 0;
	}
	
	// move to the beginning of the file
	if(cfseek(file, 0, CF_SEEK_SET)){
		return 0;
	}
	ret_code = cf_chksum_do(file, NULL, chksum, max_size);
	// move back to the start position
	cfseek(file, start_pos, CF_SEEK_SET);

	return ret_code;
}


// Flush the open file buffer
//
// exit: 0 - success
//			1 - failure
int cflush(CFILE *cfile)
{
	SDL_assert(cfile != NULL);
	Cfile_block *cb;
	SDL_assert(cfile->id >= 0 && cfile->id < MAX_CFILE_BLOCKS);
	cb = &Cfile_block_list[cfile->id];	

	SDL_assert(cb->fp != NULL);
	return fflush(cb->fp);
}

// fill in Cfile_root_dir[] and Cfile_user_dir[]
// this can be called at any time, even before cfile_init()
//  returns: non-zero on error
int cfile_init_paths()
{
	if ( strlen(Cfile_root_dir) && strlen(Cfile_user_dir) ) {
		return 0;
	}

#ifndef __EMSCRIPTEN__
	const char *t_path = SDL_GetBasePath();

	// make sure we have something
	if (t_path == NULL) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error trying to determine executable directory!", NULL);
		return 1;
	}

	// size check
	if ( strlen(t_path) >= CFILE_ROOT_DIRECTORY_LEN ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Executable path is too long!", NULL);
		return 1;
	}

	// set root directory
	SDL_strlcpy(Cfile_root_dir, t_path, SDL_arraysize(Cfile_root_dir));

	// are we in a root directory?
	if ( cfile_in_root_dir(Cfile_root_dir) ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Freespace2/Fred2 cannot be run from a drive root directory!", NULL);
		return 1;
	}

	// now for the user/pref directory, the writable location
	char *u_path = SDL_GetPrefPath(Osreg_company_name, Osreg_app_name);

	// make sure we have something
	if (u_path == NULL) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error trying to determine preferences directory!", NULL);
		return 1;
	}

	// size check
	if ( strlen(u_path) >= CFILE_ROOT_DIRECTORY_LEN ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Preferences path is too long!", NULL);
		return 1;
	}

	// set user/pref directory
	SDL_strlcpy(Cfile_user_dir, u_path, SDL_arraysize(Cfile_user_dir));
	// free SDL copy
	SDL_free(u_path);
	u_path = NULL;
#else
	const char *root_path = "/Game";
	const char *user_path = "/User";

	SDL_snprintf(Cfile_root_dir, SDL_arraysize(Cfile_root_dir), "%s/", root_path);
	SDL_snprintf(Cfile_user_dir, SDL_arraysize(Cfile_user_dir), "%s/", user_path);

	EM_ASM({
		const user_path = UTF8ToString($0);

		FS.mkdir(user_path);
		FS.mount(IDBFS, { name: UTF8ToString($1) }, user_path);

		Module.sync_in_progress = 1;

		if (Module['setStatus']) {
			Module['setStatus']('Syncing user data...');
		}

		FS.syncfs(true, function(err) {
			if (err && err.code !== 'EEXIST') {
				console.log('FS.syncfs() load error: ' + err);
			} else {
				Module.sync_in_progress = 0;

				// remove initial loading screen
				var loading = document.getElementById('loading');
				loading.hidden = true;
			}
		});
	}, user_path, Osreg_app_name);
#endif

	// see if CF_TYPE_DATA exists for user and if not populate user path
	// with full directory tree
	char pathname[MAX_PATH_LEN];
	struct stat info;

	SDL_strlcpy(pathname, Cfile_user_dir, MAX_PATH_LEN);
	SDL_strlcat(pathname, Pathtypes[CF_TYPE_DATA].path, MAX_PATH_LEN);

	if ( stat(pathname, &info) != 0 ) {
		cf_create_directory(CF_TYPE_MAPS);
		cf_create_directory(CF_TYPE_TEXT);
		cf_create_directory(CF_TYPE_MISSIONS);
		cf_create_directory(CF_TYPE_MODELS);
		cf_create_directory(CF_TYPE_TABLES);
		cf_create_directory(CF_TYPE_SOUNDS_8B22K);
		cf_create_directory(CF_TYPE_SOUNDS_16B11K);
		cf_create_directory(CF_TYPE_VOICE_BRIEFINGS);
		cf_create_directory(CF_TYPE_VOICE_CMD_BRIEF);
		cf_create_directory(CF_TYPE_VOICE_DEBRIEFINGS);
		cf_create_directory(CF_TYPE_VOICE_PERSONAS);
		cf_create_directory(CF_TYPE_VOICE_SPECIAL);
		cf_create_directory(CF_TYPE_VOICE_TRAINING);
		cf_create_directory(CF_TYPE_MUSIC);
		cf_create_directory(CF_TYPE_MOVIES);
		cf_create_directory(CF_TYPE_INTERFACE);
		cf_create_directory(CF_TYPE_FONT);
		cf_create_directory(CF_TYPE_EFFECTS);
		cf_create_directory(CF_TYPE_HUD);
		cf_create_directory(CF_TYPE_PLAYER_IMAGES_MAIN);
		cf_create_directory(CF_TYPE_CACHE);
		cf_create_directory(CF_TYPE_SINGLE_PLAYERS);
		cf_create_directory(CF_TYPE_MULTI_PLAYERS);
		cf_create_directory(CF_TYPE_MULTI_CACHE);
		cf_create_directory(CF_TYPE_CONFIG);
		cf_create_directory(CF_TYPE_SQUAD_IMAGES_MAIN);
		cf_create_directory(CF_TYPE_DEMOS);
		cf_create_directory(CF_TYPE_CBANIMS);
		cf_create_directory(CF_TYPE_INTEL_ANIMS);
	}

	return 0;
}
