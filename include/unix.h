// unix.h - duplicates some MS defines

#ifndef _UNIX_H
#define _UNIX_H

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define _MAX_FNAME 255
#define _MAX_PATH 255
#define MAX_PATH 255
#define __cdecl
#define _isnan isnan
#define HANDLE int
#define _getcwd getcwd
#define _chdir chdir
#define _unlink unlink
#define _hypot hypot
#define _access access
#define byte unsigned char

extern int filelength (int fd);
extern int WSAGetLastError ();
extern void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext);

#define _mkdir(A) mkdir(A,0700)
#define closesocket(A) close(A)
#define CopyMemory(A,B,C) memcpy(A,B,C)
#define UINT unsigned int
#define SOCKET int
#define SOCKADDR_IN struct sockaddr_in
#define SOCKADDR struct sockaddr
#define LPSOCKADDR struct sockaddr*
#define LPHOSTENT struct hostent*
#define HOSTENT struct hostent
#define LPINADDR struct in_addr*
#define LPIN_ADDR struct in_addr*
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define TIMEVAL struct timeval
#define SERVENT struct servent
#define BOOL int

#define WSAEALREADY EALREADY
#define WSAEINVAL EINVAL
#define WSAEWOULDBLOCK EAGAIN
#define WSAEISCONN EISCONN
#define WSAECONNRESET ECONNRESET
#define WSAECONNABORTED ECONNABORTED
#define WSAESHUTDOWN ESHUTDOWN
#define WSAENOTSOCK ENOTSOCK

#endif
