#include <sys/stat.h>

#include "pstypes.h"
#include "osregistry.h"
#include "osapi.h"

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);

#ifdef __APPLE__
char full_path[1024];
#endif

void vm_dump();

int main(int argc, char **argv)
{
#ifdef __APPLE__
        strcpy( full_path, *argv );
#endif
	char userdir[MAX_PATH];
	
	// create user game directory
	snprintf(userdir, MAX_PATH, "%s/%s", detect_home(), Osreg_user_dir);
	_mkdir(userdir, 0700);	
	
	char *argptr = NULL;
	int i;
	int len = 1;
	
	argptr = (char *)malloc(1);
	*argptr = 0;
	
	for (i = 1; i < argc; i++) {
		int oldlen = len-1;
		
		len += strlen(argv[i])+1;
		
		argptr = (char *)realloc(argptr, len);
		if (argptr == NULL) {
			fprintf(stderr, "ERROR: out of memory in main!\n");
			exit(1);
		}
		
		strcpy(argptr+oldlen, argv[i]);
		strcat(argptr, " ");
	}
	 
	int retr = WinMain(1, 0, argptr, 0);

	free(argptr);
		
	vm_dump();
	
	return retr;	
}
