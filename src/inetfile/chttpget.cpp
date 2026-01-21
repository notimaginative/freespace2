/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
* $Logfile: /Freespace2/code/Inetfile/Chttpget.cpp $
* $Revision$
* $Date$
* $Author$
*
* HTTP Client class (get only)
*
* $Log$
* Revision 1.8  2002/06/21 03:04:12  relnev
* nothing important
*
* Revision 1.7  2002/06/17 06:33:09  relnev
* ryan's struct patch for gcc 2.95
*
* Revision 1.6  2002/06/09 04:41:21  relnev
* added copyright header
*
* Revision 1.5  2002/06/02 04:26:34  relnev
* warning cleanup
*
* Revision 1.4  2002/05/26 20:32:24  theoddone33
* Fix some minor stuff
*
* Revision 1.3  2002/05/26 20:20:54  relnev
* unix.h: updated
*
* inetfile/: complete
*
* Revision 1.2  2002/05/07 03:16:45  theoddone33
* The Great Newline Fix
*
* Revision 1.1.1.1  2002/05/03 03:28:09  root
* Initial import.
*
 * 
 * 5     8/24/99 1:49a Dave
 * Fixed client-side afterburner stuttering. Added checkbox for no version
 * checking on PXO join. Made button info passing more friendly between
 * client and server.
 * 
 * 4     8/22/99 1:19p Dave
 * Fixed up http proxy code. Cleaned up scoring code. Reverse the order in
 * which d3d cards are detected.
 * 
 * 21    8/21/99 6:33p Kevin
 * Fixed Proxy Stuff
 * 
 * 20    8/21/99 6:48a Jeff
 * Linux port
 * 
 * 19    8/20/99 3:01p Kevin
 * Added support for Proxies (I hope!)
 * 
 * 18    8/15/99 6:38p Jeff
 * fixed compile error
 * 
 * 17    8/15/99 6:26p Kevin
 * 
 * 16    4/14/99 1:20a Jeff
 * fixed case mismatched #includes
 * 
 * 15    3/03/99 12:28a Nate
 * sped up something or other when the connection is done
 * 
 * 14    2/03/99 4:20p Kevin
 * Got multiplayer working with .mn3 files, and setup autodownloading
 * 
 * 13    1/27/99 5:49p Kevin
 * 
 * 12    1/27/99 5:38p Kevin
 * 
 * 11    12/30/98 12:15p Kevin
 * Auto Mission Download system
 * 
 * 10    10/12/98 4:59p Kevin
 * Added delay to thread when cancelled...
 * 
 * 9     10/12/98 4:49p Nate
 * More fixes
 * 
 * 8     10/12/98 1:54p Nate
 * Fixed bug
 * 
 * 7     10/12/98 11:30a Kevin
 * More memory stuff
 * 
 * 6     10/08/98 12:59p Nate
 * fixed cancel
 * 
 * 5     10/08/98 9:57a Kevin
 * made transfer cancellable
 * 
 * 4     7/31/98 12:19p Nate
 * Fixed http abort problem.
 * 
 * 3     7/31/98 11:57a Kevin
 * Added new functions for getting state
 * 
 * 2     6/01/98 10:10a Kevin
 * Added DLL connection interface and auto update DLL
 * 
 * 1     5/27/98 9:52a Kevin
 * 
 * 1     5/25/98 5:31p Kevin
 * Initial version
*
* $NoKeywords: $
*/


#ifdef SDL_PLATFORM_WINDOWS
#include <winsock2.h>
// disable warnings for argument conversion in net commands
#pragma warning(disable : 4267 4244)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "pstypes.h"
#include "inetgetfile.h"
#include "chttpget.h"



#define NW_AGHBN_CANCEL		1
#define NW_AGHBN_LOOKUP		2
#define NW_AGHBN_READ		3

int http_gethostbynameworker(void *parm);


int http_Asyncgethostbyname(in_addr_t *ip, int command, char *hostname);

int HTTPObjThread( void * obj )
{
	((ChttpGet *)obj)->WorkerThread();
	((ChttpGet *)obj)->m_Aborted = true;

	return ((ChttpGet *)obj)->GetStatus();
}

void ChttpGet::AbortGet()
{
	m_Aborting = true;
	while(!m_Aborted) SDL_Delay(10); //Wait for the thread to end
}

ChttpGet::ChttpGet(char *URL,char *localfile,char *proxyip,unsigned short proxyport)
{
	m_ProxyEnabled = true;
	m_ProxyIP = proxyip;
	m_ProxyPort = proxyport;
	GetFile(URL,localfile);
}

ChttpGet::ChttpGet(char *URL,char *localfile)
{
	m_ProxyEnabled = false;
	GetFile(URL,localfile);
}


void ChttpGet::GetFile(char *URL,char *localfile)
{
	m_DataSock = INVALID_SOCKET;
	m_iBytesIn = 0;
	m_iBytesTotal = 0;
	m_State = HTTP_STATE_STARTUP;;
	m_Aborting = false;
	m_Aborted = false;

	SDL_strlcpy(m_URL, URL, SDL_arraysize(m_URL));

	LOCALFILE = fopen(localfile,"wb");
	if(NULL == LOCALFILE)
	{
		m_State = HTTP_STATE_CANT_WRITE_FILE;
		m_Aborted = true;
		return;
	}
	m_DataSock = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == m_DataSock)
	{
		m_State = HTTP_STATE_SOCKET_ERROR;
		m_Aborted = true;
		return;
	}
	unsigned long arg;

	arg = 1;

	ioctlsocket( m_DataSock, FIONBIO, &arg );

	char *pURL = URL;
	if(SDL_strncasecmp(URL,"http:",5)==0)
	{
		pURL +=5;
		while(*pURL == '/')
		{
			pURL++;
		}
	}
	//There shouldn't be any : in this string
	if(SDL_strchr(pURL,':'))
	{
		m_State = HTTP_STATE_URL_PARSING_ERROR;
		m_Aborted = true;
		return;
	}
	//read the filename by searching backwards for a /
	//then keep reading until you find the first /
	//when you found it, you have the host and dir
	char *filestart = NULL;
	char *dirstart = NULL;
	for(int i = static_cast<int>(SDL_strlen(pURL));i>=0;i--)
	{
		if(pURL[i]== '/')
		{
			if(!filestart)
			{
				filestart = pURL+i+1;
				dirstart = pURL+i+1;
				SDL_strlcpy(m_szFilename, filestart, SDL_arraysize(m_szFilename));
			}
			else
			{
				dirstart = pURL+i+1;
			}
		}
	}
	if((dirstart==NULL) || (filestart==NULL))
	{
		m_State = HTTP_STATE_URL_PARSING_ERROR;
		m_Aborted = true;
		return;
	}
	else
	{
		SDL_strlcpy(m_szDir, dirstart, SDL_arraysize(m_szDir));//,(filestart-dirstart));
		auto len = SDL_min((dirstart-pURL), (int)SDL_arraysize(m_szHost));
		SDL_strlcpy(m_szHost, pURL, len);
	}

	SDL_Thread *thread = SDL_CreateThread(HTTPObjThread, "HTTPObjThread", this);

	if(thread == NULL)
	{
		m_State = HTTP_STATE_INTERNAL_ERROR;
		m_Aborted = true;
	}
	else
	{
		SDL_DetachThread(thread);
	}
}


ChttpGet::~ChttpGet()
{
	if(m_DataSock != INVALID_SOCKET)
	{
		shutdown(m_DataSock,2);
		closesocket(m_DataSock);
	}

	if(LOCALFILE != NULL)
	{
		fclose(LOCALFILE);
	}
}

int ChttpGet::GetStatus()
{
	return m_State;
}

unsigned int ChttpGet::GetBytesIn()
{
	return m_iBytesIn;
}

unsigned int ChttpGet::GetTotalBytes()
{
	return m_iBytesTotal;
}


void ChttpGet::WorkerThread()
{
	char szCommand[1000];
	char *p;
	int irsp = 0;
	ConnectSocket();
	if(m_Aborting)
	{
		fclose(LOCALFILE);
		LOCALFILE = NULL;
		return;
	}
	if(m_State != HTTP_STATE_CONNECTED)
	{
		fclose(LOCALFILE);
		LOCALFILE = NULL;
		return;
	}
	SDL_snprintf(szCommand,SDL_arraysize(szCommand),"GET %s%s HTTP/1.1\nAccept: */*\nAccept-Encoding: deflate\nHost: %s\n\n\n",m_ProxyEnabled?"":"/",m_ProxyEnabled?m_URL:m_szDir,m_szHost);
	send(m_DataSock,szCommand,SDL_strlen(szCommand),0);
	p = GetHTTPLine();
	if(SDL_strncasecmp("HTTP/",p,5)==0)
	{
		char *pcode;
		pcode = SDL_strchr(p,' ')+1;
		if(!pcode)
		{
			m_State = HTTP_STATE_UNKNOWN_ERROR;	
			fclose(LOCALFILE);
			LOCALFILE = NULL;
			return;

		}
		pcode[3] = '\0';
		irsp = atoi(pcode);

		if(irsp == 0)
		{
			m_State = HTTP_STATE_UNKNOWN_ERROR;	
			fclose(LOCALFILE);
			LOCALFILE = NULL;
			return;
		}
		if(irsp==200)
		{
			do
			{
				p = GetHTTPLine();
				if(p==NULL)
				{
					m_State = HTTP_STATE_UNKNOWN_ERROR;	
					fclose(LOCALFILE);
					LOCALFILE = NULL;
					return;
				}
				if(*p=='\0')
				{
					break;
				}
				if(SDL_strncasecmp(p,"Content-Length:",SDL_strlen("Content-Length:"))==0)
				{
					char *s = SDL_strchr(p,' ')+1;
					p = s;
					if(s)
					{
						while(*s)
						{
							if(!isdigit(*s))
							{
								*s='\0';
							}
							s++;
						};
						m_iBytesTotal = atoi(p);
					}

				}

				SDL_Delay(1);
			}while(true);
		ReadDataChannel();
		return;
		}
		m_State = HTTP_STATE_FILE_NOT_FOUND;
		fclose(LOCALFILE);
		LOCALFILE = NULL;
		return;
	}
	else
	{
		m_State = HTTP_STATE_UNKNOWN_ERROR;
		fclose(LOCALFILE);
		LOCALFILE = NULL;
		return;
	}
}

int ChttpGet::ConnectSocket()
{
	in_addr_t ip;
	struct servent *se;
	struct sockaddr_in hostaddr;
	if(m_Aborting){
		return 0;
	}
	
	ip = inet_addr((const char *)m_szHost);

	int rcode = 0;
	if(ip==INADDR_NONE)
	{
		http_Asyncgethostbyname(&ip,NW_AGHBN_LOOKUP,m_szHost);		
		do
		{	
			if(m_Aborting)
			{
				http_Asyncgethostbyname(&ip,NW_AGHBN_CANCEL,m_szHost);
				return 0;
			}
			rcode = http_Asyncgethostbyname(&ip,NW_AGHBN_READ,m_szHost);

			SDL_Delay(1);
		}while(rcode==0);
	}
	
	if(rcode == -1)
	{
		m_State = HTTP_STATE_HOST_NOT_FOUND;
		return 0;
	}
	//m_ControlSock
	if(m_Aborting)
		return 0;
	se = getservbyname("http", NULL);
	if(m_Aborting)
		return 0;
	if(se == NULL)
	{
		hostaddr.sin_port = htons(80);
	}
	else
	{
		hostaddr.sin_port = se->s_port;
	}
	hostaddr.sin_family = AF_INET;		
	//ip = htonl(ip);
	hostaddr.sin_addr.s_addr = ip;

	if(m_ProxyEnabled)
	{
		//This is on a proxy, so we need to make sure to connect to the proxy machine
		ip = inet_addr((const char *)m_ProxyIP);
				
		if(ip==INADDR_NONE)
		{
			http_Asyncgethostbyname(&ip,NW_AGHBN_LOOKUP,m_ProxyIP);

			do
			{	
				if(m_Aborting)
				{
					http_Asyncgethostbyname(&ip,NW_AGHBN_CANCEL,m_ProxyIP);
					return 0;
				}
				rcode = http_Asyncgethostbyname(&ip,NW_AGHBN_READ,m_ProxyIP);

				SDL_Delay(1);
			}while(rcode==0);
			
			
			if(rcode == -1)
			{
				m_State = HTTP_STATE_HOST_NOT_FOUND;
				return 0;
			}

		}
		//Use either the proxy port or 80 if none specified
		hostaddr.sin_port = htons((ushort)(m_ProxyPort ? m_ProxyPort : 80));
		//Copy the proxy address...
		hostaddr.sin_addr.s_addr = ip;

	}
	//Now we will connect to the host					
	fd_set	wfds;

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int serr = connect(m_DataSock, (struct sockaddr *)&hostaddr, sizeof(struct sockaddr));
	int cerr = WSAGetLastError();
	if(serr)
	{
		// fail after 20 seconds
		auto failtime = SDL_GetTicks() + (20 * 1000);
		while((cerr==WSAEALREADY)||(cerr==WSAEINVAL)||NETCALL_WOULDBLOCK(cerr))
		{
			FD_ZERO(&wfds);
			FD_SET( m_DataSock, &wfds );
			if(select(m_DataSock+1,NULL,&wfds,NULL,&timeout))
			{
				int error_code = 0;
				SOCKLEN_T error_code_size = sizeof(error_code);

				// check to make sure socket is *really* connected
				int rc = getsockopt(m_DataSock, SOL_SOCKET, SO_ERROR, (char *)&error_code, &error_code_size);

				if(!rc && !error_code)
				{
					serr = 0;
				}

				break;
			}
			if(m_Aborting)
				return 0;
			serr = connect(m_DataSock, (struct sockaddr *)&hostaddr, sizeof(struct sockaddr));
			if(serr == 0)
				break;
			cerr = WSAGetLastError();
			if(cerr==WSAEISCONN)
			{
				serr = 0;
				break;
			}
			if(SDL_GetTicks()>failtime)
				break;

			SDL_Delay(1);
		};
	}
	if(serr)
	{
		m_State = HTTP_STATE_CANT_CONNECT;
		return 0;
	}
	m_State = HTTP_STATE_CONNECTED;
	return 1;
}

char *ChttpGet::GetHTTPLine()
{
	ssize_t iBytesRead;
	char chunk[2];
	unsigned int igotcrlf = 0;
	memset(recv_buffer,0,1000);
	do
	{
		chunk[0]='\0';
		bool gotdata = false;
		do
		{
			iBytesRead = recv(m_DataSock,chunk,1,0);

			if(SOCKET_ERROR == iBytesRead)
			{	
				int error = WSAGetLastError();
				if(NETCALL_WOULDBLOCK(error))
				{
					gotdata = false;
					continue;
				}
				else
					return NULL;
			}
			else
			{
				gotdata = true;
			}

			SDL_Delay(1);
		}while(!gotdata);
		
		if(chunk[0]==0x0d)
		{
			//This should always read a 0x0a
			do
			{
				iBytesRead = recv(m_DataSock,chunk,1,0);

				if(SOCKET_ERROR == iBytesRead)
				{	
					int error = WSAGetLastError();
					if(NETCALL_WOULDBLOCK(error))
					{
						gotdata = false;
						continue;
					}
					else
						return NULL;
				}
				else
				{
					gotdata = true;
				}

				SDL_Delay(1);
			}while(!gotdata);
			igotcrlf = 1;	
		}
		else
		{	chunk[1] = '\0';
			SDL_strlcat(recv_buffer, chunk, SDL_arraysize(recv_buffer));
		}
		
		SDL_Delay(1);
	}while(igotcrlf==0);
	return recv_buffer;	
}

unsigned int ChttpGet::ReadDataChannel()
{
	char sDataBuffer[4096];		// Data-storage buffer for the data channel
	ssize_t nBytesRecv = 0;						// Bytes received from the data channel

	fd_set	wfds;

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;

	m_State = HTTP_STATE_RECEIVING;			
   do	
   {
		FD_ZERO(&wfds);
		FD_SET( m_DataSock, &wfds );

		if((m_iBytesTotal)&&(m_iBytesIn==m_iBytesTotal))
		{
			break;
		}
		select(m_DataSock+1,&wfds,NULL,NULL,&timeout);
    	if(m_Aborting)
		{
			fclose(LOCALFILE);
			LOCALFILE = NULL;
			return 0;		
		}
		nBytesRecv = recv(m_DataSock, (char *)&sDataBuffer,sizeof(sDataBuffer), 0);
    	if(m_Aborting)
		{
			fclose(LOCALFILE);
			LOCALFILE = NULL;
			return 0;
		}
		if(SOCKET_ERROR == nBytesRecv)
		{	
			int error = WSAGetLastError();
			if(NETCALL_WOULDBLOCK(error))
			{
				nBytesRecv = 1;
				continue;
			}
		}
		m_iBytesIn += nBytesRecv;
		if (nBytesRecv > 0 )
		{
			fwrite(sDataBuffer,nBytesRecv,1,LOCALFILE);
			//Write sDataBuffer, nBytesRecv
    	}
		
		SDL_Delay(1);
	}while (nBytesRecv > 0);
	fclose(LOCALFILE);
	LOCALFILE = NULL;
	// Close the file and check for error returns.
	if (nBytesRecv == SOCKET_ERROR)
	{ 
		//Ok, we got a socket error -- xfer aborted?
		m_State = HTTP_STATE_RECV_FAILED;
		return 0;
	}
	else
	{
		//OutputDebugString("HTTP File complete!\n");
		//done!
		m_State = HTTP_STATE_FILE_RECEIVED;
		return 1;
	}
}	


typedef struct _async_dns_lookup
{
	in_addr_t ip;	//resolved host. Write only to worker thread.
	char * host;//host name to resolve. read only to worker thread
	bool done;	//write only to the worker thread. Signals that the operation is complete
	bool error; //write only to worker thread. Thread sets this if the name doesn't resolve
	bool abort;	//read only to worker thread. If this is set, don't fill in the struct.
}async_dns_lookup;

async_dns_lookup httpaslu;
async_dns_lookup *http_lastaslu = NULL;

int http_gethostbynameworker(void *parm);

int http_Asyncgethostbyname(in_addr_t *ip,int command, char *hostname)
{
	
	if(command==NW_AGHBN_LOOKUP)
	{
		if(http_lastaslu)
			http_lastaslu->abort = true;

		async_dns_lookup *newaslu;
		newaslu = (async_dns_lookup *)malloc(sizeof(async_dns_lookup));
		newaslu->ip = 0;
		newaslu->host = hostname;
		newaslu->done = false;
		newaslu->error = false;
		newaslu->abort = false;
		http_lastaslu = newaslu;
		httpaslu.done = false;

		auto thread = SDL_CreateThread(http_gethostbynameworker, "GetHostByNameWorker", newaslu);
		SDL_DetachThread(thread);

		return 1;
	}
	else if(command==NW_AGHBN_CANCEL)
	{
		if(http_lastaslu)
			http_lastaslu->abort = true;
		http_lastaslu = NULL;
	}
	else if(command==NW_AGHBN_READ)
	{
		if(!http_lastaslu)
			return -1;
		if(httpaslu.done)
		{
			//free(http_lastaslu);
			http_lastaslu = NULL;
			*ip = httpaslu.ip;
			return 1;
		}
		else if(httpaslu.error)
		{
			free(http_lastaslu);
			http_lastaslu = NULL;
			return -1;
		}
		else return 0;
	}
	return -2;

}

// This is the worker thread which does the lookup.
int http_gethostbynameworker(void *parm)
{
	async_dns_lookup *lookup = (async_dns_lookup *)parm;
	struct hostent *he = gethostbyname(lookup->host);
	if(he==NULL)
	{
		lookup->error = true;
		return 1;
	}
	else if(!lookup->abort)
	{
		lookup->ip = ((in_addr *)(he->h_addr))->s_addr;
		lookup->done = true;
		memcpy(&httpaslu,lookup,sizeof(async_dns_lookup));
	}
	free(lookup);

	return 0;
}
