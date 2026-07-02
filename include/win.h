#ifndef WIN_H
#define WIN_H


// same thing that's in FS2_Open (credit: Mike Harris)
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STR "\\"

#define isnan _isnan
#define unlink _unlink
#define access _access
#define stat _stat
#define chdir _chdir

#define SOCKLEN_T int

#define NETCALL_WOULDBLOCK(err)	(err == WSAEWOULDBLOCK)

#define MSG_NOSIGNAL 0

typedef unsigned long in_addr_t;

#if defined(_MSC_VER)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#endif // WIN_H
