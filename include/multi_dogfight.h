/*
 * $Logfile: /Freespace2/code/Network/multi_dogfight.h $
 * $Revision$
 * $Date$
 * $Author$
 * 
 * $Log$
 * Revision 1.1  2002/05/03 03:28:12  root
 * Initial revision
 *
 * 
 * 2     2/23/99 2:29p Dave
 * First run of oldschool dogfight mode. 
 *   
 * $NoKeywords: $
 */

#ifndef __FS2_MULTIPLAYER_DOGFIGHT_HEADER_FILE
#define __FS2_MULTIPLAYER_DOGFIGHT_HEADER_FILE

// ----------------------------------------------------------------------------------------------------
// MULTI DOGFIGHT DEFINES/VARS
//

struct net_player;
struct object;


// ----------------------------------------------------------------------------------------------------
// MULTI DOGFIGHT FUNCTIONS
//

// call once per level just before entering the mission
void multi_df_level_pre_enter();

// evaluate a kill in dogfight by a netplayer
void multi_df_eval_kill(net_player *killer, object *dead_obj);

// debrief
void multi_df_debrief_init();

// do frame
void multi_df_debrief_do();

// close
void multi_df_debrief_close();

#endif

