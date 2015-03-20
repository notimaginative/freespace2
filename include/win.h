#ifndef WIN_H
#define WIN_H


// same thing that's in FS2_Open (credit: Mike Harris)
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STR "\\"

#define mkdir(A,B) _mkdir(A)

#define isnan _isnan
#define unlink _unlink
#define access _access
#define stat _stat

#endif // WIN_H
