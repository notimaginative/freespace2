/*
 * $Logfile: /Freespace2/code/OsApi/OsApi.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Low level Windows code
 *
 * $Log$
 * Revision 1.5  2002/05/30 23:46:29  theoddone33
 * some minor key changes (not necessarily fixes)
 *
 * Revision 1.4  2002/05/30 16:50:24  theoddone33
 * Keyboard partially fixed
 *
 * Revision 1.3  2002/05/29 06:25:13  theoddone33
 * Keyboard input, mouse tracking now work
 *
 * Revision 1.2  2002/05/07 03:16:48  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:10  root
 * Initial import.
 * 
 * 
 * 7     6/30/99 5:53p Dave
 * Put in new anti-camper code.
 * 
 * 6     6/03/99 6:37p Dave
 * More TNT fun. Made perspective bitmaps more flexible.
 * 
 * 5     6/02/99 6:18p Dave
 * Fixed TNT lockup problems! Wheeeee!
 * 
 * 4     12/18/98 1:13a Dave
 * Rough 1024x768 support for Direct3D. Proper detection and usage through
 * the launcher.
 * 
 * 3     10/09/98 2:57p Dave
 * Starting splitting up OS stuff.
 * 
 * 2     10/08/98 2:38p Dave
 * Cleanup up OsAPI code significantly. Removed old functions, centralized
 * registry functions.
 * 
 * 118   7/10/98 5:04p Dave
 * Fix connection speed bug on standalone server.
 * 
 * 117   5/24/98 2:28p Hoffoss
 * Because we never really care about if the left or the right shift or
 * alt key was used, but rather than either shift or alt was used, made
 * both map to the left one.  Solves some problems, causes none.
 * 
 * 116   5/18/98 9:22p John
 * Took out the annoying tab check.
 * 
 * 115   5/18/98 11:17a John
 * Fixed some bugs with software window and output window.
 * 
 * 114   5/16/98 2:20p John
 * Changed the os_suspend and resume to use a critical section to prevent
 * threads from executing rather than just suspending the thread.  Had to
 * make sure gr_close was called before os_close.
 * 
 * 113   5/15/98 4:49p John
 * 
 * 112   5/15/98 3:36p John
 * Fixed bug with new graphics window code and standalone server.  Made
 * hwndApp not be a global anymore.
 * 
 * 111   5/14/98 5:42p John
 * Revamped the whole window position/mouse code for the graphics windows.
 * 
 * 110   5/04/98 11:08p Hoffoss
 * Expanded on Force Feedback code, and moved it all into Joy_ff.cpp.
 * Updated references everywhere to it.
 *
 * $NoKeywords: $
 */

#include "pstypes.h"
#include "osapi.h"
#include "key.h"
#include "palman.h"
#include "mouse.h"
#include "outwnd.h"
#include "2d.h"
#include "cfile.h"
#include "sound.h"
#include "freespaceresource.h"
#include "managepilot.h"
#include "joy.h"
#include "joy_ff.h"
#include "gamesequence.h"
#include "freespace.h"
#include "osregistry.h"
#include "cmdline.h"

// ----------------------------------------------------------------------------------------------------
// OSAPI DEFINES/VARS
//

// os-wide globals
static int			fAppActive = 0;
static int			main_window_inited = 0;
static char			szWinTitle[128];
static char			szWinClass[128];
static int			WinX, WinY, WinW, WinH;
static int			Os_inited = 0;

static CRITICAL_SECTION Os_lock;

int Os_debugger_running = 0;

// ----------------------------------------------------------------------------------------------------
// OSAPI FORWARD DECLARATIONS
//

#ifdef THREADED
	// thread handler for the main message thread
	DWORD win32_process(DWORD lparam);
#else
	DWORD win32_process1(DWORD lparam);
	DWORD win32_process1(DWORD lparam);
#endif

// Fills in the Os_debugger_running with non-zero if debugger detected.
void os_check_debugger();

// called at shutdown. Makes sure all thread processing terminates.
void os_deinit();


// ----------------------------------------------------------------------------------------------------
// OSAPI FUNCTIONS
//

// initialization/shutdown functions -----------------------------------------------

// If app_name is NULL or ommited, then TITLE is used
// for the app name, which is where registry keys are stored.
void os_init(char * wclass, char * title, char *app_name, char *version_string )
{
	STUB_FUNCTION;

	Os_inited = 1;

	// check to see if we're running under msdev
	os_check_debugger();

	atexit(os_deinit);
}

// set the main window title
void os_set_title( char * title )
{
	STUB_FUNCTION;
}

// call at program end
void os_cleanup()
{
	STUB_FUNCTION;
	
	#ifndef NDEBUG
		outwnd_close();
	#endif
}


// window management -----------------------------------------------------------------

static int app_active = 1;
// Returns 1 if app is not the foreground app.
int os_foreground()
{
	return app_active;
}

// Returns the handle to the main window
uint os_get_window()
{
	STUB_FUNCTION;
	return 0;
}


// process management -----------------------------------------------------------------

// Sleeps for n milliseconds or until app becomes active.
void os_sleep(int ms)
{
	usleep(ms*1000);
}

// Used to stop message processing
void os_suspend()
{
	ENTER_CRITICAL_SECTION(&Os_lock);	
}

// resume message processing
void os_resume()
{
	LEAVE_CRITICAL_SECTION(&Os_lock);	
}


// ----------------------------------------------------------------------------------------------------
// OSAPI FORWARD DECLARATIONS
//

// Fills in the Os_debugger_running with non-zero if debugger detected.
void os_check_debugger()
{
}

// called at shutdown. Makes sure all thread processing terminates.
void os_deinit()
{
}

void os_poll()
{
	SDL_Event e;

	while (SDL_PollEvent (&e)) {
		switch (e.type) {
			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == SDL_BUTTON_LEFT)
					mouse_mark_button (MOUSE_LEFT_BUTTON,1);
				else if (e.button.button == SDL_BUTTON_RIGHT)
					mouse_mark_button (MOUSE_RIGHT_BUTTON,1);
				else if (e.button.button == SDL_BUTTON_MIDDLE)
					mouse_mark_button (MOUSE_MIDDLE_BUTTON, 1);
				break;
			case SDL_MOUSEBUTTONUP:
				if (e.button.button == SDL_BUTTON_LEFT)
					mouse_mark_button (MOUSE_LEFT_BUTTON,0);
				else if (e.button.button == SDL_BUTTON_RIGHT)
					mouse_mark_button (MOUSE_RIGHT_BUTTON,0);
				else if (e.button.button == SDL_BUTTON_MIDDLE)
					mouse_mark_button (MOUSE_MIDDLE_BUTTON, 0);
				break;
			case SDL_KEYDOWN:
				key_mark ((e.key.keysym.mod<<16) | e.key.keysym.sym, 1, 0);
				break;
			case SDL_KEYUP:
				key_mark ((e.key.keysym.mod<<16) | e.key.keysym.sym, 0, 0);
				break;
			default:
				break;
		}
	}
}

void debug_int3()
{
}

