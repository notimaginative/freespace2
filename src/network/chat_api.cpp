/*
 * Copyright (C) Volition, Inc. 2005.  All rights reserved.
 * 
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/


#ifdef PLAT_UNIX
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#else
#include <winsock2.h>
// disable warnings for argument conversion in net commands
#pragma warning(disable : 4267 4244 4245)
#endif

#include "pstypes.h"
#include "chat_api.h"

#define MAXCHATBUFFER	500

static SOCKET Chatsock;
static struct sockaddr_in Chataddr;
static int Socket_connecting = 0;
static char Nick_name[33];
static char Original_nick_name[33];
static int Nick_variety = 0;
static char szChat_channel[33] = "";
static char Input_chat_buffer[MAXCHATBUFFER] = "";
static char Chat_tracker_id[33];
static char Getting_user_channel_info_for[33] = "";
static char Getting_user_tracker_info_for[33] = "";
static int Getting_user_channel_error = 0;
static int Getting_user_tracker_error = 0;
static char User_req_tracker_id[100] = ""; //These are oversized for saftey
static char User_req_channel[100] = "";
static char *User_list = NULL;
static char *Chan_list = NULL;
static int Socket_connected = 0;
static int Chat_server_connected = 0;
static int Joining_channel = 0;
static int Joined_channel = 0;
static int GettingChannelList = 0;
static int GettingUserTID = 0;
static int GettingUserChannel = 0;

static Chat_user *Firstuser,*Curruser;
static Chat_command *Firstcommand,*Currcommand;
static Chat_channel *Firstchannel,*Currchannel;

void ChatInit(void)
{
	Socket_connecting = 0;
	SDL_zero(Nick_name);
	SDL_zero(Original_nick_name);
	Nick_variety = 0;
	SDL_zero(szChat_channel);
	SDL_zero(Input_chat_buffer);
	SDL_zero(Chat_tracker_id);
	SDL_zero(Getting_user_channel_info_for);
	SDL_zero(Getting_user_tracker_info_for);
	Getting_user_channel_error = 0;
	Getting_user_tracker_error = 0;
	SDL_zero(User_req_tracker_id);
	SDL_zero(User_req_channel);
	User_list = NULL;
	Chan_list = NULL;
	Socket_connected = 0;
	Chat_server_connected = 0;
	Joining_channel = 0;
	Joined_channel = 0;
	GettingChannelList = 0;
	GettingUserTID = 0;
	GettingUserChannel = 0;

}


// Return codes:
//-2 Already connected
//-1 Failed to connect
// 0 Connecting
// 1 Connected
// Call it once with the server IP address, and it will return immediately
// with 0. Keep calling it until it returns something other than 0
// note: the nickname may be changed if someone with that name already
// exists (Scourge1 for instance)
int ConnectToChatServer(char *serveraddr,char *nickname,char *trackerid)
{
	short chat_port;
	char chat_server[50];
	char *p;
	unsigned long argp = 1;
	char signon_str[100];

	//if(Socket_connected && ) return -2;

	if(!Socket_connecting)
	{
		in_addr_t iaddr;

		SDL_strlcpy(Nick_name, nickname, SDL_arraysize(Nick_name));
		SDL_strlcpy(Original_nick_name, nickname, SDL_arraysize(Original_nick_name));
		SDL_strlcpy(Chat_tracker_id, trackerid, SDL_arraysize(Chat_tracker_id));
		
		Firstuser = NULL;
		Firstcommand = NULL;
		Chat_server_connected = 0;
		FlushChatCommandQueue();

		p = strchr(serveraddr,':');

		if(NULL==p)
		{
			//AfxMessageBox("Invalid chat server, must be host.com:port (ie. irc.dal.net:6667)");
			return -1;
		}
		SDL_strlcpy(chat_server, serveraddr, SDL_arraysize(chat_server));
		chat_server[p-serveraddr]='\0';
		chat_port = (short)atoi(p+1);
		if(0==chat_port)
		{
			//AfxMessageBox("Invalid chat port, must be host.com:port (ie. irc.dal.net:6667)");
			return -1;
		}

		Chatsock = socket(AF_INET,SOCK_STREAM,0);
		if(INVALID_SOCKET == Chatsock)
		{
			//AfxMessageBox("Unable to open socket!");
			return -1;
		}

		memset( &Chataddr, 0, sizeof(struct sockaddr_in) );
		Chataddr.sin_family = AF_INET; 
		Chataddr.sin_addr.s_addr = INADDR_ANY; 
		Chataddr.sin_port = 0;
		
		if (SOCKET_ERROR==bind(Chatsock, (struct sockaddr *)&Chataddr, sizeof (struct sockaddr)))
		{
			//AfxMessageBox("Unable to bind socket!");
			closesocket(Chatsock);
			return -1;
		}
		ioctlsocket(Chatsock,FIONBIO,&argp);
		
		// first try and resolve by name
		iaddr = inet_addr( chat_server );
		if ( iaddr == INADDR_NONE ) {	
			struct hostent *he;
			he = gethostbyname(chat_server);
			if(!he)
			{
				return 0;
				/*
				//AfxMessageBox("Unable to gethostbyname.\n");

				// try and resolve by address			
				unsigned int n_order = inet_addr(chat_server);
				he = gethostbyaddr((char*)&n_order,4,PF_INET);					

				if(!he){
					return -1;
				}
				*/
			}

			iaddr = ((in_addr *)(he->h_addr))->s_addr;
		}
		
		Chataddr.sin_addr.s_addr = iaddr;

		
		// Chataddr.sin_addr.s_addr = inet_addr(chat_server);

		Chataddr.sin_port = htons( chat_port );

		if(SOCKET_ERROR == connect(Chatsock,(struct sockaddr *)&Chataddr,sizeof(struct sockaddr_in)))
		{
			int error = WSAGetLastError();
			if ( NETCALL_WOULDBLOCK(error) )
			{
				Socket_connecting = 1;
				return 0;
			}
		}
		else
		{
			//This should never happen, connect should always return WSAEWOULDBLOCK
			Socket_connecting = 1;
			Socket_connected = 1;
			return 1;
		}
	}
	else
	{
		if(Chat_server_connected)
		{
			return 1;
		}

		if(!Socket_connected)
		{
			//Do a few select to check for an error, or to see if we are writeable (connected)
			fd_set write_fds,error_fds;	           
			struct timeval timeout;
			
			timeout.tv_sec=0;            
			timeout.tv_usec=0;
			
			FD_ZERO(&write_fds);
			FD_SET(Chatsock,&write_fds);    
			//Writable -- that means it's connected
			if(select(Chatsock+1,NULL,&write_fds,NULL,&timeout))
			{
				int error_code = 0;
				SOCKLEN_T error_code_size = sizeof(error_code);

				// check to make sure socket is *really* connected
				int rc = getsockopt(Chatsock, SOL_SOCKET, SO_ERROR, (char *)&error_code, &error_code_size);

				if(rc < 0 || error_code != 0)
				{
					shutdown(Chatsock, 2);
					closesocket(Chatsock);
					return -1;
				}

				Socket_connected = 1;
				SDL_snprintf(signon_str, SDL_arraysize(signon_str), NOX("/USER %s %s %s :%s"), NOX("user"), NOX("user"), NOX("user"), Chat_tracker_id);
				SendChatString(signon_str,1);
				SDL_snprintf(signon_str, SDL_arraysize(signon_str), NOX("/NICK %s"), Nick_name);
				SendChatString(signon_str,1);
				return 0;
				//Now we are waiting for Chat_server_connected
			}
			FD_ZERO(&error_fds);
			FD_SET(Chatsock,&error_fds);    
			//error -- that means it's not going to connect
			if(select(Chatsock+1,NULL,NULL,&error_fds,&timeout))
			{
				shutdown(Chatsock, 2);
				closesocket(Chatsock);
				return -1;
			}
			return 0;
		}
	}

	return 0;
}

// Call it to close the connection. It returns immediately
void DisconnectFromChatServer()
{
	if(!Socket_connected) return;
	SendChatString(NOX("/QUIT"),1);
	shutdown(Chatsock,2);
	closesocket(Chatsock);
	Socket_connecting = 0;
	Socket_connected = 0;
	Input_chat_buffer[0] = '\0';
	if(User_list)
	{
		free(User_list);
		User_list = NULL;
	}
	if(Chan_list)
	{
		free(Chan_list);
		Chan_list = NULL;
	}
	
	Chat_server_connected = 0;
	Joining_channel = 0;
	Joined_channel = 0;
	RemoveAllChatUsers();
	FlushChatCommandQueue();
	return;
}

// returns NULL if no line is there to print, otherwise returns a string to
// print (all preformatted of course)
char * GetChatText()
{

	if(!Socket_connected) return NULL;

	//ChatGetString will do the formatting
	return ChatGetString();

}

// Send a string to be sent as chat, or scanned for messages (/msg <user>
// string)
const char * SendChatString(const char *line,int raw)
{
	char szCmd[200];
	char szTarget[50];
	if(!Socket_connected) return NULL;
	
	if(line[0]=='/')
	{

		//Start off by getting the command
		SDL_strlcpy(szCmd, GetWordNum(0,line+1), SDL_arraysize(szCmd));
		if(SDL_strcasecmp(szCmd,NOX("msg"))==0)
		{
			SDL_strlcpy(szTarget, GetWordNum(1,line+1), SDL_arraysize(szTarget));
			SDL_snprintf(szCmd, SDL_arraysize(szCmd), NOX("PRIVMSG %s :%s\n\r"), szTarget, line+SDL_strlen(NOX("/msg "))+SDL_strlen(szTarget)+1);
			send(Chatsock,szCmd,SDL_strlen(szCmd),0);
			szCmd[SDL_strlen(szCmd)-2]='\0';
			return ParseIRCMessage(szCmd,MSG_LOCAL);

		}
		if(SDL_strcasecmp(szCmd,NOX("me"))==0)
		{
			SDL_snprintf(szCmd, SDL_arraysize(szCmd), NOX("PRIVMSG %s :\001ACTION %s\001\n\r"), szChat_channel, line+SDL_strlen(NOX("/me ")));
			send(Chatsock,szCmd,SDL_strlen(szCmd),0);
			szCmd[SDL_strlen(szCmd)-2]='\0';
			return ParseIRCMessage(szCmd,MSG_LOCAL);

		}
		if(SDL_strcasecmp(szCmd,NOX("xyz"))==0)
		{
			//Special command to send raw irc commands
			SDL_snprintf(szCmd, SDL_arraysize(szCmd), "%s\n\r", line+SDL_strlen(NOX("/xyz ")));
			send(Chatsock,szCmd,SDL_strlen(szCmd),0);
			return NULL;
		}
		if(SDL_strcasecmp(szCmd,NOX("list"))==0)
		{
			SDL_snprintf(szCmd, SDL_arraysize(szCmd), "%s\n\r", line+1);
			send(Chatsock,szCmd,SDL_strlen(szCmd),0);
			return NULL;
		}
		if(raw)
		{
			SDL_snprintf(szCmd, SDL_arraysize(szCmd), "%s\n\r", line+1);
			send(Chatsock,szCmd,SDL_strlen(szCmd),0);
			return NULL;
		}
		return XSTR("Unrecognized command",634);
		
	}
	else
	{
		if(szChat_channel[0])
		{
			/*
			CString sndstr;
			sndstr.Format("PRIVMSG %s :%s\n\r",szChat_channel,line);
			send(Chatsock,LPCSTR(sndstr),sndstr.GetLength(),0);
			sndstr = sndstr.Left(sndstr.GetLength()-2);
			return ParseIRCMessage((char *)LPCSTR(sndstr),MSG_LOCAL);
			*/

			SDL_snprintf(szCmd, SDL_arraysize(szCmd), NOX("PRIVMSG %s :%s\n\r"), szChat_channel, line);
			send(Chatsock,szCmd,SDL_strlen(szCmd),0);			
			if(SDL_strlen(szCmd) >= 2){
				szCmd[SDL_strlen(szCmd)-2] = '\0';
				return ParseIRCMessage(szCmd,MSG_LOCAL);
			} 			

			return NULL;
		}
	}
	
	return NULL;
}


// Returns a structure which contains a command and possible some data (like
// a user joining or leaving) if one is waiting
// This tells you if you need to add a user from the userlist, remove a user,
// etc. Also for status messages, like if you get knocked
// off the server for some reason.
Chat_command *GetChatCommand()
{
	if(!Socket_connected) return NULL;
	return GetChatCommandFromQueue();
}

// This function returns a list of users in the current channel, in one
// string, separated by spaces, terminated by a null
// (Spaces aren't allowed as part of a nickname)
char *GetChatUserList()
{
	int iuser_list_length = 0;;
	if(User_list)
	{
		free(User_list);
		User_list = NULL;
	}
	if(!Socket_connected) return NULL;
	
	Curruser = Firstuser;
	while(Curruser) 
	{
		iuser_list_length += SDL_strlen(Curruser->nick_name)+1;
		Curruser = Curruser->next;
	}
	Curruser = Firstuser;
	User_list = (char *)malloc(iuser_list_length+1);
	User_list[0] = '\0';
	while(Curruser) 
	{
		SDL_strlcat(User_list, Curruser->nick_name, iuser_list_length+1);
		SDL_strlcat(User_list, " ", iuser_list_length+1);
		Curruser = Curruser->next;
	}

	return User_list;
}

// Call this to set/join a channel. Since we can't be sure that we will be
// able to join that channel, check it for completion
// You can't be in more than one channel at a time with this API, so you
// leave the current channel before trying to join
// a new one. Because of this if the join fails, make sure you try to join
// another channel, or the user wont be able to chat
//-1 Failed to join
// 0 joining
// 1 successfully joined
int SetNewChatChannel(char *channel)
{
	char partstr[100];
	if(!Socket_connected) return -1;
	if(Joining_channel==1) 
	{
		if(Joined_channel==1) 
		{
			//We made it in!
			Joining_channel = 0;
			return 1;
		}
		else if(Joined_channel==-1) 
		{
			//Error -- we got a message that the channel was invite only, or we were banned or something
			Joining_channel = 0;
			SDL_zero(szChat_channel);
			return -1;
		}
	}
	else
	{
		if(szChat_channel[0])
		{
			SDL_snprintf(partstr, SDL_arraysize(partstr), NOX("/PART %s"), szChat_channel);
			SendChatString(partstr,1);
		}
		SDL_strlcpy(szChat_channel, channel, SDL_arraysize(szChat_channel));
		SDL_snprintf(partstr, SDL_arraysize(partstr), NOX("/JOIN %s"), szChat_channel);
		SendChatString(partstr,1);
		Joining_channel = 1;
		Joined_channel = 0;
	}
	
	return 0;
}


char *ChatGetString(void)
{
	fd_set read_fds;	           
	struct timeval timeout;
	char ch[2];
	char *p;
	ssize_t bytesread;
	static char return_string[MAXCHATBUFFER];
	
	timeout.tv_sec=0;            
	timeout.tv_usec=0;
	
	FD_ZERO(&read_fds);
	FD_SET(Chatsock,&read_fds);    
	//Writable -- that means it's connected
	while(select(Chatsock+1,&read_fds,NULL,NULL,&timeout))
	{
		bytesread = recv(Chatsock,ch,1,0);
		if(bytesread)
		{
			ch[1] = '\0';
			
			if((ch[0] == 0x0a)||(ch[0]==0x0d))
			{
				if(Input_chat_buffer[0]=='\0')
				{
					//Blank line, ignore it
					return NULL;
				}
				SDL_strlcpy(return_string, Input_chat_buffer, SDL_arraysize(return_string));
				Input_chat_buffer[0] = '\0';
				
				p = ParseIRCMessage(return_string,MSG_REMOTE);
				
				return p;
			}
			SDL_assert(SDL_strlen(Input_chat_buffer) < MAXCHATBUFFER-1);
			SDL_strlcat(Input_chat_buffer, ch, SDL_arraysize(Input_chat_buffer));
		}
		else
		{
			//Select said we had read data, but 0 bytes read means disconnected
			AddChatCommandToQueue(CC_DISCONNECTED,NULL,0);
			return NULL;
		}
		
	}
	return NULL;
}


const char * GetWordNum(int num, const char * l_String)
{
	static char strreturn[600];
	static char ptokstr[600];
	char seps[10] = NOX(" \n\r\t");
	char *token,*strstart;

	strstart = ptokstr;

	SDL_strlcpy(ptokstr, l_String, SDL_arraysize(ptokstr));

	token=strtok(ptokstr,seps);

	for(int i=0;i!=num;i++)
	{
		token=strtok(NULL,seps);
	}
	if(token)
	{
		SDL_strlcpy(strreturn, token, SDL_arraysize(strreturn));
	}
	else
	{
		return "";
	}
	//check for the ':' char....
	if(token[0]==':')
	{
		//Its not pretty, but it works, return the rest of the string
		SDL_strlcpy(strreturn, l_String+((token-strstart)+1), SDL_arraysize(strreturn));
	}

	//return the appropriate response.
	return strreturn;
}

int AddChatUser(const char *nickname)
{
	Curruser = Firstuser;
	while(Curruser) 
	{
		if(SDL_strcasecmp(nickname,Curruser->nick_name)==0) return 0;
		Curruser = Curruser->next;
	}

	Curruser = Firstuser;
	if(Firstuser==NULL)
	{
		Firstuser = (Chat_user *)malloc(sizeof(Chat_user));
		SDL_assert(Firstuser);
		SDL_strlcpy(Firstuser->nick_name, nickname, SDL_arraysize(Firstuser->nick_name));
		Firstuser->next = NULL;
		AddChatCommandToQueue(CC_USER_JOINING,nickname,SDL_strlen(nickname)+1);
		return 1;
	}
	else
	{
		while(Curruser->next) 
		{
			Curruser = Curruser->next;
		}
		Curruser->next = (Chat_user *)malloc(sizeof(Chat_user));
		Curruser = Curruser->next;
		SDL_assert(Curruser);
		SDL_strlcpy(Curruser->nick_name, nickname, SDL_arraysize(Curruser->nick_name));
		Curruser->next = NULL;
		AddChatCommandToQueue(CC_USER_JOINING,nickname,SDL_strlen(nickname)+1);
		return 1;
	}

}

int RemoveChatUser(char *nickname)
{
	Chat_user *prv_user = NULL;
	
	Curruser = Firstuser;
	while(Curruser) 
	{
		if(SDL_strcasecmp(nickname,Curruser->nick_name)==0)
		{
			if(prv_user)
			{
				prv_user->next = Curruser->next;

			}
			else
			{
				Firstuser = Curruser->next;
			}
			AddChatCommandToQueue(CC_USER_LEAVING,Curruser->nick_name,SDL_strlen(Curruser->nick_name)+1);
			free(Curruser);
			return 1;
		}		
		prv_user = Curruser;
		Curruser = Curruser->next;
	}
	return 0;

}

void RemoveAllChatUsers(void)
{
	Chat_user *tmp_user = NULL;
	Curruser = Firstuser;
	while(Curruser) 
	{
		tmp_user = Curruser->next;
		AddChatCommandToQueue(CC_USER_LEAVING,Curruser->nick_name,SDL_strlen(Curruser->nick_name)+1);
		free(Curruser);
		Curruser = tmp_user;
	}
	Firstuser = NULL;
}


char * ParseIRCMessage(char *Line, int iMode)
{
	char szRemLine[MAXLOCALSTRING] ="";
	const char *pszTempStr;
	char szPrefix[MAXLOCALSTRING] = "";
	char szHackPrefix[MAXLOCALSTRING] = "";
	char szTarget[MAXLOCALSTRING] = "";
	char szNick[MAXLOCALSTRING] = "";
	char szCmd[MAXLOCALSTRING] = "";
	char szCTCPCmd[MAXLOCALSTRING] = "";

	static char szResponse[MAXLOCALSTRING] = "";

	size_t iPrefixLen = 0;	// JAS: Get rid of optimized warning

	if(SDL_strlen(Line)>=MAXLOCALSTRING)
	{
		return NULL; 
	}
	//Nick included....
	if(iMode==MSG_REMOTE)
	{
		SDL_strlcpy(szRemLine, Line, SDL_arraysize(szRemLine));
		//Start by getting the prefix
		if(Line[0]==':')
		{
			//
			pszTempStr=GetWordNum(0,Line+1);
			SDL_strlcpy(szPrefix, pszTempStr, SDL_arraysize(szPrefix));
			SDL_strlcpy(szHackPrefix, pszTempStr, SDL_arraysize(szHackPrefix));
			SDL_strlcpy(szRemLine, Line+1+SDL_strlen(szPrefix), SDL_arraysize(szRemLine));
		}
		//Next, get the Nick
		pszTempStr=strtok(szHackPrefix,"!");
		if(pszTempStr)
		{
			SDL_strlcpy(szNick, pszTempStr, SDL_arraysize(szNick));
		}
		else
		{
			SDL_strlcpy(szNick, szPrefix, SDL_arraysize(szNick));
		}
		//strcpy(NewMsg.Nickname,szNick);
		iPrefixLen=SDL_strlen(szPrefix);
	}
	else if(iMode==MSG_LOCAL)
	{
		SDL_strlcpy(szRemLine, Line, SDL_arraysize(szRemLine));
		SDL_strlcpy(szNick, Nick_name, SDL_arraysize(szNick));
		SDL_strlcpy(szPrefix, Nick_name, SDL_arraysize(szPrefix));
		//strcpy(NewMsg.Nickname,szNick);
		iPrefixLen=-2;
	}
	//Next is the command
	pszTempStr=GetWordNum(0,szRemLine);
	if(pszTempStr[0])
	{
		SDL_strlcpy(szCmd, pszTempStr, SDL_arraysize(szCmd));
	}
	else
	{
		//Shouldn't ever happen, but we can't be sure of what the host will send us.
		return NULL;
	}

	//Move the szRemLine string up
	SDL_strlcpy(szRemLine, Line+iPrefixLen+SDL_strlen(szCmd)+2, SDL_arraysize(szRemLine));
	//Now parse the commands!
	//printf("%s",szCmd);
	if(SDL_strcasecmp(szCmd,NOX("PRIVMSG"))==0)
	{
		pszTempStr=GetWordNum(0,szRemLine);
		SDL_strlcpy(szTarget, pszTempStr, SDL_arraysize(szTarget));
		SDL_strlcpy(szRemLine, Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+4, SDL_arraysize(szRemLine));
		if(szRemLine[0]==':')
		{
			SDL_strlcpy(szCTCPCmd, GetWordNum(0,szRemLine+1), SDL_arraysize(szCTCPCmd));
			if(szCTCPCmd[SDL_strlen(szCTCPCmd)-1]==0x01) szCTCPCmd[SDL_strlen(szCTCPCmd)-1]=0x00;

		}
		else
		{
			SDL_strlcpy(szCTCPCmd, GetWordNum(0,szRemLine), SDL_arraysize(szCTCPCmd));
			if(szCTCPCmd[SDL_strlen(szCTCPCmd)-1]==0x01) szCTCPCmd[SDL_strlen(szCTCPCmd)-1]=0x00;
		}
		if(szCTCPCmd[0]==0x01)
		{
			//Handle ctcp message
			SDL_strlcpy(szRemLine, Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+SDL_strlen(szCTCPCmd)+6, SDL_arraysize(szRemLine));
			szRemLine[SDL_strlen(szRemLine)-1]='\0';//null out the ending 0x01
			if(SDL_strcasecmp(szCTCPCmd+1,NOX("ACTION"))==0)
			{
				//Posture
				SDL_snprintf(szResponse, SDL_arraysize(szResponse), "* %s %s", szNick, szRemLine);
				return szResponse;
			}
			if(iMode==MSG_LOCAL)
			{
				SDL_strlcpy(szHackPrefix, Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+4, SDL_arraysize(szHackPrefix));
				szRemLine[SDL_strlen(szRemLine)-1]='\0';
				SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("** CTCP %s %s %s"), szTarget, szCTCPCmd+1, szRemLine);
				return szResponse;
			}
			if(SDL_strcasecmp(szCTCPCmd+1,NOX("PING"))==0)
			{
				SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("/NOTICE %s :\001PING %s\001"), szNick, szRemLine);//Don't need the trailing \001 because szremline has it.
				SendChatString(szResponse,1);
				return NULL;
			}
			if(SDL_strcasecmp(szCTCPCmd+1,NOX("VERSION"))==0)
			{
				//reply with a notice version & copyright
				//sprintf(szTempLine,"NOTICE %s :\001VERSION Copyright(c)\001\n",szNick);

				return NULL;
			}
			SDL_strlcpy(szRemLine, 1 + GetWordNum(0,Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+4), SDL_arraysize(szRemLine));
			szRemLine[SDL_strlen(szRemLine)-1]='\0';
			SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("** CTCP Message from %s (%s)"), szNick, szRemLine);
			return szResponse;

		}
		//differentiate between channel and private
		if(szTarget[0]=='#')
		{
			pszTempStr=GetWordNum(0,szRemLine);
			SDL_snprintf(szResponse, SDL_arraysize(szResponse), "[%s] %s", szNick, pszTempStr);
			return szResponse;
		}
		else
		{
			if(iMode == MSG_LOCAL)
			{
				pszTempStr=GetWordNum(0,szRemLine);
				SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("Private Message to <%s>: %s"), szNick, pszTempStr);
			}
			else
			{
				pszTempStr=GetWordNum(0,szRemLine);
				SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("Private Message from <%s>: %s"), szNick, pszTempStr);
			}
			return szResponse;
		}

	}
	//don't handle any other messages locally.
	if(iMode==MSG_LOCAL)
	{
		return NULL;
	}

	if(SDL_strcasecmp(szCmd,NOX("NOTICE"))==0)
	{
		

		pszTempStr=GetWordNum(0,szRemLine);
		SDL_strlcpy(szTarget, pszTempStr, SDL_arraysize(szTarget));
		SDL_strlcpy(szRemLine, Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+4, SDL_arraysize(szRemLine));
		if(szRemLine[0]==':')
		{
			SDL_strlcpy(szCTCPCmd, GetWordNum(0,szRemLine+1), SDL_arraysize(szCTCPCmd));
			if(szCTCPCmd[SDL_strlen(szCTCPCmd)-1]==0x01) szCTCPCmd[SDL_strlen(szCTCPCmd)-1]=0x00;

		}
		else
		{
			SDL_strlcpy(szCTCPCmd, GetWordNum(0,szRemLine), SDL_arraysize(szCTCPCmd));
			if(szCTCPCmd[SDL_strlen(szCTCPCmd)-1]==0x01) szCTCPCmd[SDL_strlen(szCTCPCmd)-1]=0x00;
		}
		if(szCTCPCmd[0]==0x01)
		{
			//Handle ctcp message
			SDL_strlcpy(szRemLine, Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+SDL_strlen(szCTCPCmd)+6, SDL_arraysize(szRemLine));
			szRemLine[SDL_strlen(szRemLine)-1]='\0';//null out the ending 0x01
			if(SDL_strcasecmp(szCTCPCmd+1,NOX("PING"))==0)
			{
				//This is a ping response, figure out time and print
				//sprintf(NewMsg.Message,"** Ping Response from %s: %ums",szNick,ulping);
				return NULL;
			}
			
			//Default message
			SDL_strlcpy(szRemLine, 1 + GetWordNum(0,Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+4), SDL_arraysize(szRemLine));
			szRemLine[SDL_strlen(szRemLine)-1]='\0';
			SDL_snprintf(szResponse, SDL_arraysize(szResponse), XSTR("** CTCP Message from %s (%s)",635), szNick, szRemLine);
			return szResponse;
			
		}
		SDL_snprintf(szResponse, SDL_arraysize(szResponse), "%s", szRemLine);
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,NOX("JOIN"))==0)
	{
		//see if it is me!
		if(SDL_strcasecmp(Nick_name,szNick)==0)
		{
			//Yup, it's me!
			//if(strcmpi(szChat_channel,GetWordNum(0,szRemLine))==0)
			//{
				Joined_channel = 1;
				if(SDL_strcasecmp(szChat_channel,NOX("#autoselect"))==0)
				{
					SDL_strlcpy(szChat_channel, GetWordNum(0,szRemLine), SDL_arraysize(szChat_channel));
					AddChatCommandToQueue(CC_YOURCHANNEL,szChat_channel,SDL_strlen(szChat_channel)+1);

				}
				//CC_YOURCHANNEL
			//}
		}
				AddChatUser(szNick);

		
		pszTempStr=GetWordNum(0,szRemLine);
		SDL_strlcpy(szTarget, pszTempStr, SDL_arraysize(szTarget));
		//strcpy(szRemLine,Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+3);

		//strcpy(NewMsg.Channel,szTarget);

		AddChatUser(szNick);
		SDL_snprintf(szResponse, SDL_arraysize(szResponse), XSTR("** %s has joined %s",636), szNick, szTarget);
		return NULL;//szResponse;
		//Add them to the userlist too!
	}
	if(SDL_strcasecmp(szCmd,NOX("PART"))==0)
	{
		pszTempStr=GetWordNum(0,szRemLine);
		SDL_strlcpy(szTarget, pszTempStr, SDL_arraysize(szTarget));
		SDL_strlcpy(szRemLine, Line+iPrefixLen+SDL_strlen(szCmd)+SDL_strlen(szTarget)+3, SDL_arraysize(szRemLine));
		//see if it is me!
		if(SDL_strcasecmp(Nick_name,szNick)==0)
		{
			//Yup, it's me!
			//szChat_channel[0]=NULL;
			RemoveAllChatUsers();
		}
		
		RemoveChatUser(szNick);
		return NULL;
		//Remove them to the userlist too!
	}
	if(SDL_strcasecmp(szCmd,NOX("KICK"))==0)
	{
		pszTempStr=GetWordNum(0,szRemLine);
		SDL_strlcpy(szTarget, pszTempStr, SDL_arraysize(szTarget));
		pszTempStr=GetWordNum(1,szRemLine);
		SDL_strlcpy(szHackPrefix, pszTempStr, SDL_arraysize(szHackPrefix));
		pszTempStr=GetWordNum(2,szRemLine);
		//see if it is me!
		if(SDL_strcasecmp(Nick_name,GetWordNum(1,szRemLine))==0)
		{
			//Yup, it's me!
			szChat_channel[0]='\0';
			//bNewStatus=1;
			AddChatCommandToQueue(CC_KICKED,NULL,0);			
			RemoveAllChatUsers();
		}
		SDL_snprintf(szResponse, SDL_arraysize(szResponse), XSTR("*** %s has kicked %s from channel %s (%s)",637), szNick, szHackPrefix, szTarget, pszTempStr);
		//Remove them to the userlist too!
		RemoveChatUser(szNick);
		return szResponse;
		
	}
	if(SDL_strcasecmp(szCmd,NOX("NICK"))==0)
	{
      //see if it is me!
		if(SDL_strcasecmp(Nick_name,szNick)==0)
		{
			//Yup, it's me!
			SDL_strlcpy(Nick_name, GetWordNum(0,szRemLine), SDL_arraysize(Nick_name));
		}
		char nicks[70];
		SDL_snprintf(nicks, SDL_arraysize(nicks), "%s %s", szNick, GetWordNum(0,szRemLine));
		AddChatCommandToQueue(CC_NICKCHANGED,nicks,SDL_strlen(nicks)+1);
		RemoveChatUser(szNick);
		AddChatUser(GetWordNum(0,szRemLine));
	  SDL_snprintf(szResponse, SDL_arraysize(szResponse), XSTR("*** %s is now known as %s",638), szNick, GetWordNum(0,szRemLine));
		return szResponse;
	}
	if(SDL_strcasecmp(szCmd,NOX("PING"))==0)
	{
		//respond with pong (GetWordNum(0,szRemLine))
		SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("/PONG :%s"), GetWordNum(0,szRemLine));
		SendChatString(szResponse,1);
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,NOX("MODE"))==0)
	{
		//Channel Mode info
		return NULL;
	}


	if(SDL_strcasecmp(szCmd,"401")==0)
	{
		//This is whois user info, we can get their tracker info from here.  -5
		char szWhoisUser[33];
		SDL_strlcpy(szWhoisUser, GetWordNum(1,szRemLine), SDL_arraysize(szWhoisUser));
		Getting_user_tracker_error = 1;			
		Getting_user_channel_error = 1;				
						
		SDL_snprintf(szResponse, SDL_arraysize(szResponse), XSTR("**Error: %s is not online!",639), szWhoisUser);
		return szResponse;

	}
	if(SDL_strcasecmp(szCmd,"311")==0)
	{
		char szWhoisUser[33];
		SDL_strlcpy(szWhoisUser, GetWordNum(1,szRemLine), SDL_arraysize(szWhoisUser));
		//This is whois user info, we can get their tracker info from here.  -5
		//if(strcmpi(Getting_user_tracker_info_for,szWhoisUser)==0)
		//{
			SDL_strlcpy(User_req_tracker_id, GetWordNum(5,szRemLine), SDL_arraysize(User_req_tracker_id));
		//}
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,"319")==0)
	{
		char szWhoisUser[33];
		SDL_strlcpy(szWhoisUser, GetWordNum(1,szRemLine), SDL_arraysize(szWhoisUser));
		//This is whois channel info -- what channel they are on		-2
		//if(strcmpi(Getting_user_channel_info_for,szWhoisUser)==0)
		//{
			SDL_strlcpy(User_req_channel, GetWordNum(2,szRemLine), SDL_arraysize(User_req_channel));
		//}
		return NULL;
	}
	
	//End of whois and we didn't get a channel means they aren't in a channel.
	if(SDL_strcasecmp(szCmd,"318")==0)
	{
		if(!*User_req_channel)
		{
			User_req_channel[0] = '*';
		}
	}


	if(SDL_strcasecmp(szCmd,"321")==0)
	{
		//start of channel list
		FlushChannelList();
		GettingChannelList = 1;
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,"322")==0)
	{
		//channel list data
		if(GettingChannelList == 1)
		{
			char channel_list_name[33];
			char sztopic[200];
			SDL_strlcpy(sztopic, GetWordNum(3,szRemLine), SDL_arraysize(sztopic));
			SDL_strlcpy(channel_list_name, GetWordNum(1,szRemLine), SDL_arraysize(channel_list_name));
			AddChannel(channel_list_name,(short)atoi(GetWordNum(2,szRemLine)),sztopic);
		}
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,"323")==0)
	{
		//end of channel list
		GettingChannelList = 2;
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,"324")==0)
	{
		//Channel Mode info
		return NULL;
	}

	if(SDL_strcasecmp(szCmd,"332")==0)
	   	{
		//Channel Topic, update status bar.
		if(SDL_strcasecmp(szChat_channel,szTarget)==0)
		{
			//strncpy(szChanTopic,GetWordNum(2,szRemLine),70);
		}
		//sprintf(NewMsg.Message,"*** %s has changed the topic to: %s",szNick,GetWordNum(2,szRemLine));

		return NULL;
	}
	if(SDL_strcasecmp(szCmd,NOX("TOPIC"))==0)
	{
		//Channel Topic, update status bar.
		if(SDL_strcasecmp(szChat_channel,szTarget)==0)
		{
			//strncpy(szChanTopic,GetWordNum(1,szRemLine),70);
		}
		//sprintf(NewMsg.Message,"*** %s has changed the topic to: %s",szNick,GetWordNum(1,szRemLine));
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,NOX("QUIT"))==0)
	{
		//Remove the user!
		RemoveChatUser(szNick);
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,"376")==0) //end of motd, trigger autojoin...
	{
		if (!Chat_server_connected)
		{
			Chat_server_connected=1;
		}

		// end of motd
		SDL_strlcpy(szResponse, PXO_CHAT_END_OF_MOTD_PREFIX, SDL_arraysize(szResponse));
		return szResponse;
	}
	if((SDL_strcasecmp(szCmd,"377")==0)||
		(SDL_strcasecmp(szCmd,"372")==0)||
		(SDL_strcasecmp(szCmd,"372")==0)
		
		)
	{
		//Stip the message, and display it.
		pszTempStr=GetWordNum(3,Line);		
		SDL_strlcpy(szResponse, PXO_CHAT_MOTD_PREFIX, SDL_arraysize(szResponse));
		SDL_strlcat(szResponse, pszTempStr, SDL_arraysize(szResponse));
		return szResponse;
	}
	//Ignore these messages
	if(((SDL_strcasecmp(szCmd,"366")==0))||
		(SDL_strcasecmp(szCmd,"333")==0) || //Who set the topic
		 (SDL_strcasecmp(szCmd,"329")==0))    //Time Channel created
		 /*
		 (SDL_strcasecmp(szCmd,"305")==0) ||
		 (SDL_strcasecmp(szCmd,"306")==0) ||
		 (SDL_strcasecmp(szCmd,"311")==0) || //WHOIS stuff
		 (SDL_strcasecmp(szCmd,"312")==0) ||
		 (SDL_strcasecmp(szCmd,"313")==0) ||
		 (SDL_strcasecmp(szCmd,"317")==0) ||
		 (SDL_strcasecmp(szCmd,"318")==0) ||
		 (SDL_strcasecmp(szCmd,"319")==0) ||
		 */

	{
		return NULL;
	}
	if(SDL_strcasecmp(szCmd,"353")==0)
	{

		//Names in the channel.
		pszTempStr = GetWordNum(3,Line+iPrefixLen+SDL_strlen(szCmd)+2);
		SDL_strlcpy(szRemLine, pszTempStr, SDL_arraysize(szRemLine));
		pszTempStr = strtok(szRemLine," ");

		while(pszTempStr)
		{
			if(pszTempStr[0]=='@')
			{
				AddChatUser(pszTempStr+1);
			}
			else if(pszTempStr[0]=='+')
			{
				AddChatUser(pszTempStr+1);
			}
			else
			{
				AddChatUser(pszTempStr);
			}
			pszTempStr=strtok(NULL," ");
		}
		return NULL;
	}
	//MOTD Codes
	if((SDL_strcasecmp(szCmd,"001")==0)||
	   (SDL_strcasecmp(szCmd,"002")==0)||
	   (SDL_strcasecmp(szCmd,"003")==0)||
	   (SDL_strcasecmp(szCmd,"004")==0)||
	   (SDL_strcasecmp(szCmd,"251")==0)||
	   (SDL_strcasecmp(szCmd,"254")==0)||
	   (SDL_strcasecmp(szCmd,"255")==0)||
	   (SDL_strcasecmp(szCmd,"265")==0)||
	   (SDL_strcasecmp(szCmd,"375")==0)||
	   (SDL_strcasecmp(szCmd,"372")==0)||
	   (SDL_strcasecmp(szCmd,"375")==0)
	   )
	{
		// Stip the message, and display it.
		// pszTempStr = GetWordNum(3, Line);
		// strcpy(szResponse, PXO_CHAT_MOTD_PREFIX);
		// strcat(szResponse, pszTempStr);
		return NULL;
		// return szResponse;
	}
	if(SDL_strcasecmp(szCmd,"432")==0)
	{
		//Channel Mode info
		SDL_strlcpy(szResponse, XSTR("Your nickname contains invalid characters",640), SDL_arraysize(szResponse));
		AddChatCommandToQueue(CC_DISCONNECTED,NULL,0);
		return szResponse;
	}
	if(SDL_strcasecmp(szCmd,"433")==0)
	{
		//Channel Mode info
		char new_nick[33];
		SDL_snprintf(new_nick, SDL_arraysize(new_nick), "%s%d", Original_nick_name, Nick_variety);
		SDL_strlcpy(Nick_name, new_nick, SDL_arraysize(Nick_name));
		Nick_variety++;
		SDL_snprintf(szResponse, SDL_arraysize(szResponse), NOX("/NICK %s"), new_nick);
		SendChatString(szResponse,1);
		return NULL;
	}
	//Default print
	SDL_strlcpy(szResponse, Line, SDL_arraysize(szResponse));
	//return szResponse;
	return NULL;

}


void AddChatCommandToQueue(int command,const void *data,size_t len)
{
	Currcommand = Firstcommand;
	if(Firstcommand==NULL)
	{
		Firstcommand = (Chat_command *)malloc(sizeof(Chat_command));
		SDL_assert(Firstcommand);
		Firstcommand->next = NULL;
		Currcommand = Firstcommand;
	}
	else
	{
		while(Currcommand->next) 
		{
			Currcommand = Currcommand->next;
		}
		Currcommand->next = (Chat_command *)malloc(sizeof(Chat_command));
		SDL_assert(Currcommand->next);
		Currcommand = Currcommand->next;
	}
	Currcommand->command = (short)command;
	if(len&&data) memcpy(&Currcommand->data,data,len);
	Currcommand->next = NULL;
	return;
}

Chat_command *GetChatCommandFromQueue(void)
{
	static Chat_command response_cmd;
	Chat_command *tmp_cmd;
	if(!Firstcommand) return NULL;
	Currcommand = Firstcommand;
	memcpy(&response_cmd,Currcommand,sizeof(Chat_command));
	tmp_cmd = Currcommand->next;
	free(Firstcommand);
	Firstcommand = tmp_cmd;
	return &response_cmd;
}

void FlushChatCommandQueue(void)
{
	Chat_command *tmp_cmd;
	Currcommand = Firstcommand;
	
	while(Currcommand) 
	{
		tmp_cmd = Currcommand->next;
		free(Currcommand);
		Currcommand = tmp_cmd;
	}
	Firstcommand = NULL;
}


void FlushChannelList(void)
{
	Chat_channel *tmp_chan;
	Currchannel = Firstchannel;
	
	while(Currchannel) 
	{
		tmp_chan = Currchannel->next;
		free(Currchannel);
		Currchannel = tmp_chan;
	}
	Firstchannel = NULL;


}
char *GetChannelList(void)
{
	int ichan_list_length = 0;
	char sznumusers[10];
	
	if(GettingChannelList != 2) return NULL;
	if(!Socket_connected) return NULL;

	if(Chan_list)
	{
		free(Chan_list);
		Chan_list = NULL;
	}
	
	
	Currchannel = Firstchannel;
	while(Currchannel) 
	{
		ichan_list_length += SDL_strlen(Currchannel->topic)+1+SDL_strlen(Currchannel->channel_name)+1+5;//1 for the space, and 4 for the number of users 0000-9999 + space
		Currchannel = Currchannel->next;
	}
	Currchannel = Firstchannel;
	Chan_list = (char *)malloc(ichan_list_length+1);
	Chan_list[0] = '\0';
	while(Currchannel) 
	{
		SDL_strlcat(Chan_list, "$", ichan_list_length+1);
		SDL_strlcat(Chan_list, Currchannel->channel_name, ichan_list_length+1);
		SDL_strlcat(Chan_list, " ", ichan_list_length+1);
		SDL_snprintf(sznumusers, SDL_arraysize(sznumusers), "%d ", Currchannel->users);
		SDL_strlcat(Chan_list, sznumusers, ichan_list_length+1);
		SDL_strlcat(Chan_list, Currchannel->topic, ichan_list_length+1);//fgets
		SDL_strlcat(Chan_list, " ", ichan_list_length+1);
		Currchannel = Currchannel->next;
	}
	FlushChannelList();
	GettingChannelList = 0;
	return Chan_list;
}

void AddChannel(char *channel,unsigned short numusers,char *topic)
{
	Currchannel = Firstchannel;
	if(Firstchannel==NULL)
	{
		Firstchannel = (Chat_channel *)malloc(sizeof(Chat_channel));
		SDL_assert(Firstchannel);
		SDL_strlcpy(Firstchannel->channel_name, channel, SDL_arraysize(Firstchannel->channel_name));
		SDL_strlcpy(Firstchannel->topic, topic, SDL_arraysize(Firstchannel->topic));
		Firstchannel->users = numusers;
		Firstchannel->next = NULL;
		Currchannel = Firstchannel;
	}
	else
	{
		while(Currchannel->next) 
		{
			Currchannel = Currchannel->next;
		}
		Currchannel->next = (Chat_channel *)malloc(sizeof(Chat_channel));
		SDL_assert(Currchannel->next);
		Currchannel = Currchannel->next;
		SDL_strlcpy(Currchannel->channel_name, channel, SDL_arraysize(Currchannel->channel_name));
		SDL_strlcpy(Currchannel->topic, topic, SDL_arraysize(Currchannel->topic));
		Currchannel->users = numusers;
	}
	Currchannel->next = NULL;
	return;
}


char *GetTrackerIdByUser(char *nickname)
{
	char szWhoisCmd[100];

	
	if(GettingUserTID)
	{
		if(Getting_user_tracker_error)
		{
			Getting_user_tracker_error = 0;
			GettingUserTID = 0;
			return (char *)-1;
		}
		
		if(*User_req_tracker_id)
		{
			GettingUserTID = 0;
			return User_req_tracker_id;
		}
	}
	else
	{
		SDL_strlcpy(Getting_user_tracker_info_for, nickname, SDL_arraysize(Getting_user_tracker_info_for));
		SDL_snprintf(szWhoisCmd, SDL_arraysize(szWhoisCmd), NOX("/WHOIS %s"), nickname);
		User_req_tracker_id[0] = '\0';
		SendChatString(szWhoisCmd,1);		
		GettingUserTID = 1;
	}
	return NULL;
}

char *GetChannelByUser(char *nickname)
{
	char szWhoisCmd[100];
	
	if(GettingUserChannel)
	{
		if(Getting_user_channel_error)
		{
			Getting_user_channel_error = 0;
			GettingUserChannel = 0;
			return (char *)-1;
		}
		if(*User_req_channel)
		{
			GettingUserChannel = 0;
			return User_req_channel;
		}
	}
	else
	{
		SDL_strlcpy(Getting_user_channel_info_for, nickname, SDL_arraysize(Getting_user_channel_info_for));
		User_req_channel[0] = '\0';
		SDL_snprintf(szWhoisCmd, SDL_arraysize(szWhoisCmd), NOX("/WHOIS %s"), nickname);
		SendChatString(szWhoisCmd,1);
		GettingUserChannel = 1;
	}
	return NULL;
}

