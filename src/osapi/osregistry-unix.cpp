#include "pstypes.h"

char *Osreg_company_name = "Volition";
char *Osreg_class_name = "Freespace2Class";
#if defined(FS2_DEMO)
char *Osreg_app_name = "FreeSpace2Demo";
char *Osreg_title = "Freespace 2 Demo";
#elif defined(OEM_BUILD)
char *Osreg_app_name = "FreeSpace2OEM";
char *Osreg_title = "Freespace 2 OEM";
#else
char *Osreg_app_name = "FreeSpace2";
char *Osreg_title = "Freespace 2";
#endif

char *os_config_read_string(char *section, char *name, char *default_value)
{
	STUB_FUNCTION;
	
	return "";
}

unsigned int os_config_read_uint(char *section, char *name, unsigned int default_value)
{
	STUB_FUNCTION;
	
	return 0;
}

void os_config_write_string(char *section, char *name, char *value)
{
	STUB_FUNCTION;
}

void os_config_write_uint(char *section, char *name, unsigned int value)
{
	STUB_FUNCTION;
}
