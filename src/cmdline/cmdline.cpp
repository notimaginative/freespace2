/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Cmdline/cmdline.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.5  2003/05/06 07:20:14  taylor
 * implement command line options
 *
 * Revision 1.4  2002/06/21 03:34:05  relnev
 * implemented a stub and fixed a path
 *
 * Revision 1.3  2002/06/09 04:41:15  relnev
 * added copyright header
 *
 * Revision 1.2  2002/05/07 03:16:43  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:08  root
 * Initial import.
 *
 * 
 * 8     8/26/99 8:51p Dave
 * Gave multiplayer TvT messaging a heavy dose of sanity. Cheat codes.
 * 
 * 7     7/15/99 3:07p Dave
 * 32 bit detection support. Mouse coord commandline.
 * 
 * 6     7/13/99 1:15p Dave
 * 32 bit support. Whee!
 * 
 * 5     6/22/99 9:37p Dave
 * Put in pof spewing.
 * 
 * 4     1/12/99 5:45p Dave
 * Moved weapon pipeline in multiplayer to almost exclusively client side.
 * Very good results. Bandwidth goes down, playability goes up for crappy
 * connections. Fixed object update problem for ship subsystems.
 * 
 * 3     11/17/98 11:12a Dave
 * Removed player identification by address. Now assign explicit id #'s.
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:48a Dave
 * 
 * 38    10/02/98 3:22p Allender
 * fix up the -connect option and fix the -port option
 * 
 * 37    9/15/98 4:04p Allender
 * added back in the -ip_addr command line switch because it needs to be
 * in the standalone server only executable
 * 
 * 36    9/14/98 11:52a Allender
 * don't use cfile
 * 
 * 35    9/14/98 11:28a Allender
 * support for server bashing of address when received from client.  Added
 * a cmdline.cfg file to process command line arguments from a file
 * 
 * 34    9/08/98 2:20p Allender
 * temporary code to force IP address to a specific value.
 * 
 * 33    8/20/98 5:30p Dave
 * Put in handy multiplayer logfile system. Now need to put in useful
 * applications of it all over the code.
 * 
 * 32    8/07/98 10:39a Allender
 * fixed debug standalone problem where stats would continually get sent
 * to tracker.  more debug code to help find stats problem
 * 
 * 31    7/24/98 11:14a Allender
 * start of new command line options for version 1.04
 * 
 * 30    5/21/98 1:50a Dave
 * Remove obsolete command line functions. Reduce shield explosion packets
 * drastically. Tweak PXO screen even more. Fix file xfer system so that
 * we can guarantee file uniqueness.
 * 
 * 29    5/18/98 9:10p Dave
 * Put in many new PXO features. Fixed skill level bashing in multiplayer.
 * Removed several old command line options. Put in network config files.
 * 
 * 28    5/09/98 7:16p Dave
 * Put in CD checking. Put in standalone host password. Made pilot into
 * popup scrollable.
 * 
 * 27    4/23/98 8:27p Allender
 * basic support for cutscene playback.  Into movie code in place.  Tech
 * room can view cutscenes stored in CDROM_dir variable
 * 
 * 26    4/09/98 5:43p Dave
 * Remove all command line processing from the demo. Began work fixing up
 * the new multi host options screen.
 * 
 * 25    4/02/98 11:40a Lawrance
 * check for #ifdef DEMO instead of #ifdef DEMO_RELEASE
 * 
 * 24    4/01/98 5:56p Dave
 * Fixed a messaging bug which caused msg_all mode in multiplayer not to
 * work. Compile out a host of multiplayer options not available in the
 * demo.
 * 
 * 23    3/14/98 2:48p Dave
 * Cleaned up observer joining code. Put in support for file xfers to
 * ingame joiners (observers or not). Revamped and reinstalled pseudo
 * lag/loss system.
 * 
 * 22    2/22/98 12:19p John
 * Externalized some strings
 * 
 * 21    1/31/98 4:32p Dave
 * Put in new support for VMT player validation, game logging in and game
 * logging out.
 * 
 * 20    12/10/97 4:45p Dave
 * Added in more detailed support for multiplayer packet lag/loss. Fixed
 * some multiplayer stuff. Added some controls to the standalone.
 * 
 * 19    12/09/97 6:14p Lawrance
 * add -nomusic flag
 * 
 * 18    12/01/97 5:10p Dave
 * Fixed a syntax bug.
 * 
 * 17    12/01/97 4:59p Dave
 * Synchronized multiplayer debris objects. Put in pilot popup in main
 * hall. Optimized simulated multiplayer lag module. Fixed a potential
 * file_xfer bug.
 * 
 * 16    11/28/97 7:04p Dave
 * Emergency checkin due to big system crash.
 * 
 * 15    11/28/97 5:06p Dave
 * Put in facilities for simulating multiplayer lag.
 * 
 * 14    11/24/97 5:42p Dave
 * Fixed a file xfer buffer free/malloc problem. Lengthened command line
 * switch string parse length.
 * 
 * 13    11/12/97 4:39p Dave
 * Put in multiplayer campaign support parsing, loading and saving. Made
 * command-line variables better named. Changed some things on the initial
 * pilot select screen.
 * 
 * 12    11/11/97 4:54p Dave
 * Put in support for single vs. multiplayer pilots. Put in initial player
 * selection screen (no command line option yet). Started work on
 * multiplayer campaign file save gaming.
 * 
 * 11    11/11/97 11:55a Allender
 * initialize network at beginning of application.  create new call to set
 * which network protocol to use
 * 
 * 10    9/18/97 10:12p Dave
 * Added -gimmemedals, which gives the current pilot all the medals in the
 * game (debug)
 * 
 * 9     9/18/97 9:20a Dave
 * Minor modifications
 * 
 * 8     9/15/97 11:40p Lawrance
 * remove demo granularity switch
 * 
 * 7     9/09/97 3:39p Sandeep
 * warning level 4 bugs
 * 
 * 6     9/03/97 5:03p Lawrance
 * add support for -nosound command line parm
 * 
 * 5     8/22/97 8:52a Dave
 * Removed a return statement which would have broken the parser out too
 * early.
 * 
 * 4     8/21/97 4:55p Dave
 * Added a switch for multiplayer chat streaming. Added a section for
 * global command line vars.
 * 
 * 3     8/06/97 2:26p Dave
 * Made the command line parse more robust. Made it easier to add and
 * process new command-line switches.
 * 
 * 2     8/04/97 3:13p Dave
 * Added command line functions. See cmdline.cpp for directions on adding
 * new switches
 * 
 * 1     8/04/97 9:58a Dave
 * 
 * $NoKeywords: $
 */

#include <string.h>
#include <stdlib.h>
#include "cmdline.h"
#include "linklist.h"
#include "systemvars.h"
#include "multi.h"
#include "cfile.h"
#include "osapi.h"
#include "osregistry.h"
#include "version.h"

// variables
class cmdline_parm {
public:
	cmdline_parm *next, *prev;
	char *name;						// name of parameter, must start with '-' char
	char *help;						// help text for this parameter
	char *args;						// string value for parameter arguements (NULL if no arguements)
	int name_found;				// true if parameter on command line, otherwise false

	cmdline_parm(char *name, char *help);
	~cmdline_parm();
	int found();
	int get_int();
	float get_float();
	char *str();
};

// here are the command line parameters that we will be using for FreeSpace
#ifndef PLAT_UNIX
cmdline_parm standalone_arg("-standalone", NULL);
cmdline_parm nosound_arg("-nosound", NULL);
cmdline_parm nomusic_arg("-nomusic", NULL);
cmdline_parm startgame_arg("-startgame", NULL);
cmdline_parm gamename_arg("-gamename", NULL);
cmdline_parm gamepassword_arg("-password", NULL);
cmdline_parm gameclosed_arg("-closed", NULL);
cmdline_parm gamerestricted_arg("-restricted", NULL);
cmdline_parm allowabove_arg("-allowabove", NULL);
cmdline_parm allowbelow_arg("-allowbelow", NULL);
cmdline_parm port_arg("-port", NULL);
cmdline_parm connect_arg("-connect", NULL);
cmdline_parm multilog_arg("-multilog", NULL);
cmdline_parm server_firing_arg("-oldfire", NULL);
cmdline_parm client_dodamage("-clientdamage", NULL);
cmdline_parm pof_spew("-pofspew", NULL);
cmdline_parm d3d_32bit("-32bit", NULL);
cmdline_parm mouse_coords("-coords", NULL);
cmdline_parm timeout("-timeout", NULL);
cmdline_parm d3d_window("-window", NULL);
#else
// double hyphens on Unix options
cmdline_parm standalone_arg("--standalone", NULL);
cmdline_parm nosound_arg("--nosound", NULL);
cmdline_parm nomusic_arg("--nomusic", NULL);
cmdline_parm startgame_arg("--startgame", NULL);
cmdline_parm gamename_arg("--gamename", NULL);
cmdline_parm gamepassword_arg("--password", NULL);
cmdline_parm gameclosed_arg("--closed", NULL);
cmdline_parm gamerestricted_arg("--restricted", NULL);
cmdline_parm allowabove_arg("--allowabove", NULL);
cmdline_parm allowbelow_arg("--allowbelow", NULL);
cmdline_parm port_arg("--port", NULL);
cmdline_parm connect_arg("--connect", NULL);
cmdline_parm multilog_arg("--multilog", NULL);
cmdline_parm server_firing_arg("--oldfire", NULL);
cmdline_parm client_dodamage("--clientdamage", NULL);
cmdline_parm pof_spew("--pofspew", NULL);
cmdline_parm mouse_coords("--coords", NULL);
cmdline_parm timeout("--timeout", NULL);
cmdline_parm d3d_window("--window", NULL);
cmdline_parm d3d_fullscreen("--fullscreen", NULL);
cmdline_parm help("--help", NULL);
cmdline_parm no_grab("--nograb", NULL);
cmdline_parm fs_version("--version", NULL);
cmdline_parm no_movies("--nomovies", NULL);

// single letter version of above options
cmdline_parm standalone_arg_s("-d", NULL);
cmdline_parm nosound_arg_s("-s", NULL);
cmdline_parm startgame_arg_s("-S", NULL);
cmdline_parm gamename_arg_s("-N", NULL);
cmdline_parm gamepassword_arg_s("-p", NULL);
cmdline_parm gameclosed_arg_s("-c", NULL);
cmdline_parm gamerestricted_arg_s("-r", NULL);
cmdline_parm allowabove_arg_s("-a", NULL);
cmdline_parm allowbelow_arg_s("-b", NULL);
cmdline_parm port_arg_s("-o", NULL);
cmdline_parm connect_arg_s("-C", NULL);
cmdline_parm multilog_arg_s("-m", NULL);
cmdline_parm server_firing_arg_s("-F", NULL);
cmdline_parm client_dodamage_s("-D", NULL);
cmdline_parm pof_spew_s("-P", NULL);
cmdline_parm mouse_coords_s("-M", NULL);
cmdline_parm timeout_s("-t", NULL);
cmdline_parm d3d_window_s("-w", NULL);
cmdline_parm d3d_fullscreen_s("-f", NULL);
cmdline_parm help_s("-h", NULL);
cmdline_parm no_grab_s("-g", NULL);
cmdline_parm fs_version_s("-v", NULL);
cmdline_parm no_movies_s("-n", NULL);
#endif

int Cmdline_multi_stream_chat_to_file = 0;
int Cmdline_freespace_no_sound = 0;
int Cmdline_freespace_no_music = 0;
int Cmdline_gimme_all_medals = 0;
int Cmdline_use_last_pilot = 0;
int Cmdline_multi_protocol = -1;
int Cmdline_cd_check = 1;
int Cmdline_start_netgame = 0;
int Cmdline_closed_game = 0;
int Cmdline_restricted_game = 0;
int Cmdline_network_port = -1;
char *Cmdline_game_name = NULL;
char *Cmdline_game_password = NULL;
char *Cmdline_rank_above= NULL;
char *Cmdline_rank_below = NULL;
char *Cmdline_connect_addr = NULL;
int Cmdline_multi_log = 0;
int Cmdline_server_firing = 0;
int Cmdline_client_dodamage = 0;
int Cmdline_spew_pof_info = 0;
int Cmdline_force_32bit = 0;
int Cmdline_mouse_coords = 0;
int Cmdline_timeout = -1;
#ifdef PLAT_UNIX
int Cmdline_no_grab = 0;
int Cmdline_play_movies = 1;
int Cmdline_fullscreen = 0;
#endif

int Cmdline_window = 0;

static cmdline_parm Parm_list(NULL, NULL);
static int Parm_list_inited = 0;

#ifdef PLAT_UNIX
void print_instructions();
#endif

//	Return true if this character is an extra char (white space and quotes)
int is_extra_space(char ch)
{
	return ((ch == ' ') || (ch == '\t') || (ch == 0x0a) || (ch == '\'') || (ch == '\"'));
}


// eliminates all leading and trailing extra chars from a string.  Returns pointer passed in.
char *drop_extra_chars(char *str)
{
	int s, e;

	s = 0;
	while (str[s] && is_extra_space(str[s]))
		s++;

	e = strlen(str) - 1;
	while (e > s) {
		if (!is_extra_space(str[e])){
			break;
		}

		e--;
	}

	if (e > s){
		memmove(str, str + s, e - s + 1);
	}

	str[e - s + 1] = 0;
	return str;
}


// internal function - copy the value for a parameter agruement into the cmdline_parm arg field
void parm_stuff_args(cmdline_parm *parm, char *cmdline)
{
	char buffer[1024];
	memset(buffer, 0, 1024);
	char *dest = buffer;

	cmdline += strlen(parm->name);

	while ((*cmdline != 0) && (*cmdline != '-')) {
		*dest++ = *cmdline++;
	}

	drop_extra_chars(buffer);

	// mwa 9/14/98 -- made it so that newer command line arguments found will overwrite
	// the old arguments
//	Assert(parm->args == NULL);
	if ( parm->args != NULL ) {
		delete( parm->args );
		parm->args = NULL;
	}

	int size = strlen(buffer) + 1;
	if (size > 0) {
		parm->args = new char[size];
		memset(parm->args, 0, size);
		strcpy(parm->args, buffer);
	}
}


// internal function - parse the command line, extracting parameter arguements if they exist
// cmdline - command line string passed to the application
void os_parse_parms(char *cmdline)
{
	// locate command line parameters
	cmdline_parm *parmp;
	char *cmdline_offset;

	for (parmp = GET_FIRST(&Parm_list); parmp !=END_OF_LIST(&Parm_list); parmp = GET_NEXT(parmp) ) {
		cmdline_offset = strstr(cmdline, parmp->name);
#ifdef PLAT_UNIX
		// verify that one hyphen and two hypen options don't collide
		if (cmdline_offset && (*(cmdline_offset-1) != '-')) {
#else
		if (cmdline_offset) {
#endif
			parmp->name_found = 1;
			parm_stuff_args(parmp, cmdline_offset);
		}
	}
}


// validate the command line parameters.  Display an error if an unrecognized parameter is located.
void os_validate_parms(char *cmdline)
{
	cmdline_parm *parmp;
	char seps[] = " ,\t\n";
	char *token;
	int parm_found;

   token = strtok(cmdline, seps);
   while(token != NULL) {
#ifdef PLAT_UNIX
		// make sure double hypens are checked first to avoid clashing with single args
		if (token[0] == '-' && token[1] == '-') {
			parm_found = 0;
			for (parmp = GET_FIRST(&Parm_list); parmp !=END_OF_LIST(&Parm_list); parmp = GET_NEXT(parmp) ) {
				if (!stricmp(parmp->name, token)) {
					parm_found = 1;
					break;
				}
			}

			if (parm_found == 0) {
				print_instructions();
			}
		} else if (token[0] == '-' && token[1] != '-') {
#else
		if (token[0] == '-') {
#endif
			parm_found = 0;
			for (parmp = GET_FIRST(&Parm_list); parmp !=END_OF_LIST(&Parm_list); parmp = GET_NEXT(parmp) ) {
				if (!stricmp(parmp->name, token)) {
					parm_found = 1;
					break;
				}
			}

			if (parm_found == 0) {
#ifndef PLAT_UNIX
				Error(LOCATION,"Unrecogzined command line parameter %s", token);
#else
				print_instructions();
#endif
			}
		}

		token = strtok(NULL, seps);
	}
}


// Call once to initialize the command line system
//
// cmdline - command line string passed to the application
void os_init_cmdline(char *cmdline)
{
	FILE *fp;

	// read the cmdline.cfg file from the data folder, and pass the command line arguments to
	// the the parse_parms and validate_parms line.  Read these first so anything actually on
	// the command line will take precedence
#ifdef PLAT_UNIX
	char cmdname[MAX_FILENAME_LENGTH];

	snprintf(cmdname, MAX_FILENAME_LENGTH, "%s/%s/Data/cmdline.cfg", detect_home(), Osreg_user_dir);
	fp = fopen(cmdname, "rt");
	
	if (!fp) {
		// if not already found check exec directory
		fp = fopen("Data/cmdline.cfg", "rt");
	}
#else
	fp = fopen("data\\cmdline.cfg", "rt");
#endif

	// if the file exists, get a single line, and deal with it
	if ( fp ) {
		char buf[1024], *p;

		fgets(buf, 1024, fp);

		// replace the newline character with a NUL:
		if ( (p = strrchr(buf, '\n')) != NULL ) {
			*p = '\0';
		}

		os_parse_parms(buf);
		os_validate_parms(buf);
		fclose(fp);
	}



	os_parse_parms(cmdline);
	os_validate_parms(cmdline);

}

#ifdef PLAT_UNIX
// help for available cmdline options
void print_instructions()
{
	printf("http://icculus.org/freespace2\n");
	printf("Support - FAQ: http://icculus.org/lgfaq\n");
	printf("          Web: http://bugzilla.icculus.org\n\n");

	printf("Usage: freespace2 [options]\n");
	printf("     [-h | --help]           Show this help message\n");
	printf("     [-v | --version]        Show game version\n");
	printf("     [-s | --nosound]        Do no access the sound card\n");
	printf("     [-f | --fullscreen]     Run the game fullscreen\n");
	printf("     [-w | --window]         Run the game in a window\n");
	printf("     [-g | --nograb]         Do not automatically grab mouse\n");
	printf("     [-n | --nomovies]       Do not play movies\n");
	printf("     [-d | --standalone]     Run as a dedicated server\n");
	printf("     [-S | --startgame]      Start a multiplayer game\n");
	printf("     [-N | --gamename]       Name of the multiplayer game\n");
	printf("     [-p | --password]       Use this password to connect\n");
	printf("     [-c | --closed]         Closed multiplayer game\n");
	printf("     [-r | --restricted]     Restricted multiplayer game\n");
	printf("     [-a | --allowabove]     Only allow above certain rank\n");
	printf("     [-b | --allowbelow]     Only allow below certain rank\n");
	printf("     [-o | --port]           Port to use for multiplayer games\n");
	printf("     [-C | --connect]        Connect to particular IP address\n");
	printf("     [-m | --multilog]       Log multiplayer events\n");
	printf("     [-F | --oldfire]        Server side firing\n");
	printf("     [-D | --clientdamage]   Client does damage\n");
	printf("     [-t | --timeout]        Multiplayer game timeout\n");
	printf("     [-P | --pofspew]        Save model info to pofspew.txt\n");
	printf("     [-M | --coords]         Show coordinates of the mouse cursor\n\n");

	printf("Freespace 2 v%d.%02d -- Linux Client v%d.%02d\n\n", FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_UNIX_VERSION_MAJOR, FS_UNIX_VERSION_MINOR);

	exit(0);
}
#endif

// arg constructor
// name_ - name of the parameter, must start with '-' character
// help_ - help text for this parameter
cmdline_parm::cmdline_parm(char *name_, char *help_)
{
	name = name_;
	help = help_;
	args = NULL;
	name_found = 0;

	if (Parm_list_inited == 0) {
		list_init(&Parm_list);
		Parm_list_inited = 1;
	}

	if (name != NULL) {
		list_append(&Parm_list, this);
	}
}


// destructor - frees any allocated memory
cmdline_parm::~cmdline_parm()
{
	if (args) {
		delete [] args;
		args = NULL;
	}
}


// returns - true if the parameter exists on the command line, otherwise false
int cmdline_parm::found()
{
	return name_found;
}


// returns - the interger representation for the parameter arguement
int cmdline_parm::get_int()
{
	Assert(args);
	return atoi(args);
}


// returns - the float representation for the parameter arguement
float cmdline_parm::get_float()
{
	Assert(args);
	return (float)atof(args);
}


// returns - the string value for the parameter arguement
char *cmdline_parm::str()
{
	Assert(args);
	return args;
}

// external entry point into this modules
int parse_cmdline(char *cmdline)
{
	os_init_cmdline(cmdline);

	// is this a standalone server??
#ifdef PLAT_UNIX
	if (standalone_arg.found() || standalone_arg_s.found()) {
#else
	if (standalone_arg.found()) {
#endif
		Is_standalone = 1;
	}

	// run with no sound
#ifdef PLAT_UNIX
	if ( nosound_arg.found() || nosound_arg_s.found() ) {
#else
	if ( nosound_arg.found() ) {
#endif
		Cmdline_freespace_no_sound = 1;
	}

	// run with no music
	if ( nomusic_arg.found() ) {
		Cmdline_freespace_no_music = 1;
	}

	// should we start a network game
#ifdef PLAT_UNIX
	if ( startgame_arg.found() || startgame_arg_s.found() ) {
#else
	if ( startgame_arg.found() ) {
#endif
		Cmdline_use_last_pilot = 1;
		Cmdline_start_netgame = 1;
	}

	// closed network game
#ifdef PLAT_UNIX
	if ( gameclosed_arg.found() || gameclosed_arg_s.found() ) {
#else
	if ( gameclosed_arg.found() ) {
#endif
		Cmdline_closed_game = 1;
	}

	// restircted network game
#ifdef PLAT_UNIX
	if ( gamerestricted_arg.found() || gamerestricted_arg_s.found() ) {
#else
	if ( gamerestricted_arg.found() ) {
#endif
		Cmdline_restricted_game = 1;
	}

	// get the name of the network game
#ifdef PLAT_UNIX
	if ( gamename_arg.found() || gamename_arg_s.found() ) {
		// be sure both options get checked if needed
		if ( gamename_arg.found() ) {
			Cmdline_game_name = gamename_arg.str();
		} else {
			Cmdline_game_name = gamename_arg_s.str();
		}
#else
	if ( gamename_arg.found() ) {
		Cmdline_game_name = gamename_arg.str();
#endif

#ifdef PLAT_UNIX
		// if there wasn't an argument then complain and exit
		if ( !(strlen(Cmdline_game_name) > 0) ) {
			fprintf(stderr, "ERROR: The --gamename (-N) option requires an additional argument!\n");
			exit(0);
		}
#endif

		// be sure that this string fits in our limits
		if ( strlen(Cmdline_game_name) > MAX_GAMENAME_LEN ) {
			Cmdline_game_name[MAX_GAMENAME_LEN-1] = '\0';
		}
	}

	// get the password for a pssword game
#ifdef PLAT_UNIX
	if ( gamepassword_arg.found() || gamepassword_arg_s.found() ) {
		// be sure both options get checked if needed
		if ( gamepassword_arg.found() ) {
			Cmdline_game_password = gamepassword_arg.str();
		} else {
			Cmdline_game_password = gamepassword_arg_s.str();
		}
#else
	if ( gamepassword_arg.found() ) {
		Cmdline_game_password = gamepassword_arg.str();
#endif

#ifdef PLAT_UNIX
		// if there wasn't an argument then complain and exit
		if ( !(strlen(Cmdline_game_password) > 0) ) {
			fprintf(stderr, "ERROR: The --password (-p) option requires an additional argument!\n");
			exit(0);
		}
#endif

		// be sure that this string fits in our limits
		if ( strlen(Cmdline_game_password) > MAX_PASSWD_LEN ) {
			Cmdline_game_password[MAX_PASSWD_LEN-1] = '\0';
		}
	}

	// set the rank above/below arguments
#ifdef PLAT_UNIX
	if ( allowabove_arg.found() || allowabove_arg_s.found() ) {
		Cmdline_rank_above = allowabove_arg.str();
	}
	if ( allowbelow_arg.found() || allowbelow_arg_s.found() ) {
		Cmdline_rank_below = allowbelow_arg.str();
	}
#else
	if ( allowabove_arg.found() ) {
		Cmdline_rank_above = allowabove_arg.str();
	}
	if ( allowbelow_arg.found() ) {
		Cmdline_rank_below = allowbelow_arg.str();
	}
#endif

	// get the port number for games
#ifdef PLAT_UNIX
	if ( port_arg.found() || port_arg_s.found() ) {
		// be sure both options get checked if needed
		if ( port_arg.found() ) {
			Cmdline_network_port = port_arg.get_int();
		} else {
			Cmdline_network_port = port_arg_s.get_int();
		}
#else
	if ( port_arg.found() ) {
		Cmdline_network_port = port_arg.get_int();
#endif

#ifdef PLAT_UNIX
		// if there wasn't an argument then complain and exit
		if ( !Cmdline_network_port ) {
			fprintf(stderr, "ERROR: The --port (-P) option requires an additional argument!\n");
			exit(0);
		}
#endif
	}

	// the connect argument specifies to join a game at this particular address
#ifdef PLAT_UNIX
	if ( connect_arg.found() || connect_arg_s.found() ) {
		Cmdline_use_last_pilot = 1;
		// be sure both options get checked it needed
		if ( connect_arg.found() ) {
			Cmdline_connect_addr = connect_arg.str();
		} else {
			Cmdline_connect_addr = connect_arg_s.str();
		}
#else
	if ( connect_arg.found() ) {
		Cmdline_use_last_pilot = 1;
		Cmdline_connect_addr = connect_arg.str();
#endif

#ifdef PLAT_UNIX
		// if there wasn't an argument then complain and exit
		if ( !(strlen(Cmdline_connect_addr) > 0) ) {
			fprintf(stderr, "ERROR: The --connect (-C) option requires an additional argument!\n");
			exit(0);
		}
#endif
	}

	// see if the multilog flag was set
#ifdef PLAT_UNIX
	if ( multilog_arg.found() || multilog_arg_s.found() ){
#else
	if ( multilog_arg.found() ){
#endif
		Cmdline_multi_log = 1;
	}	

	// maybe use old-school server-side firing
#ifdef PLAT_UNIX
	if (server_firing_arg.found() || server_firing_arg_s.found() ){
#else
	if (server_firing_arg.found() ){
#endif
		Cmdline_server_firing = 1;
	}

	// maybe use old-school client damage
#ifdef PLAT_UNIX
	if(client_dodamage.found() || client_dodamage_s.found()){
#else
	if(client_dodamage.found()){
#endif
		Cmdline_client_dodamage = 1;
	}	

	// spew pof info
#ifdef PLAT_UNIX
	if(pof_spew.found() || pof_spew_s.found()){
#else
	if(pof_spew.found()){
#endif
		Cmdline_spew_pof_info = 1;
	}

#ifndef PLAT_UNIX
	// 32 bit
	if(d3d_32bit.found()){
		Cmdline_force_32bit = 1;
	}
#endif

	// mouse coords
#ifdef PLAT_UNIX
	if(mouse_coords.found() || mouse_coords_s.found()){
#else
	if(mouse_coords.found()){
#endif
		Cmdline_mouse_coords = 1;
	}

	// net timeout
#ifdef PLAT_UNIX
	if(timeout.found() || timeout_s.found()){
#else
	if(timeout.found()){
#endif
		Cmdline_timeout = timeout.get_int();
	}

	// d3d windowed
#ifdef PLAT_UNIX
	if(d3d_window.found() || d3d_window_s.found()){
#else
	if(d3d_window.found()){
#endif
		Cmdline_window = 1;
	}

#ifdef PLAT_UNIX
	// run fullscreen
	if(d3d_fullscreen.found() || d3d_fullscreen_s.found()){
		Cmdline_fullscreen = 1;
	}

	// help!!
	if(help.found() || help_s.found()){
		print_instructions();
	}

	// no key/mouse grab
	if(no_grab.found() || no_grab_s.found()){
		Cmdline_no_grab = 1;
	}

	// play movies?
	if(no_movies.found() || no_movies_s.found()){
		Cmdline_play_movies = 0;
	}

	// display game version
	if(fs_version.found() || fs_version_s.found()){
		printf("Freespace 2 version:  %d.%02d\n", FS_VERSION_MAJOR, FS_VERSION_MINOR);
		printf("Linux client version:  %d.%02d\n", FS_UNIX_VERSION_MAJOR, FS_UNIX_VERSION_MINOR);
		exit(0);
	}
#endif

	return 1;
}
