/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Network/stand_gui.cpp $
 * $Revision: 16 $
 * $Date: 8/16/99 4:06p $
 * $Author: Dave $
 *
 * $Log: /Freespace2/code/Network/stand_gui.cpp $
 *
 * 16    8/16/99 4:06p Dave
 * Big honking checkin.
 *
 * 15    8/11/99 5:54p Dave
 * Fixed collision problem. Fixed standalone ghost problem.
 *
 * 14    8/04/99 5:45p Dave
 * Upped default standalone server framerate to 60.
 *
 * 13    5/22/99 5:35p Dave
 * Debrief and chatbox screens. Fixed small hi-res HUD bug.
 *
 * 12    5/19/99 4:07p Dave
 * Moved versioning code into a nice isolated common place. Fixed up
 * updating code on the pxo screen. Fixed several stub problems.
 *
 * 11    4/25/99 7:43p Dave
 * Misc small bug fixes. Made sun draw properly.
 *
 * 10    2/24/99 2:25p Dave
 * Fixed up chatbox bugs. Made squad war reporting better. Fixed a respawn
 * bug for dogfight more.
 *
 * 9     2/18/99 11:46a Neilk
 * hires interface coord support
 *
 * 8     2/12/99 6:16p Dave
 * Pre-mission Squad War code is 95% done.
 *
 * 7     11/19/98 4:19p Dave
 * Put IPX sockets back in psnet. Consolidated all multiplayer config
 * files into one.
 *
 * 6     11/17/98 11:12a Dave
 * Removed player identification by address. Now assign explicit id #'s.
 *
 * 5     11/05/98 5:55p Dave
 * Big pass at reducing #includes
 *
 * 4     10/09/98 2:57p Dave
 * Starting splitting up OS stuff.
 *
 * 3     10/08/98 4:29p Dave
 * Removed reference to osdefs.h
 *
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 *
 * 1     10/07/98 10:50a Dave
 *
 * 63    9/17/98 11:56a Allender
 * allow use of return key in edit box
 *
 * 62    9/04/98 3:52p Dave
 * Put in validated mission updating and application during stats
 * updating.
 *
 * 61    7/24/98 9:27a Dave
 * Tidied up endgame sequencing by removing several old flags and
 * standardizing _all_ endgame stuff with a single function call.
 *
 * 60    7/10/98 5:04p Dave
 * Fix connection speed bug on standalone server.
 *
 * 59    6/18/98 4:46p Allender
 * removed test code that I previously forgot to remove
 *
 * 58    6/18/98 3:52p Allender
 * make close button actually close down property sheet
 *
 * 57    6/13/98 6:02p Hoffoss
 * Externalized all new (or forgot to be added) strings to all the code.
 *
 * 56    6/13/98 3:19p Hoffoss
 * NOX()ed out a bunch of strings that shouldn't be translated.
 *
 * 55    6/10/98 2:56p Dave
 * Substantial changes to reduce bandwidth and latency problems.
 *
 * 54    5/24/98 3:45a Dave
 * Minor object update fixes. Justify channel information on PXO. Add a
 * bunch of configuration stuff for the standalone.
 *
 * 53    5/22/98 9:35p Dave
 * Put in channel based support for PXO. Put in "shutdown" button for
 * standalone. UI tweaks for TvT
 *
 * 52    5/21/98 9:45p Dave
 * Lengthened tracker polling times. Put in initial support for PXO
 * servers with channel filters. Fixed several small UI bugs.
 *
 * 51    5/18/98 9:15p Dave
 * Put in network config file support.
 *
 * 50    5/15/98 5:16p Dave
 * Fix a standalone resetting bug.Tweaked PXO interface. Display captaincy
 * status for team vs. team. Put in asserts to check for invalid team vs.
 * team situations.
 *
 * 49    5/15/98 3:36p John
 * Fixed bug with new graphics window code and standalone server.  Made
 * hwndApp not be a global anymore.
 *
 * 48    5/10/98 7:06p Dave
 * Fix endgame sequencing ESC key. Changed how host options warning popups
 * are done. Fixed pause/message scrollback/options screen problems in mp.
 * Make sure observer HUD doesn't try to lock weapons.
 *
 * 47    5/09/98 7:16p Dave
 * Put in CD checking. Put in standalone host password. Made pilot into
 * popup scrollable.
 *
 * 46    5/08/98 7:09p Dave
 * Lots of UI tweaking.
 *
 * 45    5/08/98 5:05p Dave
 * Go to the join game screen when quitting multiplayer. Fixed mission
 * text chat bugs. Put mission type symbols on the create game list.
 * Started updating standalone gui controls.
 *
 * 44    5/04/98 1:44p Dave
 * Fixed up a standalone resetting problem. Fixed multiplayer stats
 * collection for clients. Make sure all multiplayer ui screens have the
 * correct palette at all times.
 *
 * 43    5/02/98 5:38p Dave
 * Put in new tracker API code. Put in ship information on mp team select
 * screen. Make standalone server name permanent. Fixed standalone server
 * text messages.
 *
 * 42    5/02/98 1:45a Dave
 * Make standalone goal tree not flicker.
 *
 * 41    5/01/98 10:57a Jim
 * from Dave:  fixed mission goal problems
 *
 * 40    4/29/98 12:11a Dave
 * Put in first rev of full API support for new master tracker.
 *
 * 39    4/28/98 5:11p Dave
 * Fixed multi_quit_game() client side sequencing problem. Turn off
 * afterburners when ending multiplayer mission. Begin integration of mt
 * API from Kevin Bentley.
 *
 * 38    4/25/98 2:02p Dave
 * Put in multiplayer context help screens. Reworked ingame join ship
 * select screen. Fixed places where network timestamps get hosed.
 *
 * 37    4/06/98 6:37p Dave
 * Put in max_observers netgame server option. Make sure host is always
 * defaulted to alpha 1 or zeta 1. Changed create game so that MAX_PLAYERS
 * can always join but need to be kicked before commit can happen. Put in
 * support for server ending a game and notifying clients of a special
 * condition.
 *
 * 36    3/31/98 5:18p John
 * Removed demo/save/restore.  Made NDEBUG defined compile.  Removed a
 * bunch of debug stuff out of player file.  Made model code be able to
 * unload models and malloc out only however many models are needed.
 *
 *
 * 35    3/24/98 5:00p Dave
 * Fixed several ui bugs. Put in pre and post voice stream playback sound
 * fx. Put in error specific popups for clients getting dropped from games
 * through actions other than their own.
 *
 * 34    3/19/98 5:05p Dave
 * Put in support for targeted multiplayer text and voice messaging (all,
 * friendly, hostile, individual).
 *
 * 33    3/17/98 5:29p Dave
 * Minor bug fixes in player select menu. Solidified mp joining process.
 * Made furball mode support ingame joiners and dropped players correctly.
 *
 * 32    3/15/98 4:17p Dave
 * Fixed oberver hud problems. Put in handy netplayer macros. Reduced size
 * of network orientation matrices.
 *
 * 31    3/03/98 5:12p Dave
 * 50% done with team vs. team interface issues. Added statskeeping to
 * secondary weapon blasts. Numerous multiplayer ui bug fixes.
 *
 * 30    2/12/98 4:41p Dave
 * Seperated multiplayer kick functionality into its own module. Ui
 * tweaking
 *
 * 29    2/05/98 10:24a Hoffoss
 * Changed "goal" text to "objective", which is the correct term nowadays.
 *
 * 28    1/31/98 4:32p Dave
 * Put in new support for VMT player validation, game logging in, and game
 * logging out. Need to finish stats transfer.
 *
 * 27    1/28/98 6:24p Dave
 * Made standalone use ~8 megs less memory. Fixed multiplayer submenu
 * sequencing problem.
 *
 * 26    1/24/98 3:39p Dave
 * Fixed numerous multiplayer bugs (last frame quit problem, weapon bank
 * changing, deny packets). Add several controls to standalone server.
 *
 * 25    1/20/98 2:23p Dave
 * Removed optimized build warnings. 99% done with ingame join fixes.
 *
 * 24    1/17/98 2:46a Dave
 * Reworked multiplayer join/accept process. Ingame join still needs to be
 * integrated.
 *
 * 23    1/16/98 2:34p Dave
 * Made pause screen work properly (multiplayer). Changed how chat packets
 * work.
 *
 * 22    1/13/98 5:37p Dave
 * Reworked a lot of standalone interface code. Put in single and
 * multiplayer popups for death sequence. Solidified multiplayer kick
 * code.
 *
 * 21    1/11/98 10:03p Allender
 * removed <winsock.h> from headers which included it.  Made psnet_socket
 * type which is defined just as SOCKET type is.
 *
 * 20    1/05/98 5:07p Dave
 * Fixed a chat packet bug. Fixed a few state save/restore bugs. Updated a
 * few things for multiplayer server transfer.
 *
 * 19    12/10/97 4:46p Dave
 * Added in more detailed support for multiplayer packet lag/loss. Fixed
 * some multiplayer stuff. Added some controls to the standalone.
 *
 * 18    12/03/97 11:50p Dave
 * Fixed a bunch of multiplayer bugs (standalone and non)
 *
 * 17    12/03/97 11:59a Dave
 * Dependant merge checkin
 *
 * 16    12/02/97 10:05p Dave
 * Began some large-scale multiplayer debugging work (mostly standalone)
 *
 * 15    11/15/97 2:37p Dave
 * More multiplayer campaign support.
 *
 * 14    10/29/97 5:18p Dave
 * More debugging of server transfer. Put in debrief/brief
 * transition for multiplayer (w/standalone)
 *
 * 13    10/25/97 7:23p Dave
 * Moved back to single set stats storing. Put in better respawning
 * locations system.
 *
 * 12    10/24/97 6:19p Dave
 * More standalone testing/fixing. Added reliable endgame sequencing.
 * Added reliable ingame joining. Added reliable stats transfer (endgame).
 * Added support for dropping players in debriefing. Removed a lot of old
 * unused code.
 *
 * 11    10/21/97 5:21p Dave
 * Fixed pregame mission load/file verify debacle. Added single vs.
 * multiplayer stats system.
 *
 * 10    10/14/97 5:38p Dave
 * Player respawns 99.9% done. Only need to check cases for server/host
 * getting killed.
 *
 * 9     10/03/97 4:57p Dave
 * Added functions for new text controls. Added some more reset controls.
 * Put in checks for all-players-gone.
 *
 * 8     9/17/97 9:09a Dave
 * Observer mode works, put in standalone controls. Fixed up some stuff for
 * ingame join broken by recent code checkins.
 *
 * 7     8/29/97 5:03p Dave
 * Added a ton of new gui controls/features.
 *
 * 6     8/26/97 5:03p Dave
 * Added bunch of informational controls. Standardized some functions for
 * external use. Put in godview mode (conditionaled out though).
 *
 * 5     8/23/97 11:31a Dave
 * Put in new gui calls. Added a bunch of display controls.
 *
 * 4     8/20/97 4:19p Dave
 * Moved some functions around. Added the standalone state text box.
 *
 * 3     8/18/97 11:46a Dave
 * Moved definition of STANDALONE_FRAME_CAP tp multi.h
 *
 * 2     8/11/97 4:52p Dave
 * Spliced out standalone GUI stuff from OsApi and WinMain.cpp to its own
 * module.
 *
 * 1     8/11/97 4:21p Dave
 *
 * $NoKeywords: $
 */


#include "pstypes.h"
#include "stand_gui.h"


void std_add_ban(const char *name)
{
	STUB_FUNCTION;
}

void std_add_chat_text(const char *text, int player_index, int add_id)
{
	STUB_FUNCTION;
}

void std_add_player(net_player *p)
{
	STUB_FUNCTION;
}

int std_connect_set_connect_count()
{
	STUB_FUNCTION;
	
	return 0;
}

void std_connect_set_gamename(const char *name)
{
	STUB_FUNCTION;
}

void std_connect_set_host_connect_status()
{
	STUB_FUNCTION;
}

void std_create_gen_dialog(const char *title)
{
	STUB_FUNCTION;
}

void std_debug_set_standalone_state_string(const char *str)
{
	STUB_FUNCTION;
}

void std_destroy_gen_dialog()
{
	STUB_FUNCTION;
}

void std_do_gui_frame()
{
	STUB_FUNCTION;
}

void std_gen_set_text(const char *str, int field_num)
{
	STUB_FUNCTION;
}

void std_init_standalone()
{
	STUB_FUNCTION;
}

int std_is_host_passwd()
{
	return 0;
}

void std_multi_add_goals()
{
	STUB_FUNCTION;
}

void std_multi_set_standalone_mission_name(const char *mission_name)
{
	STUB_FUNCTION;
}

void std_multi_set_standalone_missiontime(float mission_time)
{
	STUB_FUNCTION;
}

void std_multi_setup_goal_tree()
{
	STUB_FUNCTION;
}

void std_multi_update_goals()
{
	STUB_FUNCTION;
}

void std_multi_update_netgame_info_controls()
{
	STUB_FUNCTION;
}

int std_player_is_banned(const char *name)
{
	return 0;
}

int std_remove_player(net_player *p)
{
	STUB_FUNCTION;
	
	return 0;
}

void std_reset_standalone_gui()
{
	STUB_FUNCTION;
}

void std_reset_timestamps()
{
	STUB_FUNCTION;
}

void std_set_standalone_fps(float fps)
{
	STUB_FUNCTION;
}

void std_update_player_ping(net_player *p)
{
	STUB_FUNCTION;
}
