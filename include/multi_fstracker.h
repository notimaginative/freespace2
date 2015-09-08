/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Network/multi_fstracker.h $
 * $Revision: 8 $
 * $Date: 8/25/99 4:38p $
 * $Author: Dave $
 *
 * $Log: /Freespace2/code/Network/multi_fstracker.h $
 * 
 * 8     8/25/99 4:38p Dave
 * Updated PXO stuff. Make squad war report stuff much more nicely.
 * 
 * 7     6/07/99 9:51p Dave
 * Consolidated all multiplayer ports into one.
 * 
 * 6     2/17/99 2:11p Dave
 * First full run of squad war. All freespace and tracker side stuff
 * works.
 * 
 * 5     2/12/99 6:16p Dave
 * Pre-mission Squad War code is 95% done.
 * 
 * 4     2/11/99 3:08p Dave
 * PXO refresh button. Very preliminary squad war support.
 * 
 * 3     2/03/99 6:06p Dave
 * Groundwork for FS2 PXO usertracker support.  Gametracker support next.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:50a Dave
 * 
 * 12    9/04/98 3:51p Dave
 * Put in validated mission updating and application during stats
 * updating.
 * 
 * 11    5/21/98 9:45p Dave
 * Lengthened tracker polling times. Put in initial support for PXO
 * servers with channel filters. Fixed several small UI bugs.
 * 
 * 10    5/18/98 9:15p Dave
 * Put in network config file support.
 * 
 * 9     5/15/98 9:52p Dave
 * Added new stats for freespace. Put in artwork for viewing stats on PXO.
 * 
 * 8     5/05/98 2:10p Dave
 * Verify campaign support for testing. More new tracker code.
 * 
 * 7     4/29/98 12:11a Dave
 * Put in first rev of full API support for new master tracker.
 * 
 * 6     4/28/98 7:50p Dave
 * Fixing a broken makefile.
 * 
 * 5     4/28/98 5:10p Dave
 * Fixed multi_quit_game() client side sequencing problem. Turn off
 * afterburners when ending multiplayer mission. Begin integration of mt
 * API from Kevin Bentley.
 * 
 * 4     2/02/98 8:44p Dave
 * Finished redoing master tracker stats transfer.
 * 
 * 3     1/31/98 4:32p Dave
 * Put in new support for VMT player validation, game logging in, and game
 * logging out. Need to finish stats transfer.
 * 
 * 2     1/30/98 5:53p Dave
 * Revamped master tracker API
 * 
 * 1     1/30/98 5:50p Dave
 * 
 * $NoKeywords: $
 */
#ifndef _FREESPACE_SPECIFIC_MASTER_TRACKER_HEADER
#define _FREESPACE_SPECIFIC_MASTER_TRACKER_HEADER

// -----------------------------------------------------------------------------------
// FREESPACE MASTER TRACKER DEFINES/VARS
//

// tracker mission validation status
#define MVALID_STATUS_UNKNOWN					-1
#define MVALID_STATUS_VALID					0
#define MVALID_STATUS_INVALID					1

// tracker squad war validation status
#define MSW_STATUS_UNKNOWN						-1
#define MSW_STATUS_VALID						0
#define MSW_STATUS_INVALID						1

struct vmt_freespace2_struct;
struct scoring_struct;
struct squad_war_request;
struct squad_war_result;

// channel to associate when creating a server
extern char Multi_fs_tracker_channel[255];

// channel to use when polling the tracker for games
extern char Multi_fs_tracker_filter[255];

// -----------------------------------------------------------------------------------
// FREESPACE MASTER TRACKER DECLARATIONS
//

// give some processor time to the tracker API
void multi_fs_tracker_process();

// initialize the master tracker API for Freespace
void multi_fs_tracker_init();

// validate the current player with the master tracker (will create the pilot on the MT if necessary)
int multi_fs_tracker_validate(int show_error);

// attempt to log the current game server in with the master tracker
void multi_fs_tracker_login_freespace();

// attempt to update all player statistics and scores on the tracker
int multi_fs_tracker_store_stats();

// attempt to update all player statistics (standalone mode)
int multi_fs_std_tracker_store_stats();

// log freespace out of the tracker
void multi_fs_tracker_logout();

// send a request for a list of games
void multi_fs_tracker_send_game_request();

// if the API has successfully been initialized and is running
int multi_fs_tracker_inited();

// update our settings on the tracker regarding the current netgame stuff
void multi_fs_tracker_update_game(netgame_info *ng);

// if we're currently busy performing some tracker operation (ie, you should wait or not)
int multi_fs_tracker_busy();

// copy a freespace stats struct to a tracker-freespace stats struct
void multi_stats_fs_to_tracker(scoring_struct *fs, vmt_freespace2_struct *vmt, player *pl, int tracker_id);

// copy a tracker-freespace stats struct to a freespace stats struct
void multi_stats_tracker_to_fs(vmt_freespace2_struct *vmt, scoring_struct *fs);

// return an MVALID_STATUS_* value, or -2 if the user has "cancelled"
int multi_fs_tracker_validate_mission(char *filename);

// return an MSW_STATUS_* value
int multi_fs_tracker_validate_sw(squad_war_request *sw_req, char *bad_reply, const int max_reply_len);

// store the results of a squad war mission on PXO, return 1 on success
int multi_fs_tracker_store_sw(squad_war_result *sw_res, char *bad_reply, const int max_reply_len);

#endif
