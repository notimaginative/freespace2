/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/OsApi/OsApi.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Low level Windows code
 *
 * $Log$
 * Revision 1.18  2005/10/01 21:49:11  taylor
 * don't use CTRL-Z for minimizing since it's an extrememly common key combo in the game
 *
 * Revision 1.17  2004/12/15 04:10:45  taylor
 * outwnd_unix.cpp from fs2_open for logging to file in debug mode
 * fixes for default function values
 * always use vm_* functions for sanity sake
 * make cfilearchiver 64-bit compatible
 * fix crash on exit from double free()
 * fix crash on startup from extra long GL extension string in debug
 *
 * Revision 1.16  2003/12/15 06:24:51  theoddone33
 * Bumpy ride... hang on.
 *
 * Revision 1.15  2003/08/03 15:56:59  taylor
 * simpler mouse usage; default ini settings in os_init(); cleanup
 *
 * Revision 1.14  2003/05/09 05:04:15  taylor
 * better window min/max/focus support
 *
 * Revision 1.13  2003/05/04 04:56:53  taylor
 * move SDL_Quit to os_deinit to fix fonttool segfault
 *
 * Revision 1.12  2003/02/20 17:41:07  theoddone33
 * Userdir patch from Taylor Richards
 *
 * Revision 1.11  2002/07/28 21:45:30  theoddone33
 * Add ctrl-z to iconify window
 *
 * Revision 1.10  2002/07/28 21:39:44  theoddone33
 * Add alt-enter to toggle fullscreen and ctrl-g to toggle mouse grabbing
 *
 * Revision 1.9  2002/06/16 23:59:31  relnev
 * untested joystick code
 *
 * Revision 1.8  2002/06/09 04:41:25  relnev
 * added copyright header
 *
 * Revision 1.7  2002/06/05 04:03:32  relnev
 * finished cfilesystem.
 *
 * removed some old code.
 *
 * fixed mouse save off-by-one.
 *
 * sound cleanups.
 *
 * Revision 1.6  2002/05/31 03:34:02  theoddone33
 * Fix Keyboard
 * Add titlebar
 *
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
static int			fAppActive = 1;
static int			Os_inited = 0;
static char			windowTitle[128];

static SDL_Mutex *Os_lock;
static SDL_Window *Os_window = NULL;

int Os_debugger_running = 0;

// ----------------------------------------------------------------------------------------------------
// OSAPI FORWARD DECLARATIONS
//

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
void os_init(const char *wclass, const char *title, const char *app_name, const char *version_string)
{
	os_set_title( (app_name != NULL) ? app_name : title );

	// do some first-run stuff if needed
	if ( os_config_read_uint(NULL, "StraightToSetup", 1) == 1 ) {
		// set some sane config defaults
		os_init_registry_stuff();

		// unset first-run flag
		os_config_write_uint(NULL, "StraightToSetup", 0);
	}

	Os_inited = 1;

	Os_lock = SDL_CreateMutex();

	// check to see if we're running under msdev
	os_check_debugger();

	atexit(os_deinit);
}

// set the main window title
void os_set_title( const char *title )
{
	if ( !title ) {
		return;
	}

	SDL_strlcpy(windowTitle, title, SDL_arraysize(windowTitle));

	if ( !Os_window ) {
		return;
	}

	SDL_SetWindowTitle(Os_window, title);
}

const char *os_get_title()
{
	return windowTitle;
}

// call at program end
void os_cleanup()
{
#ifndef NDEBUG
		outwnd_close();
#endif
}

void os_set_icon()
{
	#include "app_icon.h"

	Uint32 rmask, gmask, bmask, amask;

	if ( !Os_window ) {
		return;
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (app_icon.bytes_per_pixel == 3) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (app_icon.bytes_per_pixel == 3) ? 0 : 0xff000000;
#endif

	SDL_Surface *icon = SDL_CreateSurfaceFrom(app_icon.width, app_icon.height,
											  SDL_GetPixelFormatForMasks(app_icon.bytes_per_pixel*8, rmask, gmask, bmask, amask),
											  (void*)app_icon.pixel_data, app_icon.bytes_per_pixel*app_icon.width);

	SDL_SetWindowIcon(Os_window, icon);

	SDL_DestroySurface(icon);
}

// window management -----------------------------------------------------------------

// Returns 0 if app is not the foreground app.
int os_foreground()
{
	return fAppActive;
}

// Returns the handle to the main window
SDL_Window *os_get_window()
{
	return Os_window;
}

void os_set_window(SDL_Window *win)
{
	Os_window = win;
}

// process management -----------------------------------------------------------------

// Used to stop message processing
void os_suspend()
{
	SDL_LockMutex(Os_lock);
}

// resume message processing
void os_resume()
{
	SDL_UnlockMutex(Os_lock);
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
	SDL_DestroyMutex(Os_lock);

	SDL_Quit();
}

void os_poll()
{
	SDL_Event e;
	int button, state;

	while (SDL_PollEvent (&e)) {
		switch (e.type) {
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				if (e.motion.windowID > 0) {
					mouse_mark_button(e.button.button, e.button.down);
				}

				break;
			}

			case SDL_EVENT_MOUSE_MOTION: {
				if (e.motion.windowID > 0) {
					mouse_update_pos(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
				}

				break;
			}

			case SDL_EVENT_TEXT_INPUT: {
				key_set_text_input((int)e.text.text[0]);

				break;
			}

			case SDL_EVENT_KEY_DOWN: {
				// flip between fullscreen and window: ALT+ENTER
				if ( (e.key.key == SDLK_RETURN) && ( e.key.mod & SDL_KMOD_ALT) ) {
					if ( !e.key.repeat ) {
						gr_toggle_fullscreen();
					}
				}
				// minimize window: CTRL+ALT+z
				else if ( (e.key.key == SDLK_Z) && (e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) ) {
					if ( !e.key.repeat ) {
					//	SDL_MinimizeWindow(Os_window);
					}
				}
				// print screen / screenshot: CTRL+ALT+p
				else if ( (e.key.key == SDLK_P) && (e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) ) {
					if ( !e.key.repeat ) {
						key_mark(SDL_SCANCODE_PRINTSCREEN, 1, 0, 0);
					}
				}
				// everything else is processed normally
				else {
					key_mark(e.key.scancode, 1, e.key.mod, 0);
				}

				break;
			}

			case SDL_EVENT_KEY_UP: {
				key_mark(e.key.scancode, 0, e.key.mod, 0);

				break;
			}

			case SDL_EVENT_JOYSTICK_ADDED: {
				if ( !Is_standalone && (e.jdevice.which != joystick_get_id()) ) {
					joy_reinit(e.jdevice.which);
				}

				break;
			}

			case SDL_EVENT_JOYSTICK_REMOVED: {
				// if the active joystick is removed then maybe reinit
				if ( !Is_standalone && (e.jdevice.which == joystick_get_id()) ) {
					joy_reinit();
				}

				break;
			}

			case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
				if (e.gaxis.which == joystick_get_id()) {
					joystick_update_axis(e.gaxis.axis, e.gaxis.value);
				}

				break;
			}

			case SDL_EVENT_JOYSTICK_AXIS_MOTION: {
				if ( joystick_is_gamepad() ) {
					break;
				}

				if (e.jaxis.which == joystick_get_id()) {
					joystick_update_axis(e.jaxis.axis, e.jaxis.value);
				}

				break;
			}

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP: {
				if (e.gbutton.which == joystick_get_id()) {
					// convert DPAD to HAT
					switch (e.gbutton.button) {
						case SDL_GAMEPAD_BUTTON_DPAD_UP:
							button = JOY_HATFORWARD;
							break;

						case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
							button = JOY_HATBACK;
							break;

						case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
							button = JOY_HATLEFT;
							break;

						case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
							button = JOY_HATRIGHT;
							break;

						default:
							button = e.gbutton.button;
					}

					joy_mark_button(button, e.gbutton.down);
				}

				break;
			}

			case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
			case SDL_EVENT_JOYSTICK_BUTTON_UP: {
				if ( joystick_is_gamepad() ) {
					break;
				}

				if (e.jbutton.which == joystick_get_id()) {
					joy_mark_button((int)e.jbutton.button, e.jbutton.down);
				}

				break;
			}

			case SDL_EVENT_JOYSTICK_HAT_MOTION: {
				if ( joystick_is_gamepad() ) {
					break;
				}

				if (e.jhat.which == joystick_get_id()) {
					// can only handle one hat
					if (e.jhat.hat == 0) {
						switch (e.jhat.value) {
							case SDL_HAT_UP:
								button = JOY_HATFORWARD;
								state = 1;
								break;
							case SDL_HAT_DOWN:
								button = JOY_HATBACK;
								state = 1;
								break;
							case SDL_HAT_LEFT:
								button = JOY_HATLEFT;
								state = 1;
								break;
							case SDL_HAT_RIGHT:
								button = JOY_HATRIGHT;
								state = 1;
								break;
							default:
								// special case - will toggle all hat positions off
								button = JOY_HATBACK;
								state = 0;
								break;
						}

						joy_mark_button(button, state);
					}
				}

				break;
			}

			case SDL_EVENT_WINDOW_RESIZED:
				gr_set_viewport(e.window.data1, e.window.data2);
				// ungrab mouse, it will be grabbed again if needed
				mouse_grab(0);
				break;

			case SDL_EVENT_WINDOW_FOCUS_LOST:
				fAppActive = 0;
				// io stuff
				mouse_grab(0);
				joy_unacquire_ff();
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				fAppActive = 1;
				// io stuff
				joy_reacquire_ff();
				break;

			case SDL_EVENT_WINDOW_MINIMIZED:
				fAppActive = 0;
				// io stuff
				mouse_grab(0);
				joy_unacquire_ff();
				// make sure game pauses
			//	game_process_pause_key();
				// graphics
			//	gr_activate(fAppActive);
				break;

			case SDL_EVENT_WINDOW_MAXIMIZED:
			case SDL_EVENT_WINDOW_RESTORED: {
				fAppActive = 1;
				// io stuff
				mouse_grab(0);
				joy_reacquire_ff();
				// graphics
			//	gr_activate(fAppActive);
				break;
			}

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			//	gameseq_post_event(GS_EVENT_QUIT_GAME);
				break;

			case SDL_EVENT_QUIT:
				gameseq_post_event(GS_EVENT_QUIT_GAME);
				break;

			default:
				break;
		}
	}
}

void debug_int3()
{
	bool mode = SDL_GetWindowRelativeMouseMode(Os_window);
	SDL_SetWindowRelativeMouseMode(Os_window, false);
	bool grab = SDL_GetWindowMouseGrab(Os_window);
	SDL_SetWindowMouseGrab(Os_window, false);

	SDL_TriggerBreakpoint();

	SDL_SetWindowRelativeMouseMode(Os_window, mode);
	SDL_SetWindowMouseGrab(Os_window, grab);
}
