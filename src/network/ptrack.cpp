/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

//Pilot tracker client code

#ifdef PLAT_UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#endif

#include "pstypes.h"
#include "timer.h"
#include "multi.h"
#include "ptrack.h"
#include "psnet.h"

//Variables

// SOCKET	pilotsock;

SOCKADDR_IN	ptrackaddr;


int		FSWriteState;
int		FSReadState;
unsigned int	FSLastSentWrite;
unsigned int	FSFirstSentWrite;
unsigned int	FSLastSent;
unsigned int	FSFirstSent;

int		SWWriteState;
unsigned int SWLastSentWrite;
unsigned int SWFirstSentWrite;


udp_packet_header fs_pilot_req, fs_pilot_write, sw_res_write;
pilot_request *fs_pr;
#ifdef MAKE_FS1
vmt_freespace_struct *ReadFSPilot;
#else
vmt_freespace2_struct *ReadFSPilot;
#endif

// squad war response
squad_war_response SquadWarWriteResponse;

int InitPilotTrackerClient()
{
	SOCKADDR_IN sockaddr;
	unsigned long iaddr;

	FSWriteState = STATE_IDLE;
	FSReadState = STATE_IDLE;	
	SWWriteState = STATE_IDLE;
	
	ReadFSPilot = NULL;

	fs_pr = (pilot_request *)&fs_pilot_req.data;

	/*
	pilotsock = socket(AF_INET,SOCK_DGRAM,0);
	
	if ( pilotsock == INVALID_SOCKET )
	{
		printf("Unable to open a socket.\n");
		return 0;
	}
	*/
	
	memset( &sockaddr, 0, sizeof(SOCKADDR_IN) );
	sockaddr.sin_family = AF_INET; 
	sockaddr.sin_addr.s_addr = INADDR_ANY; 
	sockaddr.sin_port = 0;//htons(REGPORT);
	
	/*
	if (SOCKET_ERROR==bind(pilotsock, (SOCKADDR*)&sockaddr, sizeof (sockaddr))) 
	{	
		printf("Unable to bind a socket.\n");
		printf("WSAGetLastError() returned %d.\n",WSAGetLastError());
		return 0;
	}
	*/
	
	// iaddr = inet_addr ( Multi_user_tracker_ip_address ); 

	// first try and resolve by name
	iaddr = inet_addr( Multi_options_g.user_tracker_ip );
	if ( iaddr == INADDR_NONE ) {
		HOSTENT *he;
		he = gethostbyname( Multi_options_g.user_tracker_ip );
		if(!he)
			return 0;
	/*
		{		
			// try and resolve by address
			unsigned int n_order = inet_addr(Multi_user_tracker_ip_address);
			he = gethostbyaddr((char*)&n_order,4,PF_INET);

			if(!he){
				return 0;
			}
		}
	*/
		memcpy(&iaddr, he->h_addr_list[0],4);
	}
	
	memcpy(&ptrackaddr.sin_addr.s_addr, &iaddr, 4);
	ptrackaddr.sin_family = AF_INET; 
	ptrackaddr.sin_port = htons(REGPORT);
	
	return 1;
}

// Returns:
// -3	Error -- Called with NULL, but no request is waiting
// -2	Error -- Already sending data (hasn't timed out yet)
// -1	Timeout trying to send pilot data
// 0	Sending
// 1	Data succesfully sent
// 2	Send Cancelled (data may still have been written already, we just haven't been ACK'd yet)
// 3	Pilot not written (for some reason)
  
// Call with NULL to poll 
// Call with -1 to cancel send
// Call with valid pointer to a vmt_descent3_struct to initiate send
#ifdef MAKE_FS1
int SendFSPilotData(vmt_freespace_struct *fs_pilot)
#else
int SendFSPilotData(vmt_freespace2_struct *fs_pilot)
#endif
{
	//First check the network
	PollPTrackNet();

	if(fs_pilot == NULL)
	{
		if(FSWriteState == STATE_IDLE)
		{
			return -3;
		}
		if(FSWriteState == STATE_SENDING_PILOT)
		{
			return 0;
		}
		if(FSWriteState == STATE_WROTE_PILOT)
		{
			//We wrote this pilot, and now we are about to inform the app, so back to idle
			FSWriteState = STATE_IDLE;
			return 1;
		}
		if(FSWriteState == STATE_TIMED_OUT)
		{
			//We gave up on sending this pilot, and now we are about to inform the app, so back to idle
			FSWriteState = STATE_IDLE;

			return -1;
		}
		if(FSWriteState == STATE_WRITE_PILOT_FAILED)
		{
			//The tracker said this dude couldn't be written
			FSWriteState = STATE_IDLE;

			return 3;
		}

	}
	else if(fs_pilot == (vmt_freespace2_struct*)0xffffffff)
	{
		if(FSWriteState == STATE_IDLE)
		{
			return -3;
		}
		else
		{
			//Cancel this baby
			FSWriteState = STATE_IDLE;

			return 2;
		}

	}
	else if(FSWriteState == STATE_IDLE)
	{
		//New request, send out the req, and go for it.
		
		FSWriteState = STATE_SENDING_PILOT;
		
		FSLastSentWrite = 0;
		FSFirstSentWrite = timer_get_milliseconds();

		fs_pilot_write.type = UNT_PILOT_DATA_WRITE_NEW;
#ifdef MAKE_FS1
		fs_pilot_write.len = PACKED_HEADER_ONLY_SIZE+sizeof(vmt_freespace_struct);
		fs_pilot_write.code = CMD_GAME_FREESPACE;
		memcpy(&fs_pilot_write.data,fs_pilot,sizeof(vmt_freespace_struct));
#else
		fs_pilot_write.len = PACKED_HEADER_ONLY_SIZE+sizeof(vmt_freespace2_struct);
		fs_pilot_write.code = CMD_GAME_FREESPACE2;
		memcpy(&fs_pilot_write.data,fs_pilot,sizeof(vmt_freespace2_struct));
#endif

		return 0;	
	}
	return -2;
}

// Returns:
// -3	Error -- Called with NULL, but no request is waiting
// -2	Error -- Already sending data (hasn't timed out yet)
// -1	Timeout trying to send pilot data
// 0	Sending
// 1	Data succesfully sent
// 2	Send Cancelled (data may still have been written already, we just haven't been ACK'd yet)
// 3	Pilot not written (for some reason)
  
// Call with NULL to poll 
// Call with -1 to cancel send
// Call with valid pointer to a vmt_descent3_struct to initiate send
int SendSWData(squad_war_result *sw_res, squad_war_response *sw_resp)
{
	//First check the network
	PollPTrackNet();

	if(sw_res == NULL){
		if(SWWriteState == STATE_IDLE){
			return -3;
		}
		if(SWWriteState == STATE_SENDING_PILOT){
			return 0;
		}

		// fill in the response
		if(SWWriteState == STATE_WROTE_PILOT){
			// We wrote this pilot, and now we are about to inform the app, so back to idle
			SWWriteState = STATE_IDLE;
			
			if(sw_resp != NULL){
				memcpy(sw_resp, &SquadWarWriteResponse, sizeof(squad_war_response));
			}
			return 1;
		}
		// fill in the response
		if(SWWriteState == STATE_WRITE_PILOT_FAILED){
			// The tracker said this dude couldn't be written		
			SWWriteState = STATE_IDLE;

			if(sw_resp != NULL){
				memcpy(sw_resp, &SquadWarWriteResponse, sizeof(squad_war_response));
			}
			return 3;
		}

		if(SWWriteState == STATE_TIMED_OUT){
			// We gave up on sending this pilot, and now we are about to inform the app, so back to idle
			SWWriteState = STATE_IDLE;

			return -1;
		}		
	} else if(sw_res == (squad_war_result*)0xffffffff){
		if(SWWriteState == STATE_IDLE){
			return -3;
		} else {
			// Cancel this baby
			SWWriteState = STATE_IDLE;

			return 2;
		}
	} else if(SWWriteState == STATE_IDLE) {
		//New request, send out the req, and go for it.
		
		SWWriteState = STATE_SENDING_PILOT;
		
		SWLastSentWrite = 0;
		SWFirstSentWrite = timer_get_milliseconds();

		sw_res_write.len = PACKED_HEADER_ONLY_SIZE+sizeof(squad_war_result);
		sw_res_write.type = UNT_SW_RESULT_WRITE;
#ifdef MAKE_FS1
		sw_res_write.code = CMD_GAME_FREESPACE;
#else
		sw_res_write.code = CMD_GAME_FREESPACE2;
#endif
		memcpy(&sw_res_write.data, sw_res, sizeof(squad_war_result));

		return 0;	
	}
	return -2;
}


// Returns:
// -3	Error -- Called with NULL, but no request is waiting
// -2	Error -- Already waiting on data (hasn't timed out yet)
// -1	Timeout waiting for pilot data
// 0	Waiting for data
// 1	Data received
// 2	Get Cancelled
// 3	Pilot not found
	
// Call with NULL to poll 
// Call with -1 to cancel wait
// Call with valid pointer to a vmt_descent3_struct to get a response
#ifdef MAKE_FS1
int GetFSPilotData(vmt_freespace_struct *fs_pilot, const char *pilot_name, const char *tracker_id, int get_security)
#else
int GetFSPilotData(vmt_freespace2_struct *fs_pilot, const char *pilot_name, const char *tracker_id, int get_security)
#endif
{
	//First check the network
	PollPTrackNet();

	if(fs_pilot == NULL)
	{
		if(FSReadState == STATE_IDLE)
		{
			return -3;
		}
		if(FSReadState == STATE_READING_PILOT)
		{
			return 0;
		}
		if(FSReadState == STATE_RECEIVED_PILOT)
		{
			// We got this pilot, and now we are about to inform the app, so back to idle
			FSReadState = STATE_IDLE;
			ReadFSPilot = NULL;
			return 1;
		}
		if(FSReadState == STATE_TIMED_OUT)
		{
			// We gave up on this pilot, and now we are about to inform the app, so back to idle
			FSReadState = STATE_IDLE;
			ReadFSPilot = NULL;
			return -1;
		}
		if(FSReadState == STATE_PILOT_NOT_FOUND)
		{
			//The tracker said this dude is not found.
			FSReadState = STATE_IDLE;
			ReadFSPilot = NULL;
			return 3;
		}

	}
	else if(fs_pilot == (vmt_freespace2_struct*)0xffffffff)
	{
		if(FSReadState == STATE_IDLE)
		{
			return -3;
		}
		else
		{
			//Cancel this baby
			FSReadState = STATE_IDLE;
			ReadFSPilot = NULL;
			return 2;
		}

	}
	else if(FSReadState == STATE_IDLE)
	{
		//New request, send out the req, and go for it.
		
		FSReadState = STATE_READING_PILOT;
		ReadFSPilot = fs_pilot;
		FSLastSent = 0;
		FSFirstSent = timer_get_milliseconds();

		fs_pilot_req.len = PACKED_HEADER_ONLY_SIZE+sizeof(pilot_request);

		if(get_security){
			fs_pilot_req.type = UNT_PILOT_DATA_READ_NEW;
		} else {
			fs_pilot_req.type = UNT_PILOT_DATA_READ;			
		}

#ifdef MAKE_FS1
		fs_pilot_req.code = CMD_GAME_FREESPACE;
#else
		fs_pilot_req.code = CMD_GAME_FREESPACE2;
#endif
		strcpy(fs_pr->pilot_name,pilot_name);
		strncpy(fs_pr->tracker_id,tracker_id,TRACKER_ID_LEN);

		return 0;	
	}
	return -2;

}

// Send an ACK to the server
void AckServer(unsigned int sig)
{
	udp_packet_header ack_pack;

	ack_pack.type = UNT_CONTROL;
	ack_pack.sig = sig;
	ack_pack.code = CMD_CLIENT_RECEIVED;
	ack_pack.len = PACKED_HEADER_ONLY_SIZE;	
	
	SENDTO(Unreliable_socket, (char *)&ack_pack,PACKED_HEADER_ONLY_SIZE,0,(SOCKADDR *)&ptrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_USER_TRACKER);
}

void IdlePTrack()
{
	PSNET_TOP_LAYER_PROCESS();

	// reading pilot data
	if(FSReadState == STATE_READING_PILOT){
		if((timer_get_milliseconds()-FSFirstSent)>=PILOT_REQ_TIMEOUT){
			FSReadState = STATE_TIMED_OUT;
		} else if((timer_get_milliseconds()-FSLastSent)>=PILOT_REQ_RESEND_TIME){
			//Send 'da packet
			SENDTO(Unreliable_socket, (char *)&fs_pilot_req,fs_pilot_req.len,0,(SOCKADDR *)&ptrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_USER_TRACKER);
			FSLastSent = timer_get_milliseconds();
		}
	}

	// writing pilot data
	if(FSWriteState == STATE_SENDING_PILOT){
		if((timer_get_milliseconds()-FSFirstSentWrite)>=PILOT_REQ_TIMEOUT){
			FSWriteState = STATE_TIMED_OUT;

		} else if((timer_get_milliseconds()-FSLastSentWrite)>=PILOT_REQ_RESEND_TIME){
			// Send 'da packet
			SENDTO(Unreliable_socket, (char *)&fs_pilot_write,fs_pilot_write.len,0,(SOCKADDR *)&ptrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_USER_TRACKER);
			FSLastSentWrite = timer_get_milliseconds();
		}
	}

	// writing squad war results
	if(SWWriteState == STATE_SENDING_PILOT){
		if((timer_get_milliseconds()-SWFirstSentWrite) >= PILOT_REQ_TIMEOUT){
			SWWriteState = STATE_TIMED_OUT;
		} else if((timer_get_milliseconds()-SWLastSentWrite) >= PILOT_REQ_RESEND_TIME){
			// Send 'da packet
			SENDTO(Unreliable_socket, (char *)&sw_res_write, sw_res_write.len, 0, (SOCKADDR *)&ptrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_USER_TRACKER);
			SWLastSentWrite = timer_get_milliseconds();
		}
	}
}

void PollPTrackNet()
{
	fd_set read_fds;	           
	TIMEVAL timeout;

	IdlePTrack();
	
	timeout.tv_sec=0;            
	timeout.tv_usec=0;
	
	FD_ZERO(&read_fds);
	FD_SET(Unreliable_socket, &read_fds);    

#ifndef PLAT_UNIX
	if(SELECT(0, &read_fds,NULL,NULL,&timeout, PSNET_TYPE_USER_TRACKER)){
#else
	if(SELECT(Unreliable_socket+1, &read_fds,NULL,NULL,&timeout, PSNET_TYPE_USER_TRACKER)){
#endif
		int bytesin;
		int addrsize;
		SOCKADDR_IN fromaddr;

		udp_packet_header inpacket;
		addrsize = sizeof(SOCKADDR_IN);

		bytesin = RECVFROM(Unreliable_socket, (char *)&inpacket,sizeof(udp_packet_header),0,(SOCKADDR *)&fromaddr,&addrsize, PSNET_TYPE_USER_TRACKER);
		if(bytesin==-1){
			int wserr=WSAGetLastError();
			printf("recvfrom() failure. WSAGetLastError() returned %d\n",wserr);
			
		}

		// decrease packet size by 1
		inpacket.len--;

		//Check to make sure the packets ok
		if(bytesin==inpacket.len){
			switch(inpacket.type){
			case UNT_PILOT_DATA_RESPONSE:
				if(inpacket.code == CMD_GAME_FREESPACE2){
#ifdef MAKE_FS1
					Int3();
#else
					if(FSReadState == STATE_READING_PILOT){
						vmt_freespace2_struct *stats;
						
						// 9/17/98 MWA.  Compare the tracker id of this packet with the tracker id of
						// what we are expecting.  Only set our state to something different when
						// the tracker id's match.  This fixes possible multiple packets for a single pilto
						// accidentally getting set for the wrong pilot

						stats = (vmt_freespace2_struct *)(&inpacket.data);
						if ( !SDL_strncasecmp(stats->tracker_id, fs_pr->tracker_id,TRACKER_ID_LEN) ) {
							//Copy the data
							memcpy(ReadFSPilot,&inpacket.data,sizeof(vmt_freespace2_struct));
							//Set the state 
							FSReadState = STATE_RECEIVED_PILOT;
						}
					}
#endif
				} else if(inpacket.code == CMD_GAME_FREESPACE){
#ifndef MAKE_FS1
					Int3();
#else
					if(FSReadState == STATE_READING_PILOT){
						vmt_freespace_struct *stats;

						// 9/17/98 MWA.  Compare the tracker id of this packet with the tracker id of
						// what we are expecting.  Only set our state to something different when
						// the tracker id's match.  This fixes possible multiple packets for a single pilto
						// accidentally getting set for the wrong pilot

						stats = (vmt_freespace_struct *)(&inpacket.data);
						if ( !SDL_strncasecmp(stats->tracker_id, fs_pr->tracker_id,TRACKER_ID_LEN) ) {
							//Copy the data
							memcpy(ReadFSPilot,&inpacket.data,sizeof(vmt_freespace_struct));
							//Set the state
							FSReadState = STATE_RECEIVED_PILOT;
						}
					}
#endif
				} else {
					Int3();
				}
				break;

			case UNT_PILOT_READ_FAILED:
				if(inpacket.code == CMD_GAME_FREESPACE2){
#ifdef MAKE_FS1
					Int3();
#else
					if(FSReadState == STATE_READING_PILOT){
						FSReadState = STATE_PILOT_NOT_FOUND;
					}
#endif
				} else if(inpacket.code == CMD_GAME_FREESPACE){
#ifndef MAKE_FS1
					Int3();
#else
					if(FSReadState == STATE_READING_PILOT){
						FSReadState = STATE_PILOT_NOT_FOUND;
					}
#endif
				} else {
					Int3();
				}
				break;

			case UNT_PILOT_WRITE_SUCCESS:
				if(inpacket.code == CMD_GAME_FREESPACE2){
#ifdef MAKE_FS1
					Int3();
#else
					if(FSWriteState == STATE_SENDING_PILOT){
						FSWriteState = STATE_WROTE_PILOT;
					}
#endif
				} else if(inpacket.code == CMD_GAME_FREESPACE){
#ifndef MAKE_FS1
					Int3();
#else
					if(FSWriteState == STATE_SENDING_PILOT){
						FSWriteState = STATE_WROTE_PILOT;
					}
#endif
				} else {
					Int3();
				}
				break;

			case UNT_PILOT_WRITE_FAILED:
				if(inpacket.code == CMD_GAME_FREESPACE2){
#ifdef MAKE_FS1
					Int3();
#else
					if(FSWriteState == STATE_SENDING_PILOT){
						FSWriteState = STATE_WRITE_PILOT_FAILED;
					}
#endif
				} else  if(inpacket.code == CMD_GAME_FREESPACE){
#ifndef MAKE_FS1
					Int3();
#else
					if(FSWriteState == STATE_SENDING_PILOT){
						FSWriteState = STATE_WRITE_PILOT_FAILED;
					}
#endif
				} else {
					Int3();
				}
				break;

			case UNT_SW_RESULT_RESPONSE:
				if(SWWriteState == STATE_SENDING_PILOT){					
					// copy the data
					SDL_assert((bytesin - PACKED_HEADER_ONLY_SIZE) == sizeof(squad_war_response));
					if((bytesin - PACKED_HEADER_ONLY_SIZE) == sizeof(squad_war_response)){
						memset(&SquadWarWriteResponse, 0, sizeof(squad_war_response));
						memcpy(&SquadWarWriteResponse, inpacket.data, sizeof(squad_war_response));

						// now check to see if we're good
						if(SquadWarWriteResponse.accepted){
							SWWriteState = STATE_WROTE_PILOT;
						} else {
							SWWriteState = STATE_WRITE_PILOT_FAILED;
						}
					} else {
						SWWriteState = STATE_WRITE_PILOT_FAILED;
					}	
				}
				break;

			case UNT_CONTROL:
				Int3();
				break;

			case UNT_CONTROL_VALIDATION:
				Int3();
				break;

			default:
				break;
			}
			AckServer(inpacket.sig);
		}
	}
}
