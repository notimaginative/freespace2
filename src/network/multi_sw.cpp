/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Network/multi_sw.cpp $
 * $Revision: 10 $
 * $Date: 9/13/99 11:30a $
 * $Author: Dave $
 *
 * $Log: /Freespace2/code/Network/multi_sw.cpp $  
 * 
 * 10    9/13/99 11:30a Dave
 * Added checkboxes and functionality for disabling PXO banners as well as
 * disabling d3d zbuffer biasing.
 * 
 * 9     9/04/99 8:00p Dave
 * Fixed up 1024 and 32 bit movie support.
 * 
 * 8     8/25/99 4:38p Dave
 * Updated PXO stuff. Make squad war report stuff much more nicely.
 * 
 * 7     4/09/99 2:21p Dave
 * Multiplayer beta stuff. CD checking.
 * 
 * 6     2/24/99 2:25p Dave
 * Fixed up chatbox bugs. Made squad war reporting better. Fixed a respawn
 * bug for dogfight more.
 * 
 * 5     2/19/99 2:55p Dave
 * Temporary checking to report the winner of a squad war match.
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


#include "systemvars.h"
#include "localize.h"
#include "multi.h"
#include "popup.h"
#include "ptrack.h"
#include "multi_fstracker.h"
#include "multimsgs.h"
#include "multi_team.h"
#include "multi_sw.h"
#include "multi_pmsg.h"
#include "multiutil.h"
#include "freespace.h"

// ------------------------------------------------------------------------------------
// MULTIPLAYER SQUAD WAR DEFINES/VARS
//

// global request info
squad_war_request	Multi_sw_request;

// global result info
squad_war_result Multi_sw_result;

// set on the host in response to a standalone sw query, -1 == waiting, 0 == fail, 1 == success
int Multi_sw_std_query = -1;

// match code
char Multi_sw_match_code[MATCH_CODE_LEN];

// reply from a standalone on a bad response
char Multi_sw_bad_reply[MAX_SQUAD_RESPONSE_LEN+1] = "";

// team scores
extern int Multi_team0_score;
extern int Multi_team1_score;

// ------------------------------------------------------------------------------------
// MULTIPLAYER SQUAD WAR FORWARD DECLARATIONS
//

// popup_till_condition do frame function for the host doing verification through the standalone
int multi_sw_query_tracker_A();

// condition do frame function for the standalone querying PXO
int multi_sw_query_tracker_B();

// stuff Multi_sw_request
void multi_sw_stuff_request(char *match_code);

// stuff Multi_sw_result
void multi_sw_stuff_result();

// verify that we have the proper # of players
int multi_sw_verify_squad_counts();


// ------------------------------------------------------------------------------------
// MULTIPLAYER SQUAD WAR FUNCTIONS
//

// call before loading level - mission sync phase. only the server need do this
void multi_sw_level_init()
{	
}

// determine if everything is ok to move forward for a squad war match
int multi_sw_ok_to_commit()
{
	char *ret;
	char match_code[MATCH_CODE_LEN] = "";	
	char bad_response[MAX_SQUAD_RESPONSE_LEN+1];

	memset(bad_response, 0, MAX_SQUAD_RESPONSE_LEN+1);

	// make sure we have enough players per team
	if(!multi_sw_verify_squad_counts()){
		return 0;
	}		

	// prompt the host for the match code
	ret = popup_input(0, XSTR("Please enter the match code", 1076), 32);
	if(ret == NULL){
		return 0;
	}
	strcpy(match_code, ret);

	strcpy(Multi_sw_match_code, "");

	// if we're the server, do the nice case
	if(MULTIPLAYER_MASTER){
		// stuff match request
		multi_sw_stuff_request(match_code);
		
		// return an MSW_STATUS_* value
		if(multi_fs_tracker_validate_sw(&Multi_sw_request, bad_response) == MSW_STATUS_VALID){			
			// store the match code			
			strncpy(Multi_sw_match_code, match_code, MATCH_CODE_LEN);

			// success		
			return 1;
		} 
		// do - didn't check out properly
		else {
			if(strlen(bad_response) > 0){
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, "Error validating Squad War match\n\n(%s)", bad_response);
			} else {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, "Error validating Squad War match\n\n(%s)", "Unknown");
			}
		}
	}
	// otherwise we have to do it through the standalone - ARRRGHH!
	else {
		// send the query packet and wait for a response
		Multi_sw_std_query = -1;
		memset(Multi_sw_bad_reply, 0, MAX_SQUAD_RESPONSE_LEN+1);
		send_sw_query_packet(SW_STD_START, match_code);
		if(popup_till_condition(multi_sw_query_tracker_A, XSTR("&Cancel", 667), XSTR("Validating squad war", 1075)) == 10){
			// success
			return 1;
		} else {
			if(strlen(Multi_sw_bad_reply) > 0){
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, "Error validating Squad War match\n\n(%s)", Multi_sw_bad_reply);
			} else {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, "Error validating Squad War match\n\n(%s)", "Unknown");
			}
		}
	}

	// hmm, should probably never get here
	return 0;
}

// query PXO on the standalone
void multi_sw_std_query(char *match_code)
{
	char bad_response[MAX_SQUAD_RESPONSE_LEN+1];

	memset(bad_response, 0, MAX_SQUAD_RESPONSE_LEN+1);

	// stuff match request
	multi_sw_stuff_request(match_code);	

	strcpy(Multi_sw_match_code, "");

	// return an MSW_STATUS_* value
	if(multi_fs_tracker_validate_sw(&Multi_sw_request, bad_response) != MSW_STATUS_VALID){
		send_sw_query_packet(SW_STD_BAD, bad_response);
	} else {
		// store the match code
		strncpy(Multi_sw_match_code, match_code, MATCH_CODE_LEN);
			
		send_sw_query_packet(SW_STD_OK, NULL);		
	}
}

// call to update everything on the tracker
#define SEND_AND_DISPLAY(mesg)		do { send_game_chat_packet(Net_player, mesg, MULTI_MSG_ALL, NULL, NULL, 1); multi_display_chat_msg(mesg, 0, 0); } while(0);
void multi_sw_report(int stats_saved)
{			
	char bad_response[MAX_SQUAD_RESPONSE_LEN+1];
	memset(bad_response, 0, MAX_SQUAD_RESPONSE_LEN+1);	

	// stuff Multi_sw_result
	multi_sw_stuff_result();	

	// update on PXO	
	if(stats_saved){
		if(multi_fs_tracker_store_sw(&Multi_sw_result, bad_response)){
			SEND_AND_DISPLAY(XSTR("<SquadWar results stored on PXO>", 1079));
		} else {
			SEND_AND_DISPLAY(XSTR("<SquadWar results rejected by PXO>", 1080));
			if(strlen(bad_response) > 0){
				SEND_AND_DISPLAY(bad_response);
			}
		}	
	}
	// doh. something was bogus
	else {
		SEND_AND_DISPLAY(XSTR("<SquadWar results invalidated>", 1478));
	}
}

// ------------------------------------------------------------------------------------
// MULTIPLAYER SQUAD WAR FORWARD DEFINITIONS
//

// condition do frame function for the standalone querying PXO
int multi_sw_query_tracker_A()
{
	switch(Multi_sw_std_query){
	// still waiting
	case -1 :
		return 0;

	// failure
	case 0 :
		return 1;

	// successs
	case 1 :
		return 10;
	}

	// shouldn't get here	
	return 1;
}

// stuff Multi_sw_request
void multi_sw_stuff_request(char *match_code)
{
	int idx;	
	squad_war_request *s = &Multi_sw_request;

	// stuff squad id #'s, counts, and squad names
	s->squad_count1 = 0;
	s->squad_count2 = 0;
	for(idx=0; idx<MAX_PLAYERS; idx++){
		if(MULTI_CONNECTED(Net_players[idx]) && !MULTI_STANDALONE(Net_players[idx]) && !MULTI_PERM_OBSERVER(Net_players[idx])){
			// team 0
			if(Net_players[idx].p_info.team == 0){
				s->squad_plr1[s->squad_count1++] = Net_players[idx].tracker_player_id;				
			} 
			// team 1
			else {
				s->squad_plr2[s->squad_count2++] = Net_players[idx].tracker_player_id;				
			}
		}
	}		

	// stuff match code
	strncpy(s->match_code, match_code, MATCH_CODE_LEN);
}

// verify that we have the proper # of players
int multi_sw_verify_squad_counts()
{
	int idx;
	int team0_count = 0;
	int team1_count = 0;
	char err_string[255] = "";

	for(idx=0; idx<MAX_PLAYERS; idx++){		
		if(MULTI_CONNECTED(Net_players[idx]) && !MULTI_STANDALONE(Net_players[idx]) && !MULTI_PERM_OBSERVER(Net_players[idx])){
			if(Net_players[idx].p_info.team == 0){
				team0_count++;
			} else {
				team1_count++;
			}
		}
	}
	if((team0_count < MULTI_SW_MIN_PLAYERS) || (team1_count < MULTI_SW_MIN_PLAYERS)){	
		// print up the error string
		sprintf(err_string, XSTR("You need to have at least %d players per squad for Squad War", 1073), MULTI_SW_MIN_PLAYERS);
		popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, err_string);
		return 0;
	}

	// we're cool
	return 1;
}

// stuff Multi_sw_result
void multi_sw_stuff_result()
{
	squad_war_result *s = &Multi_sw_result;
	int idx;

	// stuff match code
	strncpy(s->match_code, Multi_sw_match_code, MATCH_CODE_LEN);

	// determine what happened
	switch(multi_team_winner()){
	// tie
	case -1:
		s->result = 0;
		s->squad_count1 = 0;
		s->squad_count2 = 0;
		break;

	// team 0 won
	case 0:
		s->result = 1;
		// stuff squad id #'s, counts, and squad names
		s->squad_count1 = 0;
		s->squad_count2 = 0;
		for(idx=0; idx<MAX_PLAYERS; idx++){
			if(MULTI_CONNECTED(Net_players[idx]) && !MULTI_STANDALONE(Net_players[idx]) && !MULTI_PERM_OBSERVER(Net_players[idx])){
				// team 0
				if(Net_players[idx].p_info.team == 0){
					s->squad_winners[s->squad_count1++] = Net_players[idx].tracker_player_id;				
				} 
				// team 1
				else {
					s->squad_losers[s->squad_count2++] = Net_players[idx].tracker_player_id;				
				}
			}
		}		
		break;

	// team 1 won
	case 1:
		s->result = 1;
		// stuff squad id #'s, counts, and squad names
		s->squad_count1 = 0;
		s->squad_count2 = 0;
		for(idx=0; idx<MAX_PLAYERS; idx++){
			if(MULTI_CONNECTED(Net_players[idx]) && !MULTI_STANDALONE(Net_players[idx]) && !MULTI_PERM_OBSERVER(Net_players[idx])){
				// team 1
				if(Net_players[idx].p_info.team == 1){
					s->squad_winners[s->squad_count1++] = Net_players[idx].tracker_player_id;				
				} 
				// team 0
				else {
					s->squad_losers[s->squad_count2++] = Net_players[idx].tracker_player_id;				
				}
			}
		}		
		break;
	}
}

