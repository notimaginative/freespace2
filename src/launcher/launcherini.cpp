
/*
 * Basically the same as osregistry.cpp but without having to be tied into
 * cfile.
 *
 * External function calls should remain identical to what's in core lib so that
 * the launcher can just include osregistry.h like elsewhere
 */


#include "pstypes.h"
#include "osregistry.h"
#include "cfile.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char *Osreg_company_name = "Volition";
#if defined(MAKE_FS1)
const char *Osreg_class_name = "FreeSpaceClass";
#else
const char *Osreg_class_name = "Freespace2Class";
#endif
#if defined(FS1_DEMO)
const char *Osreg_app_name = "FreeSpaceDemo";
const char *Osreg_title = "FreeSpace Demo";
#define PROFILE_NAME "FreeSpaceDemo.ini"
#elif defined(FS2_DEMO)
const char *Osreg_app_name = "FreeSpace2Demo";
const char *Osreg_title = "Freespace 2 Demo";
#define PROFILE_NAME "Freespace2Demo.ini"
#elif defined(OEM_BUILD)
const char *Osreg_app_name = "FreeSpace2OEM";
const char *Osreg_title = "Freespace 2 OEM";
#define PROFILE_NAME "Freespace2OEM.ini"
#elif defined(MAKE_FS1)
const char *Osreg_app_name = "FreeSpace";
const char *Osreg_title = "FreeSpace";
#define PROFILE_NAME "FreeSpace.ini"
#else
const char *Osreg_app_name = "FreeSpace2";
const char *Osreg_title = "Freespace 2";
#define PROFILE_NAME "Freespace2.ini"
#endif

#define DEFAULT_SECTION "Default"

static int Profile_initted = 0;
static char PROFILE_PATH[MAX_PATH_LEN] = { 0 };


typedef struct KeyValue
{
	char *key;
	char *value;
	
	struct KeyValue *next;
} KeyValue;

typedef struct Section
{
	char *name;
	
	struct KeyValue *pairs;
	struct Section *next;
} Section;
	
typedef struct Profile
{
	struct Section *sections;
} Profile;

static int profile_init()
{
	if (Profile_initted) {
		return 0;
	}

	char *u_path = SDL_GetPrefPath(Osreg_company_name, Osreg_title);

	// make sure we have something
	if (u_path == NULL) {
		return 1;
	}

	// size check
	if ( (strlen(u_path) + strlen(PROFILE_NAME) + 1) >= MAX_PATH_LEN ) {
		SDL_free(u_path);
		return 1;
	}

	// set profile location
	SDL_snprintf(PROFILE_PATH, SDL_arraysize(PROFILE_PATH), "%s%s%s", u_path, DIR_SEPARATOR_STR, PROFILE_NAME);

	// free SDL copy
	SDL_free(u_path);

	Profile_initted = 1;

	return 0;
}

static char *read_line_from_file(FILE *fp)
{
	char *buf, *buf_start;
	int buflen, len, eol;
	
	buflen = 80;
	buf = (char *)SDL_malloc(buflen);
	buf_start = buf;
	eol = 0;
	
	do {
		if (buf == NULL) {
			return NULL;
		}
		
		if (fgets(buf_start, 80, fp) == NULL) {
			if (buf_start == buf) {
				SDL_free(buf);
				return NULL;
			} else {
				*buf_start = 0;
				return buf;
			}
		}
		
		len = SDL_strlen(buf_start);
		
		if (buf_start[len-1] == '\n') {
			buf_start[len-1] = 0;
			eol = 1;
		} else {
			buflen += 80;
			
			buf = (char *)SDL_realloc(buf, buflen);
			
			/* be sure to skip over the proper amount of nulls */
			buf_start = buf+(buflen-80)-(buflen/80)+1;
		}
	} while (!eol);
	
	return buf;
}

static char *trim_string(char *str)
{
	char *ptr;
	int len;
	
	if (str == NULL)
		return NULL;
	
	/* kill any comment */
	ptr = SDL_strchr(str, ';');
	if (ptr)
		*ptr = 0;
	ptr = SDL_strchr(str, '#');
	if (ptr)
		*ptr = 0;
	
	ptr = str;
	len = SDL_strlen(str);
	if (len > 0) {
		ptr += len-1;
	}
	
	while ((ptr > str) && SDL_isspace(*ptr)) {
		ptr--;
	}

	if (*ptr) {
		ptr++;
		*ptr = 0;
	}
	
	ptr = str;
	while (*ptr && SDL_isspace(*ptr)) {
		ptr++;
	}
	
	return ptr;
}

static Profile *profile_read()
{
	if ( profile_init() ) {
		return NULL;
	}

	FILE *fp = fopen(PROFILE_PATH, "rt");
	if (fp == NULL)
		return NULL;
	
	Profile *profile = (Profile *)SDL_malloc(sizeof(Profile));
	profile->sections = NULL;
	
	Section **sp_ptr = &(profile->sections);
	Section *sp = NULL;

	KeyValue **kvp_ptr = NULL;
		
	char *str;
	while ((str = read_line_from_file(fp)) != NULL) {
		char *ptr = trim_string(str);
		
		if (*ptr == '[') {
			ptr++;
			
			char *pend = SDL_strchr(ptr, ']');
			if (pend != NULL) {
				// if (pend[1]) { /* trailing garbage! */ }
				
				*pend = 0;				
				
				if (*ptr) {
					sp = (Section *)SDL_malloc(sizeof(Section));
					sp->next = NULL;
				
					sp->name = SDL_strdup(ptr);
					sp->pairs = NULL;
					
					*sp_ptr = sp;
					sp_ptr = &(sp->next);
					
					kvp_ptr = &(sp->pairs);
				} // else { /* null name! */ }
			} // else { /* incomplete section name! */ }
		} else {
			if (*ptr) {
				char *key = ptr;
				char *value = NULL;
				
				ptr = SDL_strchr(ptr, '=');
				if (ptr != NULL) {
					*ptr = 0;
					ptr++;
					
					value = ptr;
				} // else { /* random garbage! */ }
				
				if (key && *key && value /* && *value */) {
					if (sp != NULL) {
						KeyValue *kvp = (KeyValue *)SDL_malloc(sizeof(KeyValue));
						
						kvp->key = SDL_strdup(key);
						kvp->value = SDL_strdup(value);
						
						kvp->next = NULL;
						
						*kvp_ptr = kvp;
						kvp_ptr = &(kvp->next);
					} // else { /* key/value with no section! */
				} // else { /* malformed key/value entry! */ }
			} // else it's just a comment or empty string
		}
				
		SDL_free(str);
	}
	
	fclose(fp);

	return profile;
}

static void profile_free(Profile *profile)
{
	if (profile == NULL)
		return;
		
	Section *sp = profile->sections;
	while (sp != NULL) {
		Section *st = sp;
		KeyValue *kvp = sp->pairs;
		
		while (kvp != NULL) {
			KeyValue *kvt = kvp;
			
			SDL_free(kvp->key);
			SDL_free(kvp->value);
			
			kvp = kvp->next;
			SDL_free(kvt);
		}
		
		SDL_free(sp->name);
		
		sp = sp->next;
		SDL_free(st);
	}
	
	SDL_free(profile);
}

static Profile *profile_update(Profile *profile, const char *section, const char *key, const char *value)
{
	if (profile == NULL) {
		profile = (Profile *)SDL_malloc(sizeof(Profile));
		
		profile->sections = NULL;
	}
	
	KeyValue *kvp;
	
	Section **sp_ptr = &(profile->sections);
	Section *sp = profile->sections;
	while (sp != NULL) {
		if (SDL_strcmp(section, sp->name) == 0) {
			KeyValue **kvp_ptr = &(sp->pairs);
			kvp = sp->pairs;
			
			while (kvp != NULL) {
				if (SDL_strcmp(key, kvp->key) == 0) {
					SDL_free(kvp->value);
					
					if (value == NULL) {
						*kvp_ptr = kvp->next;
						
						SDL_free(kvp->key);
						SDL_free(kvp);
					} else {
						kvp->value = SDL_strdup(value);
					}
					
					/* all done */
					return profile;
				}
				
				kvp_ptr = &(kvp->next);
				kvp = kvp->next;
			}
			
			if (value != NULL) {
				/* key not found */
				kvp = (KeyValue *)SDL_malloc(sizeof(KeyValue));
				kvp->next = NULL;
				kvp->key = SDL_strdup(key);
				kvp->value = SDL_strdup(value);
			}
					
			*kvp_ptr = kvp;
			
			/* all done */
			return profile;
		}
		
		sp_ptr = &(sp->next);
		sp = sp->next;
	}
	
	/* section not found */
	sp = (Section *)SDL_malloc(sizeof(Section));
	sp->next = NULL;
	sp->name = SDL_strdup(section);
	
	kvp = (KeyValue *)SDL_malloc(sizeof(KeyValue));
	kvp->next = NULL;
	kvp->key = SDL_strdup(key);
	kvp->value = SDL_strdup(value);
	
	sp->pairs = kvp;
	
	*sp_ptr = sp;
	
	return profile;
}

static const char *profile_get_value(Profile *profile, const char *section, const char *key)
{
	if (profile == NULL)
		return NULL;
	
	Section *sp = profile->sections;
	while (sp != NULL) {
		if (SDL_strcmp(section, sp->name) == 0) {
			KeyValue *kvp = sp->pairs;
		
			while (kvp != NULL) {
				if (SDL_strcmp(key, kvp->key) == 0) {
					return kvp->value;
				}
				kvp = kvp->next;
			}
		}
		
		sp = sp->next;
	}
	
	/* not found */
	return NULL;
}

static char tmp_string_data[1024];

static void profile_save(Profile *profile)
{
	FILE *fp;

	if (profile == NULL)
		return;

	if ( profile_init() ) {
		return;
	}

	fp = fopen(PROFILE_PATH, "wt");
	if (fp == NULL)
		return;
	
	Section *sp = profile->sections;
	while (sp != NULL) {
		SDL_snprintf(tmp_string_data, SDL_arraysize(tmp_string_data), "[%s]\n", sp->name);
		fputs(tmp_string_data, fp);
		
		KeyValue *kvp = sp->pairs;
		while (kvp != NULL) {
			SDL_snprintf(tmp_string_data, SDL_arraysize(tmp_string_data), "%s=%s\n", kvp->key, kvp->value);
			fputs(tmp_string_data, fp);
			kvp = kvp->next;
		}
		
		fputc('\n', fp);

		sp = sp->next;
	}
	
	fclose(fp);
}

const char *os_config_read_string(const char *section, const char *name, const char *default_value)
{
	Profile *p = profile_read();

	if (section == NULL)
		section = DEFAULT_SECTION;
		
	const char *ptr = profile_get_value(p, section, name);
	if ( (ptr != NULL) && SDL_strlen(ptr) ) {
		SDL_strlcpy(tmp_string_data, ptr, SDL_arraysize(tmp_string_data));
		default_value = tmp_string_data;
	}
	
	profile_free(p);
	
	return default_value;
}

unsigned int os_config_read_uint(const char *section, const char *name, unsigned int default_value)
{
	Profile *p = profile_read();
	
	if (section == NULL)
		section = DEFAULT_SECTION;
		
	const char *ptr = profile_get_value(p, section, name);
	if ( (ptr != NULL) && SDL_strlen(ptr) ) {
		default_value = SDL_atoi(ptr);
	}
	
	profile_free(p);
	
	return default_value;
}

void os_config_write_string(const char *section, const char *name, const char *value)
{
	Profile *p = profile_read();
	
	if (section == NULL)
		section = DEFAULT_SECTION;
		
	p = profile_update(p, section, name, value);
	profile_save(p);
	profile_free(p);	
}

void os_config_write_uint(const char *section, const char *name, unsigned int value)
{
	static char buf[21];
	
	SDL_snprintf(buf, SDL_arraysize(buf), "%u", value);
	
	Profile *p = profile_read();

	if (section == NULL)
		section = DEFAULT_SECTION;
	
	p = profile_update(p, section, name, buf);
	profile_save(p);
	profile_free(p);
}
