/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Io/Key.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Include file for keyboard reading routines
 *
 * $Log$
 * Revision 1.9  2002/08/04 02:31:00  relnev
 * make numlock not overlap with pause
 *
 * Revision 1.8  2002/06/09 04:41:13  relnev
 * added copyright header
 *
 * Revision 1.7  2002/05/31 03:34:02  theoddone33
 * Fix Keyboard
 * Add titlebar
 *
 * Revision 1.6  2002/05/30 23:46:29  theoddone33
 * some minor key changes (not necessarily fixes)
 *
 * Revision 1.5  2002/05/30 22:02:30  theoddone33
 * More gl changes
 *
 * Revision 1.4  2002/05/30 16:50:24  theoddone33
 * Keyboard partially fixed
 *
 * Revision 1.3  2002/05/29 23:17:49  theoddone33
 * Non working text code and fixed keys
 *
 * Revision 1.2  2002/05/29 06:25:12  theoddone33
 * Keyboard input, mouse tracking now work
 *
 * Revision 1.1.1.1  2002/05/03 03:28:12  root
 * Initial import.
 *
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 26    5/19/98 12:28a Mike
 * Cheat stuff.
 * 
 * 25    5/18/98 11:01p Mike
 * Adding support for cheat system.
 * 
 * 24    5/01/98 4:23p Lawrance
 * Remap the scancode for the UK "\" key
 * 
 * 23    1/07/98 6:41p Lawrance
 * Pass message latency to the keyboard lib.
 * 
 * 22    11/14/97 4:33p Mike
 * Change Debug key to backquote (from F11).
 * Balance a ton of subsystems in ships.tbl.
 * Change "Heavy Laser" to "Disruptor".
 * 
 * 21    10/21/97 7:18p Hoffoss
 * Overhauled the key/joystick control structure and usage throughout the
 * entire FreeSpace code.  The whole system is very different now.
 * 
 * 20    9/13/97 9:30a Lawrance
 * added ability to block certain keys from the keyboard
 * 
 * 19    9/10/97 6:02p Hoffoss
 * Added code to check for key-pressed sexp operator in FreeSpace as part
 * of training mission stuff.
 * 
 * 18    4/15/97 3:47p Allender
 * moved type selection of list box items into actual UI code.  Made it
 * behave more like windows listboxes do
 * 
 * 17    2/17/97 5:18p John
 * Added a bunch of RCS headers to a bunch of old files that don't have
 * them.
 *
 * $NoKeywords: $
 */

#ifndef _KEY_H
#define _KEY_H

#include "pstypes.h"


bool key_pressed(int keycode);

// O/S level hooks...
void key_init();
void key_level_init();
void key_lost_focus();
void key_got_focus();
void key_mark(SDL_Scancode scancode, int state, ushort kmod, uint latency );
int key_getch();
void key_flush();

// Routines/data you can access:
float key_down_timef(int keycode);

bool key_is_ascii(int keycode);
int key_inkey();

int key_get_text_input();
void key_set_text_input(int ch);
void key_clear_text_input();

// global flag that will enable/disable the backspace key from stopping execution
//extern int Backspace_debug;

uint key_get_shift_status();
int key_down_count(int keycode);
int key_up_count(int keycode);
int key_checkch();
int key_check(int keycode);

// used to restrict keys that are read into keyboard buffer
void key_set_filter(int *filter_array, int num);
void key_clear_filter();

extern int Cheats_enabled;
extern int Key_normal_game;

#define KEY_SHIFTED     0x1000
#define KEY_ALTED       0x2000
#define KEY_CTRLED      0x4000
#define KEY_DEBUGGED	0x8000
#define KEY_DEBUGGED1	0x0800		//	Cheat bit in release version of game.
#define KEY_MASK		(SDLK_SCANCODE_MASK|0x01FF)

#define KEY_DEBUG_KEY	SDLK_BACKQUOTE		//	KEY_LAPOSTRO (shifted = tilde, near upper-left of keyboard)

#endif

