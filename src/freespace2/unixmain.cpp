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
	
	int retr = WinMain(1, 0, "", 0);
	
	vm_dump();
	
	return retr;	
}
