#include "unix.h"

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
void vm_dump();

int main(int argc, char **argv)
{
	STUB_FUNCTION;

	int retr = WinMain(1, 0, "", 0);
	
	vm_dump();
	
	return retr;	
}
