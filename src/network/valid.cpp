/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/


//Validate tracker user class


#ifdef PLAT_UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#endif

#include "multi.h"
#include "ptrack.h"
#include "valid.h"
#include "psnet.h"
#include "timer.h"


// Variables
udp_packet_header PacketHeader;
validate_id_request *ValidIDReq;

int ValidState;

// SOCKET validsock;
SOCKADDR_IN	rtrackaddr;

int ValidFirstSent;
int ValidLastSent;

char *Psztracker_id;

// mission validation
int MissionValidState;
int MissionValidFirstSent;
int MissionValidLastSent;

// squad war validation
int SquadWarValidState;
int SquadWarFirstSent;
int SquadWarLastSent;

// squad war response
squad_war_response SquadWarValidateResponse;

int InitValidateClient(void)
{
	SOCKADDR_IN sockaddr;
	unsigned long iaddr;
	ValidFirstSent = 0;
	ValidLastSent = 0;
	ValidState = VALID_STATE_IDLE;
	
	MissionValidFirstSent = 0;
	MissionValidLastSent = 0;
	MissionValidState = VALID_STATE_IDLE;

	SquadWarFirstSent = 0;
	SquadWarLastSent = 0;
	SquadWarValidState = VALID_STATE_IDLE;

	/*
	validsock = socket(AF_INET,SOCK_DGRAM,0);	
	if ( validsock == INVALID_SOCKET )
	{
		printf("Unable to open a socket.\n");
		return 0;
	}
	*/
	
	memset( &sockaddr, 0, sizeof(SOCKADDR_IN) );
	sockaddr.sin_family = AF_INET; 
	sockaddr.sin_addr.s_addr = INADDR_ANY; 
	sockaddr.sin_port = 0;
	
	/*
	if (SOCKET_ERROR==bind(validsock, (SOCKADDR*)&sockaddr, sizeof (sockaddr))) 
	{	
		printf("Unable to bind a socket.\n");
		printf("WSAGetLastError() returned %d.\n",WSAGetLastError());
		return 0;
	}
	*/

	rtrackaddr.sin_family = AF_INET; 
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
	
	memcpy(&rtrackaddr.sin_addr.s_addr, &iaddr, 4);
	rtrackaddr.sin_port = htons(REGPORT);
	
	return 1;

}

//Call with a valid struct to validate a user
//Call with NULL to poll

//Return codes:
// -3	Still waiting (returned if we were waiting for a tracker response and ValidateUser was called with a non-NULL value
// -2 Timeout waiting for tracker to respond
// -1	User invalid
//  0	Still waiting for response from tracker/Idle
//  1	User valid
int ValidateUser(validate_id_request *valid_id, char *trackerid)
{	
	ValidIdle();
	if(valid_id==NULL)
	{
		switch(ValidState)
		{
		case VALID_STATE_IDLE:
			return 0;
			break;
		case VALID_STATE_WAITING:
			return 0;
			break;
		case VALID_STATE_VALID:
			ValidState = VALID_STATE_IDLE;
			return 1;
			break;
		case VALID_STATE_INVALID:
			ValidState = VALID_STATE_IDLE;
			return -1;
		case VALID_STATE_TIMEOUT:
			ValidState = VALID_STATE_IDLE;
			return -2;
		}
		return 0;
	}
	else
	{
		if(ValidState==VALID_STATE_IDLE)
		{
			//First, flush the input buffer for the socket
			fd_set read_fds;	           
			TIMEVAL timeout;   
			
			timeout.tv_sec=0;            
			timeout.tv_usec=0;
			
			FD_ZERO(&read_fds);
			FD_SET(Unreliable_socket, &read_fds);    

#ifndef PLAT_UNIX
			while(SELECT(0,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION))
#else
			while(SELECT(Unreliable_socket+1,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION))
#endif
			{
				int addrsize;
				SOCKADDR_IN fromaddr;

				udp_packet_header inpacket;
				addrsize = sizeof(SOCKADDR_IN);
				RECVFROM(Unreliable_socket, (char *)&inpacket,sizeof(udp_packet_header),0,(SOCKADDR *)&fromaddr,&addrsize, PSNET_TYPE_VALIDATION);
			}
			Psztracker_id = trackerid;

			//Build the request packet
			PacketHeader.type = UNT_LOGIN_AUTH_REQUEST;
			PacketHeader.len = PACKED_HEADER_ONLY_SIZE+sizeof(validate_id_request);
			ValidIDReq=(validate_id_request *)&PacketHeader.data;
			strcpy(ValidIDReq->login,valid_id->login);
			strcpy(ValidIDReq->password,valid_id->password);

			SENDTO(Unreliable_socket, (char *)&PacketHeader,PacketHeader.len,0,(SOCKADDR *)&rtrackaddr,sizeof(SOCKADDR), PSNET_TYPE_VALIDATION);
			ValidState = VALID_STATE_WAITING;
			ValidFirstSent = timer_get_milliseconds();
			ValidLastSent = timer_get_milliseconds();
			return 0;
		}
		else
		{
			return -3;
		}
	}
}


void ValidIdle()
{
	fd_set read_fds;	           
	TIMEVAL timeout;   

	PSNET_TOP_LAYER_PROCESS();
	
	timeout.tv_sec=0;            
	timeout.tv_usec=0;
	
	FD_ZERO(&read_fds);
	FD_SET(Unreliable_socket, &read_fds);    

#ifndef PLAT_UNIX
	if(SELECT(0,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION)){
#else
	if(SELECT(Unreliable_socket+1,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION)){
#endif
		int bytesin;
		int addrsize;
		SOCKADDR_IN fromaddr;

		udp_packet_header inpacket;
		addrsize = sizeof(SOCKADDR_IN);

		bytesin = RECVFROM(Unreliable_socket, (char *)&inpacket, sizeof(udp_packet_header),0,(SOCKADDR *)&fromaddr,&addrsize, PSNET_TYPE_VALIDATION);
		if(bytesin==-1){
			int wserr=WSAGetLastError();
			printf("recvfrom() failure. WSAGetLastError() returned %d\n",wserr);
			
		}
		FD_ZERO(&read_fds);
		FD_SET(Unreliable_socket, &read_fds);    
		
		// decrease packet size by 1
		inpacket.len--;

		//Check to make sure the packets ok
		if(bytesin==inpacket.len){
			switch(inpacket.type)
			{
				case UNT_LOGIN_NO_AUTH:
					if(ValidState == VALID_STATE_WAITING)
					{
						ValidState = VALID_STATE_INVALID;						
					}
					break;
				case UNT_LOGIN_AUTHENTICATED:
					if(ValidState == VALID_STATE_WAITING)
					{
						ValidState = VALID_STATE_VALID;
						strncpy(Psztracker_id, (const char *)&inpacket.data, TRACKER_ID_LEN);
					}
					break;
				// old - this is a Freespace 1 packet type
				case UNT_VALID_FS_MSN_RSP:
					Int3();
					break;

				// fs2 mission validation response
				case UNT_VALID_FS2_MSN_RSP:
					if(MissionValidState == VALID_STATE_WAITING){
						if(inpacket.code==2){
							MissionValidState = VALID_STATE_VALID;
						} else {
							MissionValidState = VALID_STATE_INVALID;
						}
					}
					break;

				// fs2 squad war validation response
				case UNT_VALID_SW_MSN_RSP:
					if(SquadWarValidState == VALID_STATE_WAITING){
						// copy the data
						SDL_assert((bytesin - PACKED_HEADER_ONLY_SIZE) == sizeof(squad_war_response));
						if((bytesin - PACKED_HEADER_ONLY_SIZE) == sizeof(squad_war_response)){
							memset(&SquadWarValidateResponse, 0, sizeof(squad_war_response));
							memcpy(&SquadWarValidateResponse, inpacket.data, sizeof(squad_war_response));

							// now check to see if we're good
							if(SquadWarValidateResponse.accepted){
								SquadWarValidState = VALID_STATE_VALID;
							} else {
								SquadWarValidState = VALID_STATE_INVALID;
							}
						} else {
							SquadWarValidState = VALID_STATE_INVALID;
						}						
					}
					break;

				case UNT_CONTROL_VALIDATION:
					Int3();
					break;

				case UNT_CONTROL:
					Int3();
					break;
			}
			AckValidServer(inpacket.sig);
		}
	}

	if(ValidState == VALID_STATE_WAITING)
	{
		if((timer_get_milliseconds()-ValidFirstSent)>=PILOT_REQ_TIMEOUT)
		{
			ValidState = VALID_STATE_TIMEOUT;

		}		
		else if((timer_get_milliseconds()-ValidLastSent)>=PILOT_REQ_RESEND_TIME)
		{
			//Send 'da packet
			SENDTO(Unreliable_socket, (char *)&PacketHeader, PacketHeader.len, 0, (SOCKADDR *)&rtrackaddr, sizeof(SOCKADDR), PSNET_TYPE_VALIDATION);
			ValidLastSent = timer_get_milliseconds();
		}
	}
}


//Send an ACK to the server
void AckValidServer(unsigned int sig)
{
	udp_packet_header ack_pack;

	ack_pack.type = UNT_CONTROL;
	ack_pack.sig = sig;
	ack_pack.code = CMD_CLIENT_RECEIVED;
	ack_pack.len = PACKED_HEADER_ONLY_SIZE;
	
	SENDTO(Unreliable_socket, (char *)&ack_pack,PACKED_HEADER_ONLY_SIZE,0,(SOCKADDR *)&rtrackaddr,sizeof(SOCKADDR_IN), PSNET_TYPE_VALIDATION);
}

// call with a valid struct to validate a mission
// call with NULL to poll

// Return codes:
// -3	Still waiting (returned if we were waiting for a tracker response and ValidateMission was called with a non-NULL value
// -2 Timeout waiting for tracker to respond
// -1	User invalid
//  0	Still waiting for response from tracker/Idle
//  1	User valid
int ValidateMission(vmt_validate_mission_req_struct *valid_msn)
{	
	ValidIdle();
	if(valid_msn==NULL)
	{
		switch(MissionValidState)
		{
		case VALID_STATE_IDLE:
			return 0;
			break;
		case VALID_STATE_WAITING:
			return 0;
			break;			
		case VALID_STATE_VALID:
			MissionValidState = VALID_STATE_IDLE;
			return 1;
			break;
		case VALID_STATE_INVALID:
			MissionValidState = VALID_STATE_IDLE;
			return -1;
		case VALID_STATE_TIMEOUT:
			MissionValidState = VALID_STATE_IDLE;
			return -2;
		}
		return 0;
	}
	else
	{
		if(MissionValidState==VALID_STATE_IDLE)
		{
			//First, flush the input buffer for the socket
			fd_set read_fds;	           
			TIMEVAL timeout;   
			
			timeout.tv_sec=0;            
			timeout.tv_usec=0;
			
			FD_ZERO(&read_fds);
			FD_SET(Unreliable_socket, &read_fds);    

#ifndef PLAT_UNIX
			while(SELECT(0,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION))
#else
			while(SELECT(Unreliable_socket+1,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION))
#endif
			{
				int addrsize;
				SOCKADDR_IN fromaddr;

				udp_packet_header inpacket;
				addrsize = sizeof(SOCKADDR_IN);
				RECVFROM(Unreliable_socket, (char *)&inpacket,sizeof(udp_packet_header),0,(SOCKADDR *)&fromaddr,&addrsize, PSNET_TYPE_VALIDATION);
				FD_ZERO(&read_fds);
				FD_SET(Unreliable_socket, &read_fds);    
			}
			//only send the header, the checksum and the string length plus the null
			PacketHeader.type = UNT_VALID_FS2_MSN_REQ;
			PacketHeader.len = (short)(PACKED_HEADER_ONLY_SIZE + sizeof(int)+1+strlen(valid_msn->file_name));
			memcpy(PacketHeader.data,valid_msn,PacketHeader.len-PACKED_HEADER_ONLY_SIZE);
			SENDTO(Unreliable_socket, (char *)&PacketHeader,PacketHeader.len,0,(SOCKADDR *)&rtrackaddr,sizeof(SOCKADDR), PSNET_TYPE_VALIDATION);
			MissionValidState = VALID_STATE_WAITING;
			MissionValidFirstSent = timer_get_milliseconds();
			MissionValidLastSent = timer_get_milliseconds();
			return 0;
		}
		else
		{
			return -3;
		}
	}
}

// query the usertracker to validate a squad war match
// call with a valid struct to validate a mission
// call with NULL to poll

// Return codes:
// -3	Still waiting (returned if we were waiting for a tracker response and ValidateSquadWae was called with a non-NULL value
// -2 Timeout waiting for tracker to respond
// -1	match invalid
//  0	Still waiting for response from tracker/Idle
//  1	match valid
int ValidateSquadWar(squad_war_request *sw_req, squad_war_response *sw_resp)
{
	ValidIdle();
	if(sw_req==NULL){
		switch(SquadWarValidState){
		case VALID_STATE_IDLE:
			return 0;
			break;
		case VALID_STATE_WAITING:
			return 0;
			break;

		// fill in the response
		case VALID_STATE_VALID:
			SquadWarValidState = VALID_STATE_IDLE;
			if(sw_resp != NULL){
				memcpy(sw_resp, &SquadWarValidateResponse, sizeof(squad_war_response));
			}
			return 1;
			break;
		// fill in the response
		case VALID_STATE_INVALID:
			SquadWarValidState = VALID_STATE_IDLE;
			if(sw_resp != NULL){
				memcpy(sw_resp, &SquadWarValidateResponse, sizeof(squad_war_response));
			}
			return -1;

		case VALID_STATE_TIMEOUT:
			SquadWarValidState = VALID_STATE_IDLE;
			return -2;
		}
		return 0;
	} else {
		if(SquadWarValidState==VALID_STATE_IDLE){
			// First, flush the input buffer for the socket
			fd_set read_fds;	           
			TIMEVAL timeout;   
			
			timeout.tv_sec=0;            
			timeout.tv_usec=0;
			
			FD_ZERO(&read_fds);
			FD_SET(Unreliable_socket, &read_fds);    

#ifndef PLAT_UNIX
			while(SELECT(0,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION)){
#else
			while(SELECT(Unreliable_socket+1,&read_fds,NULL,NULL,&timeout, PSNET_TYPE_VALIDATION)){
#endif
				int addrsize;
				SOCKADDR_IN fromaddr;

				udp_packet_header inpacket;
				addrsize = sizeof(SOCKADDR_IN);
				RECVFROM(Unreliable_socket, (char *)&inpacket,sizeof(udp_packet_header),0,(SOCKADDR *)&fromaddr,&addrsize, PSNET_TYPE_VALIDATION);
				FD_ZERO(&read_fds);
				FD_SET(Unreliable_socket, &read_fds);    
			}
			// only send the header, the checksum and the string length plus the null
			PacketHeader.type = UNT_VALID_SW_MSN_REQ;
			PacketHeader.len = (short)(PACKED_HEADER_ONLY_SIZE + sizeof(squad_war_request));
			memcpy(PacketHeader.data, sw_req, PacketHeader.len-PACKED_HEADER_ONLY_SIZE);
			SENDTO(Unreliable_socket, (char *)&PacketHeader, PacketHeader.len, 0, (SOCKADDR *)&rtrackaddr, sizeof(SOCKADDR), PSNET_TYPE_VALIDATION);
			SquadWarValidState = VALID_STATE_WAITING;
			SquadWarFirstSent = timer_get_milliseconds();
			SquadWarLastSent = timer_get_milliseconds();
			return 0;
		} else {
			return -3;
		}
	}
}
