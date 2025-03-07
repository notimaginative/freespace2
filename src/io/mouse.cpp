/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Io/Mouse.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Routines to read the mouse.
 *
 * $Log$
 * Revision 1.7  2003/05/25 02:30:42  taylor
 * Freespace 1 support
 *
 * Revision 1.6  2002/07/13 06:46:48  theoddone33
 * Warning cleanups
 *
 * Revision 1.5  2002/06/09 04:41:22  relnev
 * added copyright header
 *
 * Revision 1.4  2002/06/02 04:26:34  relnev
 * warning cleanup
 *
 * Revision 1.3  2002/05/29 06:25:13  theoddone33
 * Keyboard input, mouse tracking now work
 *
 * Revision 1.2  2002/05/07 03:16:46  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:09  root
 * Initial import.
 *
 * 
 * 4     7/15/99 9:20a Andsager
 * FS2_DEMO initial checkin
 * 
 * 3     6/02/99 6:18p Dave
 * Fixed TNT lockup problems! Wheeeee!
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 29    6/10/98 2:52p Hoffoss
 * Made mouse code use DI by default, but fall back on normal code if that
 * fails.
 * 
 * 28    5/24/98 1:35p Hoffoss
 * Fixed bug where  mouse cursor is always recentering with a
 * mouse_flush() call in debug version.
 * 
 * 27    5/22/98 4:50p Hoffoss
 * Trying to fix mouse acceleration problem..
 * 
 * 26    5/21/98 12:26p Lawrance
 * Fixed mouse jerk at mission start while in debug build only.
 * 
 * 25    5/15/98 2:41p Hoffoss
 * Made mouse default to off (for flying ship) and fixed some new pilot
 * init bugs.
 * 
 * 24    5/08/98 4:13p Hoffoss
 * Fixed problem with mouse pointer centering causing lost keypresses.
 * 
 * 23    5/07/98 6:58p Hoffoss
 * Made changes to mouse code to fix a number of problems.
 * 
 * 22    5/05/98 8:38p Hoffoss
 * Added sensitivity adjustment to options menu and made it save to pilot
 * file.
 * 
 * 21    5/05/98 1:03p Hoffoss
 * Fixed initialization bug.
 * 
 * 20    5/01/98 5:45p Hoffoss
 * Made further improvements to the mouse code.
 * 
 * 19    5/01/98 3:35p Hoffoss
 * Made changes to release version of mouse code.
 * 
 * 18    5/01/98 1:14p Hoffoss
 * Changed mouse usage so directInput is only used for release version.
 * 
 * 17    4/30/98 5:40p Hoffoss
 * Added mouse as a supported control to fly the ship.
 * 
 * 16    4/29/98 12:13a Lawrance
 * Add function to check down count of mouse button without reseting the
 * internal count.  Added hook to reset demo trailer timer when a button
 * is pressed.
 * 
 * 15    4/02/98 5:26p John
 * 
 * 14    1/19/98 6:15p John
 * Fixed all my Optimized Build compiler warnings
 * 
 * 13    12/04/97 3:47p John
 * Made joystick move mouse cursor
 * 
 * 12    11/20/97 5:36p Dave
 * Hooked in a bunch of main hall changes (including sound). Made it
 * possible to reposition (rewind/ffwd) 
 * sound buffer pointers. Fixed animation direction change framerate
 * problem.
 * 
 * 11    5/12/97 11:41a John
 * Added range checking to mouse position
 * 
 * 10    4/22/97 5:55p Lawrance
 * let mouse.cpp decide if mouse has moved
 * 
 * 9     4/22/97 12:29p John
 * Changed mouse code so that you have to call mouse_init for the mouse
 * stuff to work.
 * 
 * 8     4/22/97 10:56a John
 * fixed some resource leaks.
 * 
 * 7     3/26/97 10:52a Lawrance
 * mouse always on in menus, disappears in gameplay after 1 second
 * 
 * 6     3/11/97 1:37p Lawrance
 * added mouse_up_count(), changed mouse_mark() to mouse_mark_button() &
 * mouse_mark_move()
 * 
 * 5     12/09/96 1:29p Lawrance
 * adding 3 button support
 * 
 * 4     12/03/96 4:19p John
 * Added some code so that holding down the mouse buttons between menus
 * doesn't select the next menu.
 *
 * $NoKeywords: $
 */


#include "mouse.h"
#include "2d.h"
#include "osapi.h"
#include "gamepad.h"
#include "bmpman.h"


static int mouse_inited = 0;

static int mouse_flags;
static int mouse_left_pressed = 0;
static int mouse_right_pressed = 0;
static int mouse_middle_pressed = 0;
static int mouse_left_up = 0;
static int mouse_right_up = 0;
static int mouse_middle_up = 0;

static int Mouse_x;
static int Mouse_y;
// total mouse delta motion each game frame
static int Mouse_dx = 0;
static int Mouse_dy = 0;
static int Mouse_dz = 0;
// accumulation of mouse delta motion during each game frame
static int Mouse_dx_inc = 0;
static int Mouse_dy_inc = 0;

int Mouse_sensitivity = 4;
int Use_mouse_to_fly = 0;
int Keep_mouse_centered = 0;

void mouse_force_pos(int x, int y);



// -----------------------------------------------------------------------
// mouse_create_cursor()
//
// Creates an SDL_Cursor object from a loaded bitmap. Returns nullptr on any failure
// or the cursor object on success (which must be released with SDL_DestroyCursor!).
SDL_Cursor *mouse_create_cursor(int bmap_id)
{
	if (bmap_id < 0) {
		return nullptr;
	}

	auto bmp = bm_lock(bmap_id, 16, BMP_TEX_XPARENT);

	if ( !bmp ) {
		return nullptr;
	}

	auto surface = SDL_CreateSurfaceFrom(bmp->w, bmp->h,
										 SDL_PIXELFORMAT_RGBA5551,
										 reinterpret_cast<void *>(bmp->data),
										 bmp->rowsize * 2);

	bm_unlock(bmap_id);
	bmp = nullptr;

	if ( !surface ) {
		return nullptr;
	}

	auto cursor = SDL_CreateColorCursor(surface, 0, 0);

	SDL_DestroySurface(surface);

	return cursor;
}

// -----------------------------------------------------------------------
// mouse_set_cursor()
//
// Set the current mouse pointer.  This is called by the animating mouse
// pointer code.
//
// The lock parameter basically disables the next call of this function that doesnt
// have an unlock feature.  If adding in more cursor-changing situations, be aware of
// unexpected results. You have been warned.
bool mouse_set_cursor(SDL_Cursor *cursor, int lock)
{
	static bool locked = false;
	bool rval = false;

	if ( !locked || (lock == MOUSE_CURSOR_UNLOCK) ) {
		if (cursor) {
			SDL_SetCursor(cursor);
		}

		rval = true;
	} else {
		locked = false;
	}

	if (lock == MOUSE_CURSOR_LOCK) {
		locked = true;
	}

	return rval;
}

void mouse_hide_cursor()
{
	SDL_HideCursor();
}

void mouse_show_cursor()
{
	SDL_ShowCursor();
}

bool mouse_is_visible()
{
	return SDL_CursorVisible();
}

void mouse_close()
{
	if (!mouse_inited)
		return;

	mouse_inited = 0;
}

void mouse_init()
{
	// Initialize queue
	if (mouse_inited)
		return;

	mouse_inited = 1;

	mouse_flags = 0;
	Mouse_x = gr_screen.max_w / 2;
	Mouse_y = gr_screen.max_h / 2;

	atexit( mouse_close );
}


// ----------------------------------------------------------------------------
// mouse_mark_button() is called asynchronously by the OS when a mouse button
// goes up or down.  The mouse button that is affected is passed via the 
// flags parameter.  
//
// parameters:   flags ==> mouse button pressed/released
//               set   ==> 1 - button is pressed
//                         0 - button is released

void mouse_mark_button( uint btn, int set)
{
	uint flags = 0;

	if ( !mouse_inited )
		return;

	switch (btn) {
		case SDL_BUTTON_LEFT:
			flags |= MOUSE_LEFT_BUTTON;
			break;

		case SDL_BUTTON_RIGHT:
			flags |= MOUSE_RIGHT_BUTTON;
			break;

		case SDL_BUTTON_MIDDLE:
			flags |= MOUSE_MIDDLE_BUTTON;
			break;

		default:
			return;
	}

	if ( !(mouse_flags & MOUSE_LEFT_BUTTON) )	{

		if ( (flags & MOUSE_LEFT_BUTTON) && (set == 1) ) {
			mouse_left_pressed++;
		}
	}
	else {
		if ( (flags & MOUSE_LEFT_BUTTON) && (set == 0) ){
			mouse_left_up++;
		}
	}

	if ( !(mouse_flags & MOUSE_RIGHT_BUTTON) )	{

		if ( (flags & MOUSE_RIGHT_BUTTON) && (set == 1) ){
			mouse_right_pressed++;
		}
	}
	else {
		if ( (flags & MOUSE_RIGHT_BUTTON) && (set == 0) ){
			mouse_right_up++;
		}
	}

	if ( !(mouse_flags & MOUSE_MIDDLE_BUTTON) )	{

		if ( (flags & MOUSE_MIDDLE_BUTTON) && (set == 1) ){
			mouse_middle_pressed++;
		}
	}
	else {
		if ( (flags & MOUSE_MIDDLE_BUTTON) && (set == 0) ){
			mouse_middle_up++;
		}
	}

	if ( set ){
		mouse_flags |= flags;
	} else {
		mouse_flags &= ~flags;
	}
}

void mouse_flush()
{
	if (!mouse_inited)
		return;

	mouse_eval_deltas();
	Mouse_dx = Mouse_dy = Mouse_dz = 0;
	Mouse_dx_inc = Mouse_dy_inc = 0;
	mouse_left_pressed = 0;
	mouse_right_pressed = 0;
	mouse_middle_pressed = 0;
	mouse_flags = 0;
}

int mouse_down_count(int n, int reset_count)
{
	int tmp = 0;

	if ( !mouse_inited )
		return 0;

	if ( (n < LOWEST_MOUSE_BUTTON) || (n > HIGHEST_MOUSE_BUTTON) )
		return 0;

	switch (n) {
		case MOUSE_LEFT_BUTTON:
			tmp = mouse_left_pressed;
			if ( reset_count ) {
				mouse_left_pressed = 0;
			}
			break;

		case MOUSE_RIGHT_BUTTON:
			tmp = mouse_right_pressed;
			if ( reset_count ) {
				mouse_right_pressed = 0;
			}
			break;

		case MOUSE_MIDDLE_BUTTON:
			tmp = mouse_middle_pressed;
			if ( reset_count ) {
				mouse_middle_pressed = 0;
			}
			break;
	} // end switch

	return tmp;
}

// mouse_up_count() returns the number of times button n has gone from down to up
// since the last call
//
// parameters:  n - button of mouse (see #define's in mouse.h)
//
int mouse_up_count(int n)
{
	int tmp = 0;

	if ( !mouse_inited )
		return 0;

	if ( (n < LOWEST_MOUSE_BUTTON) || (n > HIGHEST_MOUSE_BUTTON) )
		return 0;

	switch (n) {
		case MOUSE_LEFT_BUTTON:
			tmp = mouse_left_up;
			mouse_left_up = 0;
			break;

		case MOUSE_RIGHT_BUTTON:
			tmp = mouse_right_up;
			mouse_right_up = 0;
			break;

		case MOUSE_MIDDLE_BUTTON:
			tmp = mouse_middle_up;
			mouse_middle_up = 0;
			break;

		default:
			SDL_assert(0);	// can't happen
			break;
	} // end switch

	return tmp;
}

// returns 1 if mouse button btn is down, 0 otherwise

int mouse_down(int btn)
{
	int tmp;

	if ( !mouse_inited )
		return 0;

	if ( (btn < LOWEST_MOUSE_BUTTON) || (btn > HIGHEST_MOUSE_BUTTON) )
		return 0;

	if ( mouse_flags & btn )
		tmp = 1;
	else
		tmp = 0;

	return tmp;
}

// returns the fraction of time btn has been down since last call 
// (currently returns 1 if buttons is down, 0 otherwise)
//
float mouse_down_time(int btn)
{
	float tmp;

	if ( !mouse_inited )
		return 0.0f;

	if ( (btn < LOWEST_MOUSE_BUTTON) || (btn > HIGHEST_MOUSE_BUTTON) )
		return 0.0f;

	if ( mouse_flags & btn )
		tmp = 1.0f;
	else
		tmp = 0.0f;

	return tmp;
}

void mouse_get_delta(int *dx, int *dy, int *dz)
{
	if (dx)
		*dx = Mouse_dx;
	if (dy)
		*dy = Mouse_dy;
	if (dz)
		*dz = Mouse_dz;
}

// Forces the actual windows cursor to be at (x,y).  This may be independent of our tracked (x,y) mouse pos.
void mouse_force_pos(int x, int y)
{
	// only mess with windows's mouse if we are in control of it
	if ( !os_foreground() ) {
		return;
	}

	float x1 = (x / gr_screen.viewport_scale_factor_x) + gr_screen.viewport_offset_x;
	float y1 = (y / gr_screen.viewport_scale_factor_y) + gr_screen.viewport_offset_y;

	SDL_HideCursor();		// prevents cursor getting stuck as non-game one
	SDL_WarpMouseInWindow(os_get_window(), x1, y1);
	SDL_ShowCursor();

	Mouse_x = x;
	Mouse_y = y;
}

static bool Mouse_grabbed = false;

void mouse_grab(int grab)
{
	if (grab) {
		if ( !Mouse_grabbed ) {
            SDL_SetWindowMouseGrab(os_get_window(), true);
            SDL_SetWindowRelativeMouseMode(os_get_window(), true);

			Mouse_grabbed = true;
		}
	} else if (Mouse_grabbed) {
        SDL_SetWindowMouseGrab(os_get_window(), false);
        SDL_SetWindowRelativeMouseMode(os_get_window(), false);

		Mouse_grabbed = false;
	}
}

void mouse_eval_deltas()
{
	Mouse_dx = Mouse_dx_inc;
	Mouse_dy = Mouse_dy_inc;

	Mouse_dx_inc = Mouse_dy_inc = 0;

	// make sure mouse is bound to window if we're flying with it
	if (Keep_mouse_centered && !mouse_is_visible()) {
		mouse_grab(1);
	} else {
		mouse_grab(0);
	}
}

int mouse_get_pos(int *xpos, int *ypos)
{
	if ( !mouse_inited ) {
		if (xpos) {
			*xpos = 0;
		}

		if (ypos) {
			*ypos = 0;
		}

		return 0;
	}

	if (xpos){
		*xpos = Mouse_x;
	}

	if (ypos){
		*ypos = Mouse_y;
	}

	return mouse_flags;
}

void mouse_get_real_pos(int *mx, int *my)
{
	if (mx) {
		*mx = Mouse_x;
	}

	if (my) {
		*my = Mouse_y;
	}
}

void mouse_set_pos(int xpos, int ypos)
{
	if ((xpos != Mouse_x) || (ypos != Mouse_y)){
		// cap pos so we can't jump cursor out of window (when joy/gamepad controlled)
		CAP(xpos, 0, gr_screen.max_w-1);
		CAP(ypos, 0, gr_screen.max_h-1);

		mouse_force_pos(xpos, ypos);
	}
}

void mouse_update_pos(float x, float y, float dx, float dy)
{
	int x1 = fl2i((x - gr_screen.viewport_offset_x) * gr_screen.viewport_scale_factor_x);
	int y1 = fl2i((y - gr_screen.viewport_offset_y) * gr_screen.viewport_scale_factor_y);

	CAP(x1, 0, gr_screen.max_w-1);
	CAP(y1, 0, gr_screen.max_h-1);

	Mouse_x = x1;
	Mouse_y = y1;

	Mouse_dx_inc += fl2i(dx);
	Mouse_dy_inc += fl2i(dy);
}

// update mouse with position which is already scaled for max_w/max_h
void mouse_update_pos_scaled(int x, int y, int dx, int dy)
{
	CAP(x, 0, gr_screen.max_w-1);
	CAP(y, 0, gr_screen.max_h-1);

	Mouse_x = x;
	Mouse_y = y;

	Mouse_dx_inc += dx;
	Mouse_dy_inc += dy;
}
