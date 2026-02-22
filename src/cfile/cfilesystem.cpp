/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/CFile/CfileSystem.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Functions to keep track of and find files that can exist
 * on the harddrive, cd-rom, or in a pack file on either of those.
 * This keeps a list of all the files in packfiles or on CD-rom
 * and when you need a file you call one function which then searches
 * all those locations, inherently enforcing precedence orders.
 *
 * $Log$
 * Revision 1.11  2004/07/04 11:27:29  taylor
 * cleanup CFILE code a little, warning fixes, remove redundant dir checks, amd64 support
 *
 * Revision 1.10  2004/06/11 00:29:22  tigital
 * byte-swapping changes for bigendian systems
 *
 * Revision 1.9  2003/05/27 03:03:11  taylor
 * fix second root (gamedir) searching
 *
 * Revision 1.8  2003/02/20 17:41:07  theoddone33
 * Userdir patch from Taylor Richards
 *
 * Revision 1.7  2002/06/22 23:57:39  relnev
 * remove writable strings.
 *
 * fix compile for intel compiler.
 *
 * Revision 1.6  2002/06/09 04:41:15  relnev
 * added copyright header
 *
 * Revision 1.5  2002/06/05 04:03:32  relnev
 * finished cfilesystem.
 *
 * removed some old code.
 *
 * fixed mouse save off-by-one.
 *
 * sound cleanups.
 *
 * Revision 1.4  2002/05/28 17:26:57  theoddone33
 * Fill in some timer and palette setting stubs.  Still no display
 *
 * Revision 1.3  2002/05/28 06:45:38  theoddone33
 * Cleanup some stuff
 *
 * Revision 1.2  2002/05/28 06:28:20  theoddone33
 * Filesystem mods, actually reads some data files now
 *
 * Revision 1.1.1.1  2002/05/03 03:28:08  root
 * Initial import.
 *
 * 
 * 6     9/08/99 10:01p Dave
 * Make sure game won't run in a drive's root directory. Make sure
 * standalone routes suqad war messages properly to the host.
 * 
 * 5     9/03/99 1:31a Dave
 * CD checking by act. Added support to play 2 cutscenes in a row
 * seamlessly. Fixed super low level cfile bug related to files in the
 * root directory of a CD. Added cheat code to set campaign mission # in
 * main hall.
 * 
 * 4     2/22/99 10:31p Andsager
 * Get rid of unneeded includes.
 * 
 * 3     10/13/98 9:19a Andsager
 * Add localization support to cfile.  Optional parameter with cfopen that
 * looks for localized files.
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:48a Dave
 * 
 * 14    8/31/98 2:06p Dave
 * Make cfile sort the ordering or vp files. Added support/checks for
 * recognizing "mission disk" players.
 * 
 * 13    6/23/98 4:18p Hoffoss
 * Fixed some bugs with AC release build.
 * 
 * 12    5/20/98 10:46p John
 * Added code that doesn't include duplicate filenames in any file list
 * functions.
 * 
 * 11    5/14/98 2:14p Lawrance2
 * Use filespec filtering for packfiles
 * 
 * 10    5/03/98 11:53a John
 * Fixed filename case mangling.
 * 
 * 9     5/02/98 11:06p Allender
 * correctly deal with pack pathnames
 * 
 * 8     5/01/98 11:41a Allender
 * Fixed bug with mission saving in Fred.
 * 
 * 7     5/01/98 10:21a John
 * Added code to find all pack files in all trees.   Added code to create
 * any directories that we write to.
 * 
 * 6     4/30/98 10:21p John
 * Added code to cleanup cfilesystem
 * 
 * 5     4/30/98 10:18p John
 * added source safe header
 *
 * $NoKeywords: $
 */

#include <string>
#include <cstring>
#include <algorithm>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifdef SDL_PLATFORM_WINDOWS
#include <io.h>
#include <shlwapi.h>	// for PathIsRealtive()
#endif

#include "pstypes.h"
#include "cfile.h"
#include "cfilesystem.h"
#include "localize.h"
#include "osregistry.h"

#include "embedvp.h"


typedef struct cf_pathtype {
	int		index;					// To verify that the CF_TYPE define is correctly indexed into this array
	const char	*path;					// Path relative to Freespace root, has ending backslash.
	const char	*extensions;			// Extensions used in this pathtype, separated by spaces
	int		parent_index;			// Index of this directory's parent.  Used for creating directories when writing.
} cf_pathtype;

// During cfile_init, verify that Pathtypes[n].index == n for each item
// Each path must have a valid parent that can be tracable all the way back to the root
// so that we can create directories when we need to.
//
static cf_pathtype Pathtypes[CF_MAX_PATH_TYPES]  = {
	// What type this is          Path                             Extensions              Parent type
	{ CF_TYPE_INVALID,				NULL,										NULL,				CF_TYPE_INVALID },
	// Root must be index 1!!
	{ CF_TYPE_ROOT,					"",											".mve .png",			CF_TYPE_ROOT	},
	{ CF_TYPE_DATA,					"Data",										".cfg .txt",			CF_TYPE_ROOT	},
	{ CF_TYPE_MAPS,					"Data" DIR_SEPARATOR_STR "Maps",			".pcx .ani .tga",		CF_TYPE_DATA	},
	{ CF_TYPE_TEXT,					"Data" DIR_SEPARATOR_STR "Text",			".txt .net",			CF_TYPE_DATA	},
#ifdef MAKE_FS1
	{ CF_TYPE_MISSIONS,				"Data" DIR_SEPARATOR_STR "Missions",		".fsm .fsc .ntl .ssv",	CF_TYPE_DATA	},
#else
	{ CF_TYPE_MISSIONS,				"Data" DIR_SEPARATOR_STR "Missions",		".fs2 .fc2 .ntl .ssv",	CF_TYPE_DATA	},
#endif
	{ CF_TYPE_MODELS,				"Data" DIR_SEPARATOR_STR "Models",			".pof",					CF_TYPE_DATA	},
	{ CF_TYPE_TABLES,				"Data" DIR_SEPARATOR_STR "Tables",			".tbl",					CF_TYPE_DATA	},
	{ CF_TYPE_SOUNDS,				"Data" DIR_SEPARATOR_STR "Sounds",			".wav",					CF_TYPE_DATA	},
	{ CF_TYPE_SOUNDS_8B22K,			"Data" DIR_SEPARATOR_STR "Sounds" DIR_SEPARATOR_STR "8b22k",			".wav",	CF_TYPE_SOUNDS	},
	{ CF_TYPE_SOUNDS_16B11K,		"Data" DIR_SEPARATOR_STR "Sounds" DIR_SEPARATOR_STR "16b11k",			".wav",	CF_TYPE_SOUNDS	},
	{ CF_TYPE_VOICE,				"Data" DIR_SEPARATOR_STR "Voice",			"",						CF_TYPE_DATA	},
#ifdef MAKE_FS1
	{ CF_TYPE_VOICE_BRIEFINGS,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Briefings",			".wav",	CF_TYPE_VOICE	},
#else
	{ CF_TYPE_VOICE_BRIEFINGS,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Briefing",			".wav",	CF_TYPE_VOICE	},
#endif
	{ CF_TYPE_VOICE_CMD_BRIEF,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Command_briefings",	".wav",	CF_TYPE_VOICE	},
#ifdef MAKE_FS1
	{ CF_TYPE_VOICE_DEBRIEFINGS,	"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Debriefings",		".wav",	CF_TYPE_VOICE	},
#else
	{ CF_TYPE_VOICE_DEBRIEFINGS,	"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Debriefing",		".wav",	CF_TYPE_VOICE	},
#endif
	{ CF_TYPE_VOICE_PERSONAS,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Personas",			".wav",	CF_TYPE_VOICE	},
	{ CF_TYPE_VOICE_SPECIAL,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Special",			".wav",	CF_TYPE_VOICE	},
	{ CF_TYPE_VOICE_TRAINING,		"Data" DIR_SEPARATOR_STR "Voice" DIR_SEPARATOR_STR "Training",			".wav",	CF_TYPE_VOICE	},
	{ CF_TYPE_MUSIC,				"Data" DIR_SEPARATOR_STR "Music",			".wav",					CF_TYPE_VOICE	},
	{ CF_TYPE_MOVIES,				"Data" DIR_SEPARATOR_STR "Movies",			".mve .msb",			CF_TYPE_DATA	},
	{ CF_TYPE_INTERFACE,			"Data" DIR_SEPARATOR_STR "Interface",		".pcx .ani .tga .png",	CF_TYPE_DATA	},
	{ CF_TYPE_FONT,					"Data" DIR_SEPARATOR_STR "Fonts",			".vf .ttf",				CF_TYPE_DATA	},
	{ CF_TYPE_EFFECTS,				"Data" DIR_SEPARATOR_STR "Effects",			".ani .pcx .neb .tga",	CF_TYPE_DATA	},
	{ CF_TYPE_HUD,					"Data" DIR_SEPARATOR_STR "Hud",				".ani .pcx .tga",		CF_TYPE_DATA	},
	{ CF_TYPE_PLAYER_MAIN,			"Data" DIR_SEPARATOR_STR "Players",			"",						CF_TYPE_DATA	},
	{ CF_TYPE_PLAYER_IMAGES_MAIN,	"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Images",			".pcx",	CF_TYPE_PLAYER_MAIN	},
#ifdef MAKE_FS1
	{ CF_TYPE_CACHE,				"Cache",									".clr .tmp",			CF_TYPE_DATA	}, 	//clr=cached color
	{ CF_TYPE_PLAYERS,				"Players",									".hcf",					CF_TYPE_ROOT	},
	{ CF_TYPE_SINGLE_PLAYERS,		"Players" DIR_SEPARATOR_STR "Single",		".plr .csg .css",		CF_TYPE_PLAYERS	},
	{ CF_TYPE_MULTI_PLAYERS,		"Players" DIR_SEPARATOR_STR "Multi",		".plr",					CF_TYPE_PLAYERS	},
#else
	{ CF_TYPE_CACHE,				"Data" DIR_SEPARATOR_STR "Cache",			".clr .tmp",			CF_TYPE_DATA	}, 	//clr=cached color
	{ CF_TYPE_PLAYERS,				"Data" DIR_SEPARATOR_STR "Players",			".hcf",					CF_TYPE_DATA	},
	{ CF_TYPE_SINGLE_PLAYERS,		"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Single",			".plr .csg .css",	CF_TYPE_PLAYERS	},
	{ CF_TYPE_MULTI_PLAYERS,		"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Multi",			".plr",	CF_TYPE_DATA	},
#endif
	{ CF_TYPE_MULTI_CACHE,			"Data" DIR_SEPARATOR_STR "MultiData",		".pcx .fs2",			CF_TYPE_DATA	},
	{ CF_TYPE_CONFIG,				"Data" DIR_SEPARATOR_STR "Config",			".cfg",					CF_TYPE_DATA	},
	{ CF_TYPE_SQUAD_IMAGES_MAIN,	"Data" DIR_SEPARATOR_STR "Players" DIR_SEPARATOR_STR "Squads",			".pcx",	CF_TYPE_DATA	},
	{ CF_TYPE_DEMOS,				"Data" DIR_SEPARATOR_STR "Demos",			".fsd",					CF_TYPE_DATA	},
	{ CF_TYPE_CBANIMS,				"Data" DIR_SEPARATOR_STR "CBAnims",			".ani",					CF_TYPE_DATA	},
	{ CF_TYPE_INTEL_ANIMS,			"Data" DIR_SEPARATOR_STR "IntelAnims",		".ani",					CF_TYPE_DATA	},
	{ CF_TYPE_SHADERS,				"Data" DIR_SEPARATOR_STR "Shaders",			".glsl",				CF_TYPE_DATA	},
};


#define CF_ROOTTYPE_PATH 0
#define CF_ROOTTYPE_PACK 1
#define CF_ROOTTYPE_EMBED 2

static std::string Cfile_root_dir;
static std::string Cfile_exec_dir;
static std::string Cfile_user_dir;

//  Created by:
//    specifying hard drive tree
//    searching for pack files on hard drive		// Found by searching all known paths
//    specifying cd-rom tree
//    searching for pack files on CD-rom tree
typedef struct cf_root {
	char				path[CF_MAX_PATHNAME_LENGTH];		// Contains something like c:\projects\freespace or c:\projects\freespace\freespace.vp
	int				roottype;								// CF_ROOTTYPE_PATH  = Path, CF_ROOTTYPE_PACK =Pack file
} cf_root;

// convenient type for sorting (see cf_build_pack_list())
typedef struct cf_root_sort { 
	char				path[CF_MAX_PATHNAME_LENGTH];
	int				roottype;
	int				cf_type;
} cf_root_sort;

#define CF_NUM_ROOTS_PER_BLOCK   32
#define CF_MAX_ROOT_BLOCKS			256				// Can store 32*256 = 8192 Roots
#define CF_MAX_ROOTS					(CF_NUM_ROOTS_PER_BLOCK * CF_MAX_ROOT_BLOCKS)

typedef struct cf_root_block {
	cf_root			roots[CF_NUM_ROOTS_PER_BLOCK];
} cf_root_block;

static int Num_roots = 0;
static cf_root_block  *Root_blocks[CF_MAX_ROOT_BLOCKS];
  

// Created by searching all roots in order.   This means Files is then sorted by precedence.
typedef struct cf_file {
	std::string	name_ext;								// Filename and extension
	std::string	subpath;								// subdirectory where file was found
	int		root_index;										// Where in Roots this is located
	int		pathtype_index;								// Where in Paths this is located
	time_t	write_time;										// When it was last written
	int		size;												// How big it is in bytes
	int		pack_offset;									// For pack files, where it is at.   0 if not in a pack file.  This can be used to tell if in a pack file.
} cf_file;

#define CF_NUM_FILES_PER_BLOCK   256
#define CF_MAX_FILE_BLOCKS			128						// Can store 256*128 = 32768 files

typedef struct cf_file_block {
	cf_file						files[CF_NUM_FILES_PER_BLOCK];
} cf_file_block;

static int Num_files = 0;
static cf_file_block  *File_blocks[CF_MAX_FILE_BLOCKS];


// Return a pointer to to file 'index'.
cf_file *cf_get_file(int index)
{
	int block = index / CF_NUM_FILES_PER_BLOCK;
	int offset = index % CF_NUM_FILES_PER_BLOCK;

	return &File_blocks[block]->files[offset];
}

// Create a new file and return a pointer to it.
cf_file *cf_create_file()
{
	int block = Num_files / CF_NUM_FILES_PER_BLOCK;
	int offset = Num_files % CF_NUM_FILES_PER_BLOCK;
	
	if ( File_blocks[block] == NULL )	{
		File_blocks[block] = new(std::nothrow) cf_file_block;
		SDL_assert( File_blocks[block] != NULL);
	}

	Num_files++;

	return &File_blocks[block]->files[offset];
}

extern int cfile_inited;

// Create a new root and return a pointer to it.  The structure is assumed unitialized.
cf_root *cf_get_root(int n)
{
	int block = n / CF_NUM_ROOTS_PER_BLOCK;
	int offset = n % CF_NUM_ROOTS_PER_BLOCK;

	if (!cfile_inited)
		return NULL;

	return &Root_blocks[block]->roots[offset];
}


// Create a new root and return a pointer to it.  The structure is assumed unitialized.
cf_root *cf_create_root()
{
	int block = Num_roots / CF_NUM_ROOTS_PER_BLOCK;
	int offset = Num_roots % CF_NUM_ROOTS_PER_BLOCK;
	
	if ( Root_blocks[block] == NULL )	{
		Root_blocks[block] = (cf_root_block *)malloc( sizeof(cf_root_block) );
		SDL_assert(Root_blocks[block] != NULL);
	}

	Num_roots++;

	return &Root_blocks[block]->roots[offset];
}

static bool cf_build_glob_pattern(std::string &pattern, int pathtype, const char *filter)
{
	if ( !filter || !CF_TYPE_SPECIFIED(pathtype) ) {
		Int3();
		pattern.clear();

		return false;
	}

	pattern = Pathtypes[pathtype].path;

	// SDL_GlobDirectory() requires directories have '/' separators
	if (DIR_SEPARATOR_CHAR == '\\') {
		std::replace(pattern.begin(), pattern.end(), '\\', '/');
	}

	if (pathtype != CF_TYPE_ROOT) {
		pattern += '/';
	}

	pattern += filter;

	return true;
}

// return the # of packfiles which exist
int cf_get_packfile_count(cf_root *root)
{
	int i;
	int packfile_count = 0;
	int count;
	std::string pattern;

	// count up how many packfiles we're gonna have
	for (i=CF_TYPE_ROOT; i<CF_MAX_PATH_TYPES; i++ )	{
		cf_build_glob_pattern(pattern, i, "*.vp");

		auto results = SDL_GlobDirectory(root->path, pattern.c_str(), SDL_GLOB_CASEINSENSITIVE, &count);

		if (results) {
			SDL_free(results);
		}

		packfile_count += count;
	}

	return packfile_count;
}

// packfile sort function
int cf_packfile_sort_func(const void *elem1, const void *elem2)
{
	cf_root_sort *r1, *r2;
	r1 = (cf_root_sort*)elem1;
	r2 = (cf_root_sort*)elem2;

	// if the 2 directory types are the same, do a string compare
	if(r1->cf_type == r2->cf_type){
		return SDL_strcasecmp(r1->path, r2->path);
	}

	// otherwise return them in order of CF_TYPE_* precedence
	return (r1->cf_type < r2->cf_type) ? -1 : 1;
}

// Go through a root and look for pack files
void cf_build_pack_list( cf_root *root )
{
	int i;
	cf_root_sort *temp_roots_sort, *rptr_sort;
	int temp_root_count, root_index;
	SDL_PathInfo pinfo;
	std::string pattern;
	char rpath[MAX_PATH_LEN];

	// determine how many packfiles there are
	temp_root_count = cf_get_packfile_count(root);
	if(temp_root_count <= 0){
		return;
	}

	// allocate a temporary array of temporary roots so we can easily sort them
	temp_roots_sort = (cf_root_sort*)malloc(sizeof(cf_root_sort) * temp_root_count);
	if(temp_roots_sort == NULL){
		Int3();
		return;
	}

	// now just setup all the root info
	root_index = 0;
	for (i=CF_TYPE_ROOT; i<CF_MAX_PATH_TYPES; i++ )	{
		cf_build_glob_pattern(pattern, i, "*.vp");

		auto results = SDL_GlobDirectory(root->path, pattern.c_str(), SDL_GLOB_CASEINSENSITIVE, nullptr);

		if ( !results ) {
			continue;
		}

		for (int ridx = 0; results[ridx]; ridx++) {
			SDL_snprintf(rpath, SDL_arraysize(rpath), "%s%s", root->path, results[ridx]);

			if ( !SDL_GetPathInfo(rpath, &pinfo) ) {
				continue;
			}

			if (pinfo.type != SDL_PATHTYPE_FILE) {
				continue;
			}

			rptr_sort = &temp_roots_sort[root_index++];

			SDL_strlcpy(rptr_sort->path, rpath, SDL_arraysize(rptr_sort->path));
			rptr_sort->roottype = CF_ROOTTYPE_PACK;
			rptr_sort->cf_type = i;
		}

		SDL_free(results);
	}

	// these should always be the same
	SDL_assert(root_index == temp_root_count);

	// sort tht roots
	qsort(temp_roots_sort,  temp_root_count, sizeof(cf_root_sort), cf_packfile_sort_func);

	// now insert them all into the real root list properly
	cf_root *new_root;
	for(i=0; i<temp_root_count; i++){		
		new_root = cf_create_root();
		SDL_strlcpy( new_root->path, root->path, SDL_arraysize(new_root->path) );

		// mwa -- 4/2/98 put in the next 2 lines because the path name needs to be there
		// to find the files.
		SDL_strlcpy(new_root->path, temp_roots_sort[i].path, SDL_arraysize(new_root->path));
		new_root->roottype = CF_ROOTTYPE_PACK;		
	}

	// free up the temp list
	free(temp_roots_sort);
}


// Builds a list of special roots for GOG/Steam compatibility
//
// These installs have numbered data folders (data1, data2, data3) which represent
// the CDs. Some of those files will be duplicates, some will not, but we've got to
// index them as normal roots to catch the full installation of files.
static void cf_build_root_list_special()
{
	for (int i = 0; i < Num_roots; ++i) {
		const auto root = cf_get_root(i);

		if (root->roottype == CF_ROOTTYPE_PATH) {
			auto results = SDL_GlobDirectory(root->path, "data?", SDL_GLOB_CASEINSENSITIVE, nullptr);

			if ( !results ) {
				continue;
			}

			for (int ridx = 0; results[ridx]; ridx++) {
				// add special root
				auto sr = cf_create_root();

				SDL_snprintf(sr->path, SDL_arraysize(sr->path), "%s%s%c",
							 root->path, results[ridx], DIR_SEPARATOR_CHAR);

				sr->roottype = CF_ROOTTYPE_PATH;

				// then check any VP files under it
				cf_build_pack_list(sr);
			}

			SDL_free(results);
		}
	}
}

void cf_build_root_list(const char *extras_dir)
{
	Num_roots = 0;

	cf_root	*root;

	// ================================================================
	// have user's writable directory as default for loading and saving files
	root = cf_create_root();
	SDL_strlcpy( root->path, Cfile_user_dir.c_str(), SDL_arraysize(root->path) );
	root->roottype = CF_ROOTTYPE_PATH;

	//======================================================
	// then check any VP files under the directory.
	cf_build_pack_list(root);

	//======================================================
	// Next, use the executable's directory for game data. This could be inside
	// the .app bundle on macOS or inside the AppImage root on Linux.
	root = cf_create_root();
	SDL_strlcpy( root->path, Cfile_root_dir.c_str(), SDL_arraysize(root->path) );
	root->roottype = CF_ROOTTYPE_PATH;

	//======================================================
	// then check any VP files under the directory.
	cf_build_pack_list(root);

	//======================================================
	// Next we check the *actual* executable directory, in case we're running from
	// an installation. Gets skipped if it matches Cfile_root_dir or doesn't have
	// any .vp files in it.
	if ( !Cfile_exec_dir.empty() ) {
		root = cf_create_root();
		SDL_strlcpy( root->path, Cfile_exec_dir.c_str(), SDL_arraysize(root->path) );
		root->roottype = CF_ROOTTYPE_PATH;

		//======================================================
		// then check any VP files under the directory.
		cf_build_pack_list(root);
	}


	//======================================================
	// Check the real CD if one...
	if ( extras_dir && SDL_strlen(extras_dir) )	{
		root = cf_create_root();
		SDL_strlcpy( root->path, extras_dir, SDL_arraysize(root->path) );
		root->roottype = CF_ROOTTYPE_PATH;

		//======================================================
		// Next, check any VP files in the CD-ROM directory.
		cf_build_pack_list(root);
	}

	//======================================================
	// We also need to handle the GOG/Steam setup they use for additional CDs
	cf_build_root_list_special();

	//======================================================
	// And lastly, check embedded VP archive
	if (embedvp::size > 0) {
		root = cf_create_root();
		SDL_strlcpy(root->path, "embed", SDL_arraysize(root->path));
		root->roottype = CF_ROOTTYPE_EMBED;
	}

}

// Given a lower case list of file extensions 
// separated by spaces, return zero if ext is
// not in the list.
int is_ext_in_list( const char *ext_list, char *ext )
{
	if ( SDL_strcasestr(ext_list, ext) ) {
		return 1;
	}	

	return 0;
}

void cf_search_root_path(int root_index)
{
	int i;
	SDL_PathInfo pinfo;
	std::string pattern;
	char rpath[MAX_PATH_LEN];

	cf_root *root = cf_get_root(root_index);

	mprintf(( "Searching root '%s'\n", root->path ));

	for (i=CF_TYPE_ROOT; i<CF_MAX_PATH_TYPES; i++ )	{
		switch (i) {
			case CF_TYPE_SINGLE_PLAYERS:
			case CF_TYPE_MULTI_PLAYERS:
			case CF_TYPE_MULTI_CACHE:
				// avoid indexting locations with a lot of writing
				continue;

			default:
				break;
		}

		cf_build_glob_pattern(pattern, i, "*");

		auto results = SDL_GlobDirectory(root->path, pattern.c_str(), SDL_GLOB_CASEINSENSITIVE, nullptr);

		if ( !results ) {
			continue;
		}

		for (int ridx = 0; results[ridx]; ridx++) {
			SDL_snprintf(rpath, SDL_arraysize(rpath), "%s%s", root->path, results[ridx]);

			if ( !SDL_GetPathInfo(rpath, &pinfo) ) {
				continue;
			}

			if (pinfo.type != SDL_PATHTYPE_FILE) {
				continue;
			}

			char *ext = SDL_strrchr(results[ridx], '.' );

			if (ext && is_ext_in_list(Pathtypes[i].extensions, ext)) {
				// Found a file!!!!
				cf_file *file = cf_create_file();

				file->name_ext = results[ridx];

				auto pos = file->name_ext.rfind(DIR_SEPARATOR_CHAR);

				// if result is in a subfolder then move that info to file->subpath
				if (pos != std::string::npos) {
					file->subpath = file->name_ext.substr(0, pos+1);	// include trailing slash
					file->name_ext.erase(0, pos+1);
				}

				file->root_index = root_index;
				file->pathtype_index = i;

				file->write_time = static_cast<time_t>(pinfo.modify_time);
				file->size = static_cast<int>(pinfo.size);

				file->pack_offset = 0;			// Mark as a non-packed file

				// mprintf(( "Found file '%s'\n", file->name_ext ));
			}
		}

		SDL_free(results);
	}
}

static const int32_t VP_ID = 0x50565056;

typedef struct VP_FILE_HEADER {
	int id;
	int version;
	int index_offset;
	int num_files;
} VP_FILE_HEADER;

typedef struct VP_FILE {
	int	offset;
	int	size;
	char	filename[32];
	fs_time_t write_time;
} VP_FILE;

SDL_COMPILE_TIME_ASSERT(VP_FILE_HEADER, sizeof(VP_FILE_HEADER) == 16);
SDL_COMPILE_TIME_ASSERT(VP_FILE, sizeof(VP_FILE) == 44);

void cf_search_root_pack(int root_index)
{
	size_t rc = 0;
	int i;
	SDL_IOStream *fp = nullptr;

	cf_root *root = cf_get_root(root_index);

	//mprintf(( "Searching root pack '%s'\n", root->path ));

	// Open data

	if (root->roottype == CF_ROOTTYPE_PACK) {
		fp = SDL_IOFromFile(root->path, "rb");
	} else if (root->roottype == CF_ROOTTYPE_EMBED) {
		fp = SDL_IOFromConstMem(embedvp::data, embedvp::size);
	}

	if (!fp) {
		return;
	}

	// Read and validate the file header

	VP_FILE_HEADER VP_header;

	rc = SDL_ReadIO(fp, &VP_header, sizeof(VP_header));

	if (rc != sizeof(VP_header)) {
		SDL_CloseIO(fp);
		return;
	}

	VP_header.id = INTEL_INT(VP_header.id);
	VP_header.version = INTEL_INT(VP_header.version);
	VP_header.index_offset = INTEL_INT(VP_header.index_offset);
	VP_header.num_files = INTEL_INT(VP_header.num_files);

	// verify ID
	if (VP_header.id != VP_ID) {
		SDL_CloseIO(fp);
		return;
	}

	// verify size
	if (SDL_GetIOSize(fp) != (VP_header.index_offset + (VP_header.num_files * sizeof(VP_FILE)))) {
		SDL_CloseIO(fp);
		return;
	}

	// Read index info
	SDL_SeekIO(fp, VP_header.index_offset, SDL_IO_SEEK_SET);

	char search_path[CF_MAX_PATHNAME_LENGTH];

	SDL_strlcpy( search_path, "", SDL_arraysize(search_path) );
	
	// Go through all the files
	for (i=0; i<VP_header.num_files; i++ )	{
		VP_FILE find;

		rc = SDL_ReadIO(fp, &find, sizeof(VP_FILE));

		if (rc != sizeof(VP_FILE)) {
			break;
		}

		find.offset = INTEL_INT(find.offset);
		find.size = INTEL_INT(find.size);
		find.write_time = INTEL_INT(find.write_time);

		if ( find.size == 0 )	{
			if ( !SDL_strcasecmp( find.filename, ".." ))	{
				auto l = SDL_strlen(search_path);
				char *p = &search_path[l-1];
				while( (p > search_path) && (*p != DIR_SEPARATOR_CHAR) )	{
					p--;
				}
				*p = 0;
			} else {
				if ( SDL_strlen(search_path)	)	{
					SDL_strlcat( search_path,	DIR_SEPARATOR_STR, SDL_arraysize(search_path) );
				}
				SDL_strlcat( search_path, find.filename, SDL_arraysize(search_path) );
			}

			//mprintf(( "Current dir = '%s'\n", search_path ));
		} else {
	
			int j;
			for (j=CF_TYPE_ROOT; j<CF_MAX_PATH_TYPES; j++ )	{

				if ( !SDL_strcasecmp( search_path, Pathtypes[j].path ))	{

					char *ext = SDL_strrchr( find.filename, '.' );
					if ( ext )	{
						if ( is_ext_in_list( Pathtypes[j].extensions, ext ) )	{
							// Found a file!!!!
							cf_file *file = cf_create_file();
							
							file->name_ext = find.filename;
							file->root_index = root_index;
							file->pathtype_index = j;
							file->write_time = find.write_time;
							file->size = find.size;
							file->pack_offset = find.offset;			// Mark as a non-packed file

							//mprintf(( "Found pack file '%s'\n", file->name_ext ));
						}
					}
					

				}
			}

		}
	}

	SDL_CloseIO(fp);
}


void cf_build_file_list()
{
	int i;

	Num_files = 0;

	// For each root, find all files...
	for (i=0; i<Num_roots; i++) {
		cf_root	*root = cf_get_root(i);
		if ( root->roottype == CF_ROOTTYPE_PATH )	{
			cf_search_root_path(i);
		} else if (root->roottype == CF_ROOTTYPE_PACK || root->roottype == CF_ROOTTYPE_EMBED) {
			cf_search_root_pack(i);
		}
	}

}


void cf_build_secondary_filelist(const char *extras_dir)
{
	int i;

	// Assume no files
	Num_roots = 0;
	Num_files = 0;

	// Init the path types
	for (i=0; i<CF_MAX_PATH_TYPES; i++ )	{
		SDL_assert( Pathtypes[i].index == i );
	}
	
	// Init the root blocks
	for (i=0; i<CF_MAX_ROOT_BLOCKS; i++ )	{
		Root_blocks[i] = NULL;
	}

	// Init the file blocks	
	for (i=0; i<CF_MAX_FILE_BLOCKS; i++ )	{
		File_blocks[i] = NULL;
	}

	mprintf(( "Building file index...\n" ));
	
	// build the list of searchable roots
	cf_build_root_list(extras_dir);

	// build the list of files themselves
	cf_build_file_list();

	mprintf(( "Found %d roots and %d files.\n", Num_roots, Num_files ));

	// it should be safe to clear Cfile_root_dir/Cfile_exec_dir now
	// (DO NOT CLEAR Cfile_user_dir HERE!!)
	Cfile_root_dir.clear();
	Cfile_root_dir.shrink_to_fit();
	Cfile_exec_dir.clear();
	Cfile_exec_dir.shrink_to_fit();
}

void cf_free_secondary_filelist()
{
	int i;

	// Free the root blocks
	for (i=0; i<CF_MAX_ROOT_BLOCKS; i++ )	{
		if ( Root_blocks[i] )	{
			free( Root_blocks[i] );
			Root_blocks[i] = NULL;
		}
	}
	Num_roots = 0;

	// Init the file blocks	
	for (i=0; i<CF_MAX_FILE_BLOCKS; i++ )	{
		if ( File_blocks[i] )	{
			delete File_blocks[i];
			File_blocks[i] = NULL;
		}
	}
	Num_files = 0;
}

static bool is_absolute_path(const char *path)
{
	if ( !path || !strlen(path) ) {
		return false;
	}

#ifdef SDL_PLATFORM_WINDOWS
	return (PathIsRelative(path) == FALSE);
#else
	return (*path == '/');
#endif
}

// Searches for a file.   Follows all rules and precedence and searches
// CD's and pack files.
// Input:  filespace   - Filename & extension
//         pathtype    - See CF_TYPE_ defines in CFILE.H
// Output: pack_filename - Absolute path and filename of this file.   Could be a packfile or the actual file.
//         size        - File size
//         offset      - Offset into pack file.  0 if not a packfile.
// Returns: If not found returns 0.
int cf_find_file_location( const char *filespec, int pathtype, char *pack_filename, int *size, int *offset, bool localize )
{
	SDL_PathInfo pinfo;
	int i;

	SDL_assert(filespec && SDL_strlen(filespec));

	// see if we have something other than just a filename
	// our current rules say that any file that specifies a direct
	// path will try to be opened on that path.  If that open
	// fails, then we will open the file based on the extension
	// of the file

	// NOTE: full path should also include localization, if so desired
	if ( is_absolute_path(filespec) ) {		// do we have a full path already?
		if (SDL_GetPathInfo(filespec, &pinfo)) {
			if ( size ) *size = static_cast<int>(pinfo.size);
			if ( offset ) *offset = 0;
			if ( pack_filename ) {
				SDL_strlcpy( pack_filename, filespec, MAX_PATH_LEN );
			}

			return 1;
		}

		return 0;		// If they give a full path, fail if not found.
	}

	// Search the hard drive for files first.
	// NOTE: This is a case-sensitive check!!
	int num_search_dirs = 0;
	int search_order[CF_MAX_PATH_TYPES];

	if ( CF_TYPE_SPECIFIED(pathtype) )	{
		search_order[num_search_dirs++] = pathtype;
	} else {
		for (i=CF_TYPE_ROOT; i<CF_MAX_PATH_TYPES; i++)	{
			search_order[num_search_dirs++] = i;
		}
	}

	for (i=0; i<num_search_dirs; i++ )	{
		char longname[MAX_PATH_LEN];

		cf_create_default_path_string( longname, search_order[i], filespec, localize );

		if (SDL_GetPathInfo(longname, &pinfo))	{
			if ( size ) *size = static_cast<int>(pinfo.size);
			if ( offset ) *offset = 0;
			if ( pack_filename ) {
				SDL_strlcpy( pack_filename, longname, MAX_PATH_LEN );
			}

			return 1;
		}
	} 

	// Search indexed files from filesystem, pak files, and CD-ROM.

		for (i=0; i<Num_files; i++ )	{
			cf_file * f = cf_get_file(i);

			// only search paths we're supposed to...
			if ( (pathtype != CF_TYPE_ANY) && (pathtype != f->pathtype_index)  )	{
				continue;
			}

			if (localize) {
				// create localized filespec
				char loc_filespec[MAX_PATH_LEN];
				SDL_strlcpy(loc_filespec, filespec, SDL_arraysize(loc_filespec));
				lcl_add_dir_to_path_with_filename(loc_filespec, sizeof(loc_filespec));
			
				if ( !SDL_strcasecmp(loc_filespec, f->name_ext.c_str()) )	{
					if ( size ) *size = f->size;
					if ( offset ) *offset = f->pack_offset;
					if ( pack_filename ) {
						cf_root * r = cf_get_root(f->root_index);

						SDL_strlcpy( pack_filename, r->path, MAX_PATH_LEN );
						if ( f->pack_offset < 1 )	{
							SDL_strlcat( pack_filename, f->subpath.c_str(), MAX_PATH_LEN );
							SDL_strlcat( pack_filename, f->name_ext.c_str(), MAX_PATH_LEN );
						}
					}				
					return 1;		
				}
			}

			// file either not localized or localized version not found
			if ( !SDL_strcasecmp(filespec, f->name_ext.c_str()) )	{
				if ( size ) *size = f->size;
				if ( offset ) *offset = f->pack_offset;
				if ( pack_filename ) {
					cf_root * r = cf_get_root(f->root_index);

					SDL_strlcpy( pack_filename, r->path, MAX_PATH_LEN );
					if ( f->pack_offset < 1 )	{
						SDL_strlcat( pack_filename, f->subpath.c_str(), MAX_PATH_LEN );
						SDL_strlcat( pack_filename, f->name_ext.c_str(), MAX_PATH_LEN );
					}
				}				
				return 1;		
			}
		}
	
	return 0;
}


// Returns true if filename matches filespec, else zero if not
static int cf_matches_spec(const char *filespec, const char *filename)
{
	const char *src_ext, *dst_ext;

	src_ext = SDL_strrchr(filespec, '.');
	if (!src_ext)
		return 1;
	if (*src_ext == '*')
		return 1;

	dst_ext = SDL_strrchr(filename, '.');
	if (!dst_ext)
		return 1;
	
	return !SDL_strcasecmp(dst_ext, src_ext);
}

int (*Get_file_list_filter)(const char *filename) = NULL;
int Skip_packfile_search = 0;

static int cf_file_already_in_list( int num_files, char **list, const char *filename )
{
	int i;

	char name_no_extension[MAX_PATH_LEN];

	SDL_strlcpy(name_no_extension, filename, SDL_arraysize(name_no_extension));
	char *p = SDL_strrchr( name_no_extension, '.' );
	if ( p ) *p = 0;

	for (i=0; i<num_files; i++ )	{
		if ( !SDL_strcasecmp(list[i], name_no_extension ) )	{
			// Match found!
			return 1;
		}
	}
	// Not found
	return 0;
}

// An alternative cf_get_file_list(), dynamic list version.
// This one has a 'type', which is a CF_TYPE_* value.  Because this specifies the directory
// location, 'filter' only needs to be the filter itself, with no path information.
// See above descriptions of cf_get_file_list() for more information about how it all works.
int cf_get_file_list( int max, char **list, int pathtype, const char *filter, int sort, file_list_info *info )
{
	char *ptr;
	int i, num_files = 0, own_flag = 0;
	size_t l;

	if ((max < 1) || (filter == nullptr) || !CF_TYPE_SPECIFIED(pathtype)) {
		Get_file_list_filter = NULL;
		return 0;
	}

	SDL_assert(list);

	if (!info && (sort == CF_SORT_TIME)) {
		info = (file_list_info *) malloc(sizeof(file_list_info) * max);
		own_flag = 1;
	}

	char filespec[MAX_PATH_LEN];
	char rpath[MAX_PATH_LEN];
	SDL_PathInfo pinfo;

	// Search the default directories
	// NOTE: This is a case-sensitive subfolder check!!
	cf_create_default_path_string(filespec, pathtype, nullptr);

	auto results = SDL_GlobDirectory(filespec, filter, SDL_GLOB_CASEINSENSITIVE, nullptr);

	if (results) {
		for (int ridx = 0; results[ridx]; ridx++) {
			SDL_snprintf(rpath, SDL_arraysize(rpath), "%s%s", filespec, results[ridx]);
			
			if ( !SDL_GetPathInfo(rpath, &pinfo) ) {
				continue;
			}
			
			if (pinfo.type != SDL_PATHTYPE_FILE) {
				continue;
			}
			
			if ( !Get_file_list_filter || (*Get_file_list_filter)(results[ridx]) ) {
				ptr = SDL_strrchr(results[ridx], '.');
				
				if (ptr) {
					l = ptr - results[ridx];
				} else {
					l = SDL_strlen(results[ridx]);
				}
				
				list[num_files] = reinterpret_cast<char *>(malloc(l + 1));
				SDL_strlcpy(list[num_files], results[ridx], l+1);
				
				if (info) {
					info[num_files].write_time = static_cast<time_t>(pinfo.modify_time);
				}
				
				++num_files;
			}
		}
		
		SDL_free(results);
	}

	// Search indexed filesystem and all the packfiles and CD.
	if ( !Skip_packfile_search )	{
		for (i=0; i<Num_files; i++ )	{
			cf_file * f = cf_get_file(i);

			// only search paths we're supposed to...
			if ( (pathtype != CF_TYPE_ANY) && (pathtype != f->pathtype_index)  )	{
				continue;
			}

			if (num_files >= max)
				break;

			if ( !cf_matches_spec(filter, f->name_ext.c_str()) ) {
				continue;
			}

			if ( cf_file_already_in_list(num_files,list, f->name_ext.c_str()) ) {
				continue;
			}

			if ( !Get_file_list_filter || (*Get_file_list_filter)(f->name_ext.c_str()) ) {

				//mprintf(( "Found '%s' in root %d path %d\n", f->name_ext, f->root_index, f->pathtype_index ));

				auto pos = f->name_ext.rfind('.');

				if (pos != std::string::npos) {
					l = pos;
				} else {
					l = f->name_ext.length();
				}

				list[num_files] = (char *)malloc(l + 1);
				SDL_strlcpy(list[num_files], f->name_ext.c_str(), l+1);

				if (info)	{
					info[num_files].write_time = f->write_time;
				}

				num_files++;
			}

		}
	}


	if (sort != CF_SORT_NONE)	{
		cf_sort_filenames( num_files, list, sort, info );
	}

	if (own_flag)	{
		free(info);
	}

	Get_file_list_filter = NULL;
	return num_files;
}

static int cf_file_already_in_list_preallocated( int num_files, char arr[][MAX_FILENAME_LEN], const char *filename )
{
	int i;

	char name_no_extension[MAX_PATH_LEN];

	SDL_strlcpy(name_no_extension, filename, SDL_arraysize(name_no_extension));
	char *p = SDL_strrchr( name_no_extension, '.' );
	if ( p ) *p = 0;

	for (i=0; i<num_files; i++ )	{
		if ( !SDL_strcasecmp(arr[i], name_no_extension ) )	{
			// Match found!
			return 1;
		}
	}
	// Not found
	return 0;
}

// An alternative cf_get_file_list(), fixed array version.
// This one has a 'type', which is a CF_TYPE_* value.  Because this specifies the directory
// location, 'filter' only needs to be the filter itself, with no path information.
// See above descriptions of cf_get_file_list() for more information about how it all works.
int cf_get_file_list_preallocated( int max, char arr[][MAX_FILENAME_LEN], char **list, int pathtype, const char *filter, int sort, file_list_info *info )
{
	int i, num_files = 0, own_flag = 0;

	if ((max < 1) || (filter == nullptr) || !CF_TYPE_SPECIFIED(pathtype)) {
		Get_file_list_filter = NULL;
		return 0;
	}

	if (list) {
		for (i=0; i<max; i++)	{
			list[i] = arr[i];
		}
	} else {
		sort = CF_SORT_NONE;  // sorting of array directly not supported.  Sorting done on list only
	}

	if (!info && (sort == CF_SORT_TIME)) {
		info = (file_list_info *) malloc(sizeof(file_list_info) * max);
		own_flag = 1;
	}

	char filespec[MAX_PATH_LEN];
	char rpath[MAX_PATH_LEN];
	SDL_PathInfo pinfo;

	// Search the default directories
	// NOTE: This is a case-sensitive subfolder check!!
	cf_create_default_path_string(filespec, pathtype, nullptr);

	auto results = SDL_GlobDirectory(filespec, filter, SDL_GLOB_CASEINSENSITIVE, nullptr);

	if (results) {
		for (int ridx = 0; results[ridx]; ridx++) {
			if (num_files >= max) {
				break;
			}

			SDL_snprintf(rpath, SDL_arraysize(rpath), "%s%s", filespec, results[ridx]);
			
			if ( !SDL_GetPathInfo(rpath, &pinfo) ) {
				continue;
			}
			
			if (pinfo.type != SDL_PATHTYPE_FILE) {
				continue;
			}
			
			if ( !Get_file_list_filter || (*Get_file_list_filter)(results[ridx]) ) {
				SDL_strlcpy(arr[num_files], results[ridx], MAX_FILENAME_LEN);

				char *ptr = SDL_strrchr(arr[num_files], '.');
				if (ptr) {
					*ptr = 0;
				}

				if (info) {
					info[num_files].write_time = static_cast<time_t>(pinfo.modify_time);
				}

				++num_files;
			}
		}
		
		SDL_free(results);
	}

	// Search all the packfiles and CD.
	if ( !Skip_packfile_search )	{
		for (i=0; i<Num_files; i++ )	{
			cf_file * f = cf_get_file(i);

			// only search paths we're supposed to...
			if ( (pathtype != CF_TYPE_ANY) && (pathtype != f->pathtype_index)  )	{
				continue;
			}

			if (num_files >= max)
				break;

			if ( !cf_matches_spec( filter,f->name_ext.c_str()))	{
				continue;
			}

			if ( cf_file_already_in_list_preallocated( num_files, arr, f->name_ext.c_str() ))	{
				continue;
			}

			if ( !Get_file_list_filter || (*Get_file_list_filter)(f->name_ext.c_str()) ) {

				//mprintf(( "Found '%s' in root %d path %d\n", f->name_ext, f->root_index, f->pathtype_index ));

				SDL_strlcpy(arr[num_files], f->name_ext.c_str(), MAX_FILENAME_LEN);
				char *ptr = strrchr(arr[num_files], '.');
				if ( ptr ) {
					*ptr = 0;
				}

				if (info)	{
					info[num_files].write_time = f->write_time;
				}

				num_files++;
			}

		}
	}

	if (sort != CF_SORT_NONE) {
		SDL_assert(list);
		cf_sort_filenames( num_files, list, sort, info );
	}

	if (own_flag)	{
		free(info);
	}

	Get_file_list_filter = NULL;
	return num_files;
}

// Returns the default storage path for files given a 
// particular pathtype.   In other words, the path to 
// the unpacked, non-cd'd, stored on hard drive path.
// If filename isn't null it will also tack the filename
// on the end, creating a completely valid filename.
// Input:   pathtype  - CF_TYPE_??
//          filename  - optional, if set, tacks the filename onto end of path.
// Output:  path      - Fully qualified pathname.
void cf_create_default_path_string( char *path, int pathtype, const char *filename, bool localize )
{
	if ( is_absolute_path(filename) ) {
		// Already has full path
		SDL_strlcpy( path, filename, MAX_PATH_LEN );
	} else {
		if ( !cfile_init_paths() ) {
			exit(EXIT_FAILURE);
		}

		SDL_assert(CF_TYPE_SPECIFIED(pathtype));

		SDL_strlcpy(path, Cfile_user_dir.c_str(), MAX_PATH_LEN);

		if (pathtype > CF_TYPE_ROOT) {
			SDL_strlcat(path, Pathtypes[pathtype].path, MAX_PATH_LEN);
			SDL_strlcat(path, DIR_SEPARATOR_STR, MAX_PATH_LEN);
		}

		// add filename
		if (filename) {
			SDL_strlcat(path, filename, MAX_PATH_LEN);

			// localize filename
			if (localize) {
				// create copy of path
				char temp_path[MAX_PATH_LEN];
				SDL_strlcpy(temp_path, path, SDL_arraysize(temp_path));

				// localize the path
				lcl_add_dir_to_path_with_filename(path, MAX_PATH_LEN);

				// verify localized path
				if ( !SDL_GetPathInfo(path, nullptr) ) {
					SDL_strlcpy(path, temp_path, SDL_arraysize(temp_path));
				}
			}
		}
	}
}

// returns true if packfile has been indexed by CFILE (case-insensitive search)
bool cf_has_packfile(const char *fn)
{
	cf_root *root;
	char vp_name[CF_MAX_PATHNAME_LENGTH];
	char *p;
	int i;

	if (fn == NULL) {
		return false;
	}

	if ( !SDL_strlen(fn) || (SDL_strlen(fn) < 4) ) {
		return false;
	}

	// add ".vp" if needed
	SDL_strlcpy(vp_name, fn, CF_MAX_PATHNAME_LENGTH);

	p = &vp_name[SDL_strlen(vp_name)-3];

	if ( SDL_strcasecmp(p, ".vp") ) {
		SDL_strlcat(vp_name, ".vp", CF_MAX_PATHNAME_LENGTH);
	}

	// now see if we have the packfile
	for (i = 0; i < Num_roots; i++) {
		root = cf_get_root(i);

		if (root->roottype != CF_ROOTTYPE_PACK) {
			continue;
		}

		p = SDL_strrchr(root->path, DIR_SEPARATOR_CHAR);

		if (p) {
			p++;

			if ( !SDL_strcasecmp(p, vp_name) ) {
				return true;
			}
		}
	}

	return false;
}

// determine if the given path is in a root directory (c:\  or  c:\freespace2.exe  or  c:\fred2.exe   etc)
static bool cfile_in_root_dir(const std::string &exe_path)
{
	int token_count = 0;
	char path_copy[MAX_PATH_LEN] = "";
	char *p;

	// bogus
	if (exe_path.empty()) {
		return true;
	}

	// copy the path
	SDL_strlcpy(path_copy, exe_path.c_str(), SDL_arraysize(path_copy));

	// count how many slashes there are in the path
	p = path_copy;

	while ((p = SDL_strchr(p, DIR_SEPARATOR_CHAR)) != nullptr) {
		++p;
		++token_count;
	}

	// root directory if we have <= 1 slash
	if(token_count <= 1){
		return true;
	}

	// not-root directory
	return false;
}

// checks for presence of root vp in path
static bool cfile_has_game_files(const std::string &path)
{
	int count = 0;

#if defined(FS1_DEMO)
	auto ignore = SDL_GlobDirectory(path.c_str(), "data/freespace.vp", SDL_GLOB_CASEINSENSITIVE, &count);

	if (ignore) {
		SDL_free(ignore);
	}

	return (count > 0);
#elif defined(MAKE_FS1)
	auto ignore = SDL_GlobDirectory(path.c_str(), "root.vp", SDL_GLOB_CASEINSENSITIVE, &count);

	if (ignore) {
		SDL_free(ignore);
	}

	return (count > 0);
#else
	// this is true for both full game and demo
	auto ignore = SDL_GlobDirectory(path.c_str(), "root_fs2.vp", SDL_GLOB_CASEINSENSITIVE, &count);

	if (ignore) {
		SDL_free(ignore);
	}

	return (count > 0);
#endif
}

// fill in Cfile_root_dir[] and Cfile_user_dir[]
// this can be called at any time, even before cfile_init()
//  returns: true on success, false on error
bool cfile_init_paths()
{
	if (cfile_inited || (!Cfile_root_dir.empty() && !Cfile_user_dir.empty())) {
		return true;
	}

#ifndef __EMSCRIPTEN__
	const char *t_path = SDL_GetBasePath();

	// make sure we have something
	if (t_path == NULL) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error trying to determine executable directory!", NULL);
		return false;
	}

	// size check
	if ( SDL_strlen(t_path) >= CF_MAX_PATHNAME_LENGTH ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Executable path is too long!", NULL);
		return false;
	}

	// set root directory
	Cfile_root_dir = t_path;

	// are we in a root directory?
	if ( cfile_in_root_dir(Cfile_root_dir) ) {
#ifndef MAKE_FS1
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Freespace2/Fred2 cannot be run from a drive root directory!", NULL);
#else
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Freespace/Fred cannot be run from a drive root directory!", NULL);
#endif
		return false;
	}

	// set exec directory, if:
	//	* it's in some sort of app bundle/container
	//	* it doesn't match Cfile_root_dir
	//	* it contains required .vp's
	// (Note: Cfile_exec_dir should be left empty if it fails required checks)
#if defined(SDL_PLATFORM_LINUX)
	auto appimage = SDL_getenv("APPIMAGE");

	if (appimage) {
		Cfile_exec_dir = appimage;

		auto pos = Cfile_exec_dir.rfind(DIR_SEPARATOR_CHAR);

		if (pos != std::string::npos) {
			Cfile_exec_dir.resize(pos+1);	// include path separator
		}

		if (cfile_in_root_dir(Cfile_exec_dir) ||
			!Cfile_exec_dir.compare(Cfile_root_dir) ||
			!cfile_has_game_files(Cfile_exec_dir) )
		{
			Cfile_exec_dir.clear();
			Cfile_exec_dir.shrink_to_fit();
		}
	}
#elif defined(SDL_PLATFORM_APPLE)
	// root should be in Resources of bundle by default
	auto app_pos = Cfile_root_dir.rfind(".app/Contents/Resources");

	if (app_pos != std::string::npos) {
		auto pos = Cfile_root_dir.rfind(DIR_SEPARATOR_CHAR, app_pos);

		Cfile_exec_dir = Cfile_root_dir.substr(0, pos+1);	// include path separator

		if (cfile_in_root_dir(Cfile_exec_dir) ||
			!Cfile_exec_dir.compare(Cfile_root_dir) ||
			!cfile_has_game_files(Cfile_exec_dir) )
		{
			Cfile_exec_dir.clear();
			Cfile_exec_dir.shrink_to_fit();
		}
	}
#endif

	// now for the user/pref directory, the writable location
	char *u_path = SDL_GetPrefPath(Osreg_company_name, Osreg_app_name);

	// make sure we have something
	if (u_path == NULL) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error trying to determine preferences directory!", NULL);
		return false;
	}

	// size check
	if ( SDL_strlen(u_path) >= CF_MAX_PATHNAME_LENGTH ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Preferences path is too long!", NULL);
		return false;
	}

	// set user/pref directory
	Cfile_user_dir = u_path;
	// free SDL copy
	SDL_free(u_path);
	u_path = nullptr;
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
	std::string pathname = Cfile_user_dir;
	pathname += Pathtypes[CF_TYPE_DATA].path;

	if ( !SDL_GetPathInfo(pathname.c_str(), nullptr) ) {
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

	return true;
}
