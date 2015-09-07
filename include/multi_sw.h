/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Network/multi_sw.h $
 * $Revision: 7 $
 * $Date: 9/13/99 11:30a $
 * $Author: Dave $
 *
 * $Log: /Freespace2/code/Network/multi_sw.h $  
 * 
 * 7     9/13/99 11:30a Dave
 * Added checkboxes and functionality for disabling PXO banners as well as
 * disabling d3d zbuffer biasing.
 * 
 * 6     8/25/99 4:38p Dave
 * Updated PXO stuff. Make squad war report stuff much more nicely.
 * 
 * 5     6/07/99 9:51p Dave
 * Consolidated all multiplayer ports into one.
 * 
 * 4     2/17/99 2:11p Dave
 * First full run of squad war. All freespace and tracker side stuff
 * works.
 * 
 * 3     2/12/99 6:16p Dave
 * Pre-mission Squad War code is 95% done.
 * 
 * 2     2/11/99 3:08p Dave
 * PXO refresh button. Very preliminary squad war support.
 *  
 *  
 * $NoKeywords: $
 */

#ifndef __FREESPACE2_SQUAD_WAR_HEADER_FILE
#define __FREESPACE2_SQUAD_WAR_HEADER_FILE

#include "ptrack.h"

// ------------------------------------------------------------------------------------
// MULTIPLAYER SQUAD WAR DEFINES/VARS
//

// the min # of players required from each squad for the mission to be valid
#define MULTI_SW_MIN_PLAYERS					1

// set on the host in response to a standalone sw query, -1 == waiting, 0 == fail, 1 == success
extern int Multi_sw_std_query;

// match code
#define MATCH_CODE_LEN		34			// from ptrack.h
extern char Multi_sw_match_code[MATCH_CODE_LEN];

// reply from a standalone on a bad response
extern char Multi_sw_bad_reply[MAX_SQUAD_RESPONSE_LEN+1];

// ------------------------------------------------------------------------------------
// MULTIPLAYER SQUAD WAR FUNCTIONS
//

// call before loading level - mission sync phase. only the server need do this
void multi_sw_level_init();

// determine if everything is ok to move forward for a squad war match
int multi_sw_ok_to_commit();

// query PXO on the standalone
void multi_sw_std_query(char *match_code);

// call to update everything on the tracker
void multi_sw_report(int stats_saved);

#endif

