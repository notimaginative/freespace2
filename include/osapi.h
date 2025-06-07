/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef _OSAPI_H
#define _OSAPI_H

#include <stdlib.h>

#include "pstypes.h"

// --------------------------------------------------------------------------------------------------
// OSAPI DEFINES/VARS
//


// --------------------------------------------------------------------------------------------------
// OSAPI FUNCTIONS
//

// initialization/shutdown functions -----------------------------------------------


/// Initialize basic app functions and settings, and optionally set window title and app id if not default
void os_init(const char *title = nullptr, const char *appid = nullptr);

// set the main window title
void os_set_title( const char *title );
// get the main window title
const char *os_get_title();

void os_set_icon(SDL_Window *window = nullptr);

// call at program end
void os_cleanup();


// window management ---------------------------------------------------------------

// toggle window size between full screen and windowed
void os_toggle_fullscreen();

// Returns 1 if app is not the foreground app.
int os_foreground();

// Returns the handle to the main window
SDL_Window *os_get_window();
// Sets the handle to the main window
void os_set_window(SDL_Window *win);

// process management --------------------------------------------------------------

// call to process windows messages. only does something in non THREADED mode
void os_poll();

// Used to stop message processing
void os_suspend();

// resume message processing
void os_resume();

#endif

