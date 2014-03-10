// unix.h - duplicates some MS defines

#ifndef _UNIX_H
#define _UNIX_H


// same thing that's in FS2_Open (credit: Mike Harris)
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STR "/"

#define __cdecl
#define HANDLE int

extern int filelength (int fd);

#define ioctlsocket(A,B,C) ioctl(A,B,C)
#define ioctlsocket(A,B,C) ioctl(A,B,C)
#define closesocket(A) close(A)
#define SOCKET int
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

extern int WSAGetLastError ();

#define WSAEALREADY EALREADY
#define WSAEINVAL EINVAL
#define WSAEWOULDBLOCK EAGAIN
#define WSAEISCONN EISCONN
#define WSAECONNRESET ECONNRESET
#define WSAECONNABORTED ECONNABORTED
#define WSAESHUTDOWN ESHUTDOWN
#define WSAENOTSOCK ENOTSOCK

#endif	// _UNIX_H
