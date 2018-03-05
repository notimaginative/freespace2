#ifndef PLATFORM_H
#define PLATFORM_H


#include <stdio.h>	// For NULL, etc
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>

#ifdef PLAT_UNIX
#include "unix.h"
#else
#include "win.h"
#endif


void base_filename(const char *path, char *filename, const int max_fname);

void platform_init();
void platform_close();

int platform_open_url(const char *url);

unsigned int platform_get_kmod();

#endif // PLATFORM_H
