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


// check structs for size compatibility
SDL_COMPILE_TIME_ASSERT(udp_packet_header, sizeof(udp_packet_header) == 497);
SDL_COMPILE_TIME_ASSERT(vmt_freespace2_struct, sizeof(vmt_freespace2_struct) == 440);
SDL_COMPILE_TIME_ASSERT(validate_id_request, sizeof(validate_id_request) == 60);
SDL_COMPILE_TIME_ASSERT(squad_war_request, sizeof(squad_war_request) == 104);
SDL_COMPILE_TIME_ASSERT(squad_war_response, sizeof(squad_war_response) == 256);
SDL_COMPILE_TIME_ASSERT(squad_war_result, sizeof(squad_war_result) == 72);
SDL_COMPILE_TIME_ASSERT(pilot_request, sizeof(pilot_request) == 32);


//Variables

// SOCKET	pilotsock;

struct sockaddr_in	ptrackaddr;


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


static int SerializePilotPacket(const udp_packet_header *uph, ubyte *data)
{
	int packet_size = 0;
	int i;

	PXO_ADD_DATA(uph->type);
	PXO_ADD_USHORT(uph->len);
	PXO_ADD_UINT(uph->code);
	PXO_ADD_USHORT(uph->xcode);
	PXO_ADD_UINT(uph->sig);
	PXO_ADD_UINT(uph->security);

	switch (uph->type) {
		// no extra data for this
		case UNT_CONTROL:
			break;

		case UNT_PILOT_DATA_WRITE_NEW: {
			vmt_freespace2_struct *fs2 = (vmt_freespace2_struct *)&uph->data;

			PXO_ADD_DATA(fs2->tracker_id);
			PXO_ADD_DATA(fs2->pilot_name);

			PXO_ADD_DATA(fs2->pad_a);	// junk, for size/alignment

			PXO_ADD_INT(fs2->score);
			PXO_ADD_INT(fs2->rank);
			PXO_ADD_INT(fs2->assists);
			PXO_ADD_INT(fs2->kill_count);
			PXO_ADD_INT(fs2->kill_count_ok);
			PXO_ADD_UINT(fs2->p_shots_fired);
			PXO_ADD_UINT(fs2->s_shots_fired);

			PXO_ADD_UINT(fs2->p_shots_hit);
			PXO_ADD_UINT(fs2->s_shots_hit);

			PXO_ADD_UINT(fs2->p_bonehead_hits);
			PXO_ADD_UINT(fs2->s_bonehead_hits);
			PXO_ADD_INT(fs2->bonehead_kills);

			PXO_ADD_INT(fs2->security);
			PXO_ADD_DATA(fs2->virgin_pilot);

			PXO_ADD_DATA(fs2->pad_b);	// junk, for size/alignment

			PXO_ADD_UINT(fs2->checksum);

			PXO_ADD_UINT(fs2->missions_flown);
			PXO_ADD_UINT(fs2->flight_time);
			PXO_ADD_UINT(fs2->last_flown);

			PXO_ADD_USHORT(fs2->num_medals);
			PXO_ADD_USHORT(fs2->num_ship_types);

			for (i = 0; i < MAX_FS2_MEDALS; i++) {
				PXO_ADD_INT(fs2->medals[i]);
			}

			for (i =0; i < MAX_FS2_SHIP_TYPES; i++) {
				PXO_ADD_USHORT(fs2->kills[i]);
			}

			break;
		}

		case UNT_SW_RESULT_WRITE: {
			squad_war_result *sw_result	= (squad_war_result *)&uph->data;

			PXO_ADD_DATA(sw_result->match_code);
			PXO_ADD_DATA(sw_result->result);
			PXO_ADD_DATA(sw_result->squad_count1);
			PXO_ADD_DATA(sw_result->squad_count2);
			PXO_ADD_DATA(sw_result->pad);		// junk, for size/alignment

			for (i = 0; i < MAX_SQUAD_PLAYERS; i++) {
				PXO_ADD_INT(sw_result->squad_winners[i]);
			}

			for (i = 0; i < MAX_SQUAD_PLAYERS; i++) {
				PXO_ADD_INT(sw_result->squad_losers[i]);
			}

			break;
		}

		case UNT_PILOT_DATA_READ_NEW:
		case UNT_PILOT_DATA_READ: {
			pilot_request *pr = (pilot_request *)&uph->data;

			PXO_ADD_DATA(pr->pilot_name);
			PXO_ADD_DATA(pr->tracker_id);
			PXO_ADD_DATA(pr->pad);		// junk, for size/alignment

			break;
		}

		// we shouldn't be sending any other packet types
		default:
			Int3();
			break;
	}

	SDL_assert(packet_size >= (int)PACKED_HEADER_ONLY_SIZE);
	SDL_assert(packet_size == (int)uph->len);

	return packet_size;
}

static void DeserializePilotPacket(const ubyte *data, const int data_size, udp_packet_header *uph)
{
	int offset = 0;
	int i;

	memset(uph, 0, sizeof(udp_packet_header));

	// make sure we received a complete base packet
	if (data_size < (int)PACKED_HEADER_ONLY_SIZE) {
		uph->len = 0;
		uph->type = 0xff;

		return;
	}

	PXO_GET_DATA(uph->type);
	PXO_GET_USHORT(uph->len);
	PXO_GET_UINT(uph->code);
	PXO_GET_USHORT(uph->xcode);
	PXO_GET_UINT(uph->sig);
	PXO_GET_UINT(uph->security);

	// sanity check data size to make sure we reveived all of the expected packet
	// (not exactly sure what -1 is for, but that's how it is later)
	if ((int)uph->len-1 > data_size) {
		uph->len = 0;
		uph->type = 0xff;

		return;
	}

	switch (uph->type) {
		// no extra data for these
		case UNT_PILOT_READ_FAILED:
		case UNT_PILOT_WRITE_SUCCESS:
		case UNT_PILOT_WRITE_FAILED:
			break;

		case UNT_PILOT_DATA_RESPONSE: {
			vmt_freespace2_struct *fs2 = (vmt_freespace2_struct *)&uph->data;

			PXO_GET_DATA(fs2->tracker_id);
			PXO_GET_DATA(fs2->pilot_name);

			PXO_GET_DATA(fs2->pad_a);	// junk, for size/alignment

			PXO_GET_INT(fs2->score);
			PXO_GET_INT(fs2->rank);
			PXO_GET_INT(fs2->assists);
			PXO_GET_INT(fs2->kill_count);
			PXO_GET_INT(fs2->kill_count_ok);
			PXO_GET_UINT(fs2->p_shots_fired);
			PXO_GET_UINT(fs2->s_shots_fired);

			PXO_GET_UINT(fs2->p_shots_hit);
			PXO_GET_UINT(fs2->s_shots_hit);

			PXO_GET_UINT(fs2->p_bonehead_hits);
			PXO_GET_UINT(fs2->s_bonehead_hits);
			PXO_GET_INT(fs2->bonehead_kills);

			PXO_GET_INT(fs2->security);
			PXO_GET_DATA(fs2->virgin_pilot);

			PXO_GET_DATA(fs2->pad_b);	// junk, for size/alignment

			PXO_GET_UINT(fs2->checksum);

			PXO_GET_UINT(fs2->missions_flown);
			PXO_GET_UINT(fs2->flight_time);
			PXO_GET_UINT(fs2->last_flown);

			PXO_GET_USHORT(fs2->num_medals);
			PXO_GET_USHORT(fs2->num_ship_types);

			for (i = 0; i < MAX_FS2_MEDALS; i++) {
				PXO_GET_INT(fs2->medals[i]);
			}

			for (i =0; i < MAX_FS2_SHIP_TYPES; i++) {
				PXO_GET_USHORT(fs2->kills[i]);
			}

			break;
		}

		case UNT_SW_RESULT_RESPONSE: {
			squad_war_response *sw_resp = (squad_war_response *)&uph->data;

			PXO_GET_DATA(sw_resp->reason);
			PXO_GET_DATA(sw_resp->accepted);

			break;
		}

		default:
			break;
	}

	//SDL_assert(offset == data_size);
}


int InitPilotTrackerClient()
{
	struct sockaddr_in sockaddr;
	in_addr_t iaddr;

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
	
	memset( &sockaddr, 0, sizeof(struct sockaddr_in) );
	sockaddr.sin_family = AF_INET; 
	sockaddr.sin_addr.s_addr = INADDR_ANY; 
	sockaddr.sin_port = 0;//htons(REGPORT);
	
	/*
	if (SOCKET_ERROR==bind(pilotsock, (struct sockaddr*)&sockaddr, sizeof (sockaddr)))
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
		struct hostent *he;
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
		iaddr = ((in_addr *)(he->h_addr))->s_addr;
	}
	
	ptrackaddr.sin_addr.s_addr = iaddr;
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
		SDL_strlcpy(fs_pr->pilot_name, pilot_name, SDL_arraysize(fs_pr->pilot_name));
		SDL_strlcpy(fs_pr->tracker_id, tracker_id, SDL_arraysize(fs_pr->tracker_id));

		return 0;	
	}
	return -2;

}

// Send an ACK to the server
void AckServer(unsigned int sig)
{
	udp_packet_header ack_pack;
	ubyte packet_data[sizeof(udp_packet_header)];
	int packet_length = 0;

	ack_pack.type = UNT_CONTROL;
	ack_pack.sig = sig;
	ack_pack.code = CMD_CLIENT_RECEIVED;
	ack_pack.len = PACKED_HEADER_ONLY_SIZE;	

	packet_length = SerializePilotPacket(&ack_pack, packet_data);
	SDL_assert(packet_length == PACKED_HEADER_ONLY_SIZE);
	SENDTO(Unreliable_socket, (char *)&packet_data, packet_length, 0, (struct sockaddr *)&ptrackaddr, sizeof(struct sockaddr_in), PSNET_TYPE_USER_TRACKER);
}

void IdlePTrack()
{
	ubyte packet_data[sizeof(udp_packet_header)];
	int packet_length = 0;

	PSNET_TOP_LAYER_PROCESS();

	// reading pilot data
	if(FSReadState == STATE_READING_PILOT){
		if((timer_get_milliseconds()-FSFirstSent)>=PILOT_REQ_TIMEOUT){
			FSReadState = STATE_TIMED_OUT;
		} else if((timer_get_milliseconds()-FSLastSent)>=PILOT_REQ_RESEND_TIME){
			//Send 'da packet
			packet_length = SerializePilotPacket(&fs_pilot_req, packet_data);
			SENDTO(Unreliable_socket, (char *)&packet_data, packet_length, 0, (struct sockaddr *)&ptrackaddr, sizeof(struct sockaddr_in), PSNET_TYPE_USER_TRACKER);
			FSLastSent = timer_get_milliseconds();
		}
	}

	// writing pilot data
	if(FSWriteState == STATE_SENDING_PILOT){
		if((timer_get_milliseconds()-FSFirstSentWrite)>=PILOT_REQ_TIMEOUT){
			FSWriteState = STATE_TIMED_OUT;

		} else if((timer_get_milliseconds()-FSLastSentWrite)>=PILOT_REQ_RESEND_TIME){
			// Send 'da packet
			packet_length = SerializePilotPacket(&fs_pilot_write, packet_data);
			SENDTO(Unreliable_socket, (char *)&packet_data, packet_length, 0, (struct sockaddr *)&ptrackaddr, sizeof(struct sockaddr_in), PSNET_TYPE_USER_TRACKER);
			FSLastSentWrite = timer_get_milliseconds();
		}
	}

	// writing squad war results
	if(SWWriteState == STATE_SENDING_PILOT){
		if((timer_get_milliseconds()-SWFirstSentWrite) >= PILOT_REQ_TIMEOUT){
			SWWriteState = STATE_TIMED_OUT;
		} else if((timer_get_milliseconds()-SWLastSentWrite) >= PILOT_REQ_RESEND_TIME){
			// Send 'da packet
			packet_length = SerializePilotPacket(&sw_res_write, packet_data);
			SENDTO(Unreliable_socket, (char *)&packet_data, packet_length, 0, (struct sockaddr *)&ptrackaddr, sizeof(struct sockaddr_in), PSNET_TYPE_USER_TRACKER);
			SWLastSentWrite = timer_get_milliseconds();
		}
	}
}

void PollPTrackNet()
{
	fd_set read_fds;	           
	struct timeval timeout;
	ubyte packet_data[sizeof(udp_packet_header)];

	IdlePTrack();
	
	timeout.tv_sec=0;            
	timeout.tv_usec=0;
	
	FD_ZERO(&read_fds);
	FD_SET(Unreliable_socket, &read_fds);    

	if(SELECT(Unreliable_socket+1, &read_fds,NULL,NULL,&timeout, PSNET_TYPE_USER_TRACKER)){
		int bytesin;
		int addrsize;
		struct sockaddr_in fromaddr;

		udp_packet_header inpacket;

		SDL_zero(inpacket);
		addrsize = sizeof(struct sockaddr_in);

		bytesin = RECVFROM(Unreliable_socket, (char *)&packet_data, sizeof(udp_packet_header), 0, (struct sockaddr *)&fromaddr, &addrsize, PSNET_TYPE_USER_TRACKER);

		if (bytesin > 0) {
			DeserializePilotPacket(packet_data, bytesin, &inpacket);

			// decrease packet size by 1
			inpacket.len--;
#ifndef NDEBUG
		} else {
			int wserr=WSAGetLastError();
			mprintf(("recvfrom() failure. WSAGetLastError() returned %d\n",wserr));
#endif
		}

		//Check to make sure the packets ok
		if ( (bytesin > 0) && (bytesin == inpacket.len) ) {
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
