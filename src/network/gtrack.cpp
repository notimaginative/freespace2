/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/


//Game Tracker client code
/*

  InitGameTracker(int game_type); //D3 or Freespace
  	 
  //Call periodically so we can send our update, and make sure it is received
  IdleGameTracker();
  
  StartTrackerGame(void *buffer)  //Call with a freespace_net_game_data or d3_net_game_data structure
  //Call with a freespace_net_game_data or d3_net_game_data structure
  //Updates our memory so when it is time, we send the latest game data
  UpdateGameData(gamedata);


  //Call to signify end of game

  RequestGameList();//Sends a GNT_GAMELIST_REQ packet to the server.

  game_list * GetGameList();//returns a pointer to a game_list struct

*/


#ifdef PLAT_UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#endif

#include "pstypes.h"
#include "timer.h"
#include "multi.h"
#include "multi_pxo.h"
#include "gtrack.h"


//Variables
// SOCKET gamesock;
SOCKADDR_IN	gtrackaddr;

game_list GameBuffer[MAX_GAME_BUFFERS];
int GameType;//d3 or fs

unsigned int LastTrackerUpdate;
unsigned int LastSentToTracker;
unsigned int TrackerAckdUs;
unsigned int TrackerGameIsRunning;

game_packet_header TrackerGameData;
game_packet_header GameListReq;
game_packet_header TrackAckPacket;
game_packet_header GameOverPacket;

#ifdef MAKE_FS1
freespace_net_game_data		*FreeSpaceTrackerGameData;
#else
freespace2_net_game_data	*FreeSpace2TrackerGameData;
#endif

//Start New 7-9-98
unsigned int LastGameOverPacket;
unsigned int FirstGameOverPacket;

int SendingGameOver;
//End New 7-9-98


int InitGameTrackerClient(int gametype)
{
	SOCKADDR_IN sockaddr;
	unsigned int iaddr;

	GameType = gametype;
	LastTrackerUpdate = 0;
	switch(gametype)
	{
	case GT_FREESPACE:
#ifndef MAKE_FS1
		Int3();
		return 0;
#else
		TrackerGameData.len = GAME_HEADER_ONLY_SIZE+sizeof(freespace_net_game_data);
#endif
		break;

	case GT_FREESPACE2:
#ifdef MAKE_FS1
		Int3();
		return 0;
#else
		TrackerGameData.len = GAME_HEADER_ONLY_SIZE+sizeof(freespace2_net_game_data);
#endif
		break;

	default:
		Int3();
		return 0;
	}
	TrackerGameData.game_type = (unsigned char)gametype;	//1==freespace (GT_FREESPACE), 2==D3, 3==tuberacer, etc.
	TrackerGameData.type = GNT_GAMEUPDATE;	//Used to specify what to do ie. Add a new net game (GNT_GAMESTARTED), remove a net game (game over), etc.

#ifdef MAKE_FS1
	FreeSpaceTrackerGameData = (freespace_net_game_data *)&TrackerGameData.data;
#else
	FreeSpace2TrackerGameData = (freespace2_net_game_data *)&TrackerGameData.data;
#endif
	
	GameListReq.game_type = (unsigned char)gametype;
	GameListReq.type = GNT_GAMELIST_REQ;
	GameListReq.len = GAME_HEADER_ONLY_SIZE;

	TrackAckPacket.game_type = (unsigned char)gametype;
	TrackAckPacket.len = GAME_HEADER_ONLY_SIZE;
	TrackAckPacket.type = GNT_CLIENT_ACK;

	GameOverPacket.game_type = (unsigned char)gametype;
	GameOverPacket.len = GAME_HEADER_ONLY_SIZE;
	GameOverPacket.type = GNT_GAMEOVER;

	// gamesock = socket(AF_INET,SOCK_DGRAM,0);
	
	/*
	if ( gamesock == INVALID_SOCKET )
	{
		printf("Unable to open a socket.\n");
		return 0;
	}
	*/
	
	memset( &sockaddr, 0, sizeof(SOCKADDR_IN) );
	sockaddr.sin_family = AF_INET; 
	sockaddr.sin_addr.s_addr = INADDR_ANY; 
	sockaddr.sin_port = 0;//htons(GAMEPORT);
	
	/*
	if (SOCKET_ERROR==bind(gamesock, (SOCKADDR*)&sockaddr, sizeof (sockaddr))) 
	{	
		printf("Unable to bind a socket.\n");
		printf("WSAGetLastError() returned %d.\n",WSAGetLastError());
		return 0;
	}
	*/
		
	iaddr = inet_addr ( Multi_options_g.game_tracker_ip ); 
	if ( iaddr == INADDR_NONE ) {
		// first try and resolve by name
		HOSTENT *he;
		he = gethostbyname( Multi_options_g.game_tracker_ip );
		if(!he)
		{		
			return 0;
			/*
			// try and resolve by address		
			unsigned int n_order = inet_addr(Multi_game_tracker_ip_address);
			he = gethostbyaddr((char*)&n_order,4,PF_INET);		

			if(!he){
				return 0;
			}
			*/
		}
		memcpy(&iaddr, he->h_addr_list[0],4);
	}

	// This would be a good place to resolve the IP based on a domain name
	memcpy(&gtrackaddr.sin_addr.s_addr, &iaddr, 4);
	gtrackaddr.sin_family = AF_INET; 
	gtrackaddr.sin_port = htons( GAMEPORT );

	//Start New 7-9-98
	SendingGameOver = 0;
	//End New 7-9-98

	return 1;
}

void IdleGameTracker()
{
	fd_set read_fds;	           
	TIMEVAL timeout;

	PSNET_TOP_LAYER_PROCESS();
	
	timeout.tv_sec=0;            
	timeout.tv_usec=0;
	if((TrackerGameIsRunning) && ((timer_get_seconds()-LastTrackerUpdate)>TRACKER_UPDATE_INTERVAL) && !SendingGameOver)
	{
		//Time to update the tracker again
		SENDTO(Unreliable_socket, (char *)&TrackerGameData,TrackerGameData.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
		TrackerAckdUs = 0;
		LastTrackerUpdate = timer_get_seconds();
	}
	else if((TrackerGameIsRunning)&&(!TrackerAckdUs)&&((timer_get_milliseconds()-LastSentToTracker)>TRACKER_RESEND_TIME))
	{
		//We still haven't been acked by the last packet and it's time to resend.
		SENDTO(Unreliable_socket, (char *)&TrackerGameData,TrackerGameData.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
		TrackerAckdUs = 0;
		LastTrackerUpdate = timer_get_seconds();
		LastSentToTracker = timer_get_milliseconds();
	}

	//Start New 7-9-98
	if(SendingGameOver){
		if((timer_get_milliseconds()-LastGameOverPacket)>TRACKER_RESEND_TIME){
			//resend
			LastGameOverPacket = timer_get_milliseconds();
			SENDTO(Unreliable_socket, (char *)&GameOverPacket,GameOverPacket.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
		} 
		/*
		else if((timer_get_milliseconds()-FirstGameOverPacket)>NET_ACK_TIMEOUT) {
			//Giving up, it timed out.
			SendingGameOver = 2;
		}
		*/
	}
	//End New 7-9-98

	//Check for incoming
		
	FD_ZERO(&read_fds);
	FD_SET(Unreliable_socket, &read_fds);    

#ifndef PLAT_UNIX
	if(SELECT(0,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_GAME_TRACKER))
#else
	if(SELECT(Unreliable_socket+1,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_GAME_TRACKER))
#endif
	{
		unsigned int bytesin;
		int addrsize;
		SOCKADDR_IN fromaddr;

		game_packet_header inpacket;
		addrsize = sizeof(SOCKADDR_IN);

		bytesin = RECVFROM(Unreliable_socket, (char *)&inpacket,sizeof(game_packet_header),0,(SOCKADDR *)&fromaddr,&addrsize, PSNET_TYPE_GAME_TRACKER);
		if((int)bytesin==-1)
		{
			int wserr=WSAGetLastError();
			printf("RECVFROM() failure. WSAGetLastError() returned %d\n",wserr);
			
		}

		// subtract one from the header
		inpacket.len--;

		//Check to make sure the packets ok
		if(bytesin==inpacket.len)
		{
			switch(inpacket.type)
			{
			case GNT_SERVER_ACK:
				//The server got our packet so we can stop sending now
				TrackerAckdUs = 1;				
				
				// 7/13/98 -- because of the FreeSpace iterative frame process -- set this value to 0, instead
				// of to 2 (as it originally was) since we call SendGameOver() only once.  Once we get the ack
				// from the server, we can assume that we are done.
				// need to mark this as 0
				SendingGameOver = 0;							
				break;
			case GNT_GAMELIST_DATA:
				int i;
				//Woohoo! Game data! put it in the buffer (if one's free)
				for(i=0;i<MAX_GAME_BUFFERS;i++)
				{
					if(GameBuffer[i].game_type==GT_UNUSED)
					{
						memcpy(&GameBuffer[i],&inpacket.data,sizeof(game_list));
						i=MAX_GAME_BUFFERS+1;
					}
				}
				break;

			case GNT_GAME_COUNT_DATA:
				//Here, inpacket.data contains the following structure
				//struct {
				//	int numusers;
				//	char channel[];//Null terminated
				//	}
				//You can add whatever code, or callback, etc. you need to deal with this data

				// let the PXO screen know about this data
				int num_servers;
				char channel[512];

				// get the user count
				memcpy(&num_servers,inpacket.data,sizeof(int));

				// copy the channel name
				memset(channel,0,512);
				strcpy(channel,inpacket.data+4);

				// send it to the PXO screen				
				multi_pxo_channel_count_update(channel,num_servers);
				break;
			}
			AckPacket(inpacket.sig);			
		}
	}
}

void UpdateGameData(void *buffer)
{
	SendingGameOver = 0;

	switch(GameType){
	case GT_FREESPACE:
#ifndef MAKE_FS1
		Int3();
#else
		memcpy(FreeSpaceTrackerGameData,buffer,sizeof(freespace_net_game_data));
#endif
		break;

	case GT_FREESPACE2:
#ifdef MAKE_FS1
		Int3();
#else
		memcpy(FreeSpace2TrackerGameData,buffer,sizeof(freespace2_net_game_data));
#endif
		break;

	default:
		Int3();
		break;
	}
}

game_list * GetGameList()
{
	static game_list gl;
	for(int i=0;i<MAX_GAME_BUFFERS;i++)
	{
		if(GameBuffer[i].game_type!=GT_UNUSED)
		{
			memcpy(&gl,&GameBuffer[i],sizeof(game_list));
			GameBuffer[i].game_type = GT_UNUSED;
			return &gl;
		}
	}
	return NULL;
}

void RequestGameList()
{
	GameListReq.len = GAME_HEADER_ONLY_SIZE;
	SENDTO(Unreliable_socket, (char *)&GameListReq,GameListReq.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
}

void RequestGameListWithFilter(void *filter)
{
	memcpy(&GameListReq.data,filter,sizeof(filter_game_list_struct));
	GameListReq.len = GAME_HEADER_ONLY_SIZE+sizeof(filter_game_list_struct);
	SENDTO(Unreliable_socket, (char *)&GameListReq,GameListReq.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
}


/* REPLACED BELOW
void SendGameOver()
{
	TrackerGameIsRunning = 0;
	sendto(gamesock,(const char *)&GameOverPacket,GameOverPacket.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN));
}
*/

//Start New 7-9-98
int SendGameOver()
{
	if(SendingGameOver==2) 
	{
		SendingGameOver = 0;	
		return 1;
	}
	if(SendingGameOver==1) 
	{
		//Wait until it's sent.
		IdleGameTracker();
		return 0;
	}
	if(SendingGameOver==0)
	{
		LastGameOverPacket = timer_get_milliseconds();
		FirstGameOverPacket = timer_get_milliseconds();
		SendingGameOver = 1;
		TrackerGameIsRunning = 0;
		SENDTO(Unreliable_socket, (char *)&GameOverPacket,GameOverPacket.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
		return 0;
	}
	return 0;
}
//End New 7-9-98

void AckPacket(int sig)
{
	TrackAckPacket.sig = sig;
	SENDTO(Unreliable_socket, (char *)&TrackAckPacket,TrackAckPacket.len,0,(SOCKADDR *)&gtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
}

void StartTrackerGame(void *buffer)
{
	SendingGameOver = 0;

	switch(GameType){
	case GT_FREESPACE:
#ifndef MAKE_FS1
		Int3();
#else
		memcpy(FreeSpaceTrackerGameData,buffer,sizeof(freespace_net_game_data));
#endif
		break;

	case GT_FREESPACE2:
#ifdef MAKE_FS1
		Int3();
#else
		memcpy(FreeSpace2TrackerGameData,buffer,sizeof(freespace2_net_game_data));
#endif
		break;

	default:
		Int3();
		break;
	}
	TrackerGameIsRunning = 1;
	LastTrackerUpdate = 0;	
}

//A new function
void RequestGameCountWithFilter(void *filter) 
{
	game_packet_header GameCountReq;

#ifdef MAKE_FS1
	GameCountReq.game_type = GT_FREESPACE;
#else
	GameCountReq.game_type = GT_FREESPACE2;
#endif
	GameCountReq.type = GNT_GAME_COUNT_REQ;
	GameCountReq.len = GAME_HEADER_ONLY_SIZE+sizeof(filter_game_list_struct);	
	memcpy(&GameCountReq.data, ((filter_game_list_struct*)filter)->channel, sizeof(filter_game_list_struct) - 4);
	SENDTO(Unreliable_socket, (char *)&GameCountReq, GameCountReq.len, 0, (SOCKADDR *)&gtrackaddr, sizeof(SOCKADDR_IN), PSNET_TYPE_GAME_TRACKER);
}
