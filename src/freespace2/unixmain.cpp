#include "pstypes.h"
#include "osregistry.h"

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
void vm_dump();

int main(int argc, char **argv)
{
	/* set some sane defaults since we don't have a laucher... */
	if (os_config_read_string(NULL, NOX("Videocard"), NULL) == NULL)
		os_config_write_string(NULL, NOX("Videocard"), NOX("OpenGL (640x480)"));
	
	if (os_config_read_string(NULL, NOX("NetworkConnection"), NULL) == NULL)
		os_config_write_string(NULL, NOX("NetworkConnection"), NOX("lan"));
	
	if (os_config_read_string(NULL, NOX("ConnectionSpeed"), NULL) == NULL)
		os_config_write_string(NULL, NOX("ConnectionSpeed"), NOX("Slow"));		
	
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
