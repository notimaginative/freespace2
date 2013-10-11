/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Io/Key.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * <insert description of file here>
 *
 * $Log$
 * Revision 1.12  2004/06/11 01:14:12  tigital
 * OSX: switched to __APPLE__
 *
 * Revision 1.11  2003/05/18 03:57:08  taylor
 * do not swap German z and y keys if they are already swapped
 *
 * Revision 1.9  2003/01/30 19:55:01  relnev
 * add German keys (this is mostly a patch already sent in by someone else that hasn't made it into cvs yet) (Taylor Richards)
 *
 * Revision 1.8  2002/06/17 23:11:39  relnev
 * enable sdl key repeating.
 *
 * swap '/` keys.
 *
 * Revision 1.7  2002/06/09 04:41:21  relnev
 * added copyright header
 *
 * Revision 1.6  2002/06/05 04:03:32  relnev
 * finished cfilesystem.
 *
 * removed some old code.
 *
 * fixed mouse save off-by-one.
 *
 * sound cleanups.
 *
 * Revision 1.5  2002/05/31 03:34:02  theoddone33
 * Fix Keyboard
 * Add titlebar
 *
 * Revision 1.4  2002/05/30 23:46:29  theoddone33
 * some minor key changes (not necessarily fixes)
 *
 * Revision 1.3  2002/05/30 16:50:24  theoddone33
 * Keyboard partially fixed
 *
 * Revision 1.2  2002/05/29 23:17:50  theoddone33
 * Non working text code and fixed keys
 *
 * Revision 1.1.1.1  2002/05/03 03:28:09  root
 * Initial import.
 *
 * 
 * 6     10/29/99 6:10p Jefff
 * squashed the damned y/z german issues once and for all
 * 
 * 5     6/07/99 1:21p Dave
 * Fixed debug console scrolling problem. Thread related.
 * 
 * 4     6/02/99 6:18p Dave
 * Fixed TNT lockup problems! Wheeeee!
 * 
 * 3     11/05/98 4:18p Dave
 * First run nebula support. Beefed up localization a bit. Removed all
 * conditional compiles for foreign versions. Modified mission file
 * format.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 37    6/19/98 3:50p Lawrance
 * change GERMAN to GR_BUILD
 * 
 * 36    6/17/98 11:05a Lawrance
 * translate french and german keys
 * 
 * 35    6/12/98 4:49p Hoffoss
 * Added code to remap scancodes for german and french keyboards.
 * 
 * 34    5/20/98 12:10a Mike
 * Remove mprintfs.
 * 
 * 33    5/19/98 12:19p Mike
 * Cheat codes!
 * 
 * 32    5/19/98 12:28a Mike
 * Cheat stuff.
 * 
 * 31    5/18/98 11:01p Mike
 * Adding support for cheat system.
 * 
 * 30    5/11/98 12:09a Lawrance
 * Put in code to turn on/off NumLock key when running under 95
 * 
 * 29    5/01/98 4:23p Lawrance
 * Remap the scancode for the UK "\" key
 * 
 * 28    4/18/98 12:42p John
 * Added code to use DirectInput to read keyboard. Took out because it
 * didn't differentiate btwn Pause and Numlock and sometimes Ctrl.
 * 
 * 27    4/13/98 10:16a John
 * Switched gettime back to timer_get_milliseconds, which is now thread
 * safe.
 * 
 * 26    4/12/98 11:08p Lawrance
 * switch back to using gettime() in separate threads
 * 
 * 25    4/12/98 5:31p Lawrance
 * use timer_get_milliseconds() instead of gettime()
 * 
 * 24    3/25/98 8:08p John
 * Restructured software rendering into two modules; One for windowed
 * debug mode and one for DirectX fullscreen.   
 * 
 * 23    1/23/98 3:49p Mike
 * Fix bug in negative time-down due to latency.
 * 
 * 22    1/07/98 6:41p Lawrance
 * Pass message latency to the keyboard lib.
 * 
 * 21    11/17/97 10:42a John
 * On Debug+Backsp, cleared out keys so that it looks like nothing ever
 * happened, so they're not stuck down.
 * 
 * 20    11/14/97 4:33p Mike
 * Change Debug key to backquote (from F11).
 * Balance a ton of subsystems in ships.tbl.
 * Change "Heavy Laser" to "Disruptor".
 * 
 * 19    9/13/97 9:30a Lawrance
 * added ability to block certain keys from the keyboard
 * 
 * 18    9/10/97 6:02p Hoffoss
 * Added code to check for key-pressed sexp operator in FreeSpace as part
 * of training mission stuff.
 * 
 * 17    9/09/97 11:08a Sandeep
 * fixed warning level 4
 * 
 * 16    7/29/97 5:30p Lawrance
 * move gettime() to timer module
 * 
 * 15    4/22/97 10:56a John
 * fixed some resource leaks.
 * 
 * 14    2/03/97 4:23p Allender
 * use F11 as debug key now
 * 
 * 13    1/10/97 5:15p Mike
 * Moved ship-specific parameters from obj_subsystem to ship_subsys.
 * 
 * Added turret code to AI system.
 *
 * $NoKeywords: $
 */

#include <ctype.h>	// for toupper
#include "pstypes.h"
#include "key.h"
#include "fix.h"
#include "timer.h"
#include "osapi.h"
#include "localize.h"

#define KEY_BUFFER_SIZE 16

//-------- Variable accessed by outside functions ---------
ubyte				keyd_buffer_type;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
ubyte				keyd_repeat;
uint				keyd_last_pressed;
uint				keyd_last_released;
int				keyd_time_when_last_pressed;

static bool		keyd_pressed[SDL_NUM_SCANCODES];

typedef struct keyboard	{
	ushort			keybuffer[KEY_BUFFER_SIZE];
	uint				time_pressed[KEY_BUFFER_SIZE];
	uint				TimeKeyWentDown[SDL_NUM_SCANCODES];
	uint				TimeKeyHeldDown[SDL_NUM_SCANCODES];
	uint				TimeKeyDownChecked[SDL_NUM_SCANCODES];
	uint				NumDowns[SDL_NUM_SCANCODES];
	uint				NumUps[SDL_NUM_SCANCODES];
	int				down_check[SDL_NUM_SCANCODES];  // nonzero if has been pressed yet this mission
	uint				keyhead, keytail;
} keyboard;

keyboard key_data;

int key_inited = 0;

CRITICAL_SECTION key_lock;

//int Backspace_debug=1;	// global flag that will enable/disable the backspace key from stopping execution
								// This flag was created since the backspace key is also used to correct mistakes
								// when typing in your pilots callsign.  This global flag is checked before execution
								// is stopped.

// used to limit the keypresses that are accepted from the keyboard
#define MAX_FILTER_KEYS 64
int Num_filter_keys;
int Key_filter[MAX_FILTER_KEYS];

static int Key_numlock_was_on = 0;	// Flag to indicate whether NumLock is on at start

int Cheats_enabled = 0;
int Key_normal_game = 0;


bool key_pressed(int keycode)
{
	SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode & KEY_MASK);

	return keyd_pressed[scancode];
}

int key_numlock_is_on()
{
	const Uint8 *state;
	state = SDL_GetKeyboardState(NULL);
	if ( state[SDL_SCANCODE_NUMLOCKCLEAR] ) {
		return 1;
	}

	return 0;
}

void key_turn_off_numlock()
{
}

void key_turn_on_numlock()
{
}

//	Convert a BIOS scancode to ASCII.
//	If scancode >= 127, returns 255, meaning there is no corresponding ASCII code.
//	Uses ascii_table and shifted_ascii_table to translate scancode to ASCII.
int key_to_ascii(int keycode, bool force_up)
{
	int shifted;

	// bail on non-printable keycodes
	if (keycode & SDLK_SCANCODE_MASK) {
		return 255;
	}

	shifted = keycode & KEY_SHIFTED;
	keycode &= KEY_MASK;

	// this is definitely never come back to bite me in the ass
	if ( ((keycode >= SDLK_SPACE) && (keycode <= SDLK_AT))
			|| ((keycode >= SDLK_LEFTBRACKET) && (keycode <= SDLK_z)) )
	{
		if ( (keycode >= SDLK_a) && (shifted || force_up) ) {
			return toupper(keycode);
		} else {
			return keycode;
		}
	}

	return 255;
}

//	Flush the keyboard buffer.
//	Clear the keyboard array (keyd_pressed).
void key_flush()
{
	int i;
	uint CurTime;

	if ( !key_inited ) return;

	ENTER_CRITICAL_SECTION(&key_lock);	

	key_data.keyhead = key_data.keytail = 0;

	//Clear the keyboard buffer
	for (i=0; i<KEY_BUFFER_SIZE; i++ )	{
		key_data.keybuffer[i] = 0;
		key_data.time_pressed[i] = 0;
	}
	
	//Clear the keyboard array

	CurTime = timer_get_milliseconds();


	for (i=0; i<SDL_NUM_SCANCODES; i++ )	{
		keyd_pressed[i] = false;
		key_data.TimeKeyDownChecked[i] = CurTime;
		key_data.TimeKeyWentDown[i] = CurTime;
		key_data.TimeKeyHeldDown[i] = 0;
		key_data.NumDowns[i]=0;
		key_data.NumUps[i]=0;
	}

	LEAVE_CRITICAL_SECTION(&key_lock);	
}

//	A nifty function which performs the function:
//		n = (n+1) % KEY_BUFFER_SIZE
//	(assuming positive values of n).
int add_one( int n )
{
	n++;
	if ( n >= KEY_BUFFER_SIZE ) n=0;
	return n;
}

// Returns 1 if character waiting... 0 otherwise
int key_checkch()
{
	int is_one_waiting = 0;

	if ( !key_inited ) return 0;

	ENTER_CRITICAL_SECTION(&key_lock);	

	if (key_data.keytail != key_data.keyhead){
		is_one_waiting = 1;
	}

	LEAVE_CRITICAL_SECTION(&key_lock);		

	return is_one_waiting;
}

//	Return keycode if a key has been pressed,
//	else return 0.
//	Reads keys out of the key buffer and updates keyhead.
int key_inkey()
{
	SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
	int mod, keycode;

	if ( !key_inited )
		return 0;

	if (key_data.keytail != key_data.keyhead) {
		scancode = (SDL_Scancode)key_data.keybuffer[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	} else {
		return 0;
	}

	// need to strip key mod state for keycode lookup
	mod = (scancode & 0xf900);
	keycode = SDL_GetKeyFromScancode((SDL_Scancode)(scancode & KEY_MASK));

	return (keycode | mod);
}

// If not installed, uses BIOS and returns getch();
//	Else returns pending key (or waits for one if none waiting).
int key_getch()
{
	int dummy=0;
	int in;

	if ( !key_inited ) return 0;
	
	while (!key_checkch()){
		os_poll();

		dummy++;
	}
	in = key_inkey();

	return in;
}

//	Set global shift_status with modifier results (shift, ctrl, alt).
uint key_get_shift_status()
{
	unsigned int shift_status = 0;

	if ( !key_inited )
		return 0;

	SDL_Keymod kmod = SDL_GetModState();

	if (kmod & KMOD_SHIFT)
		shift_status |= KEY_SHIFTED;

	if (kmod & KMOD_ALT)
		shift_status |= KEY_ALTED;

	if (kmod & KMOD_CTRL)
		shift_status |= KEY_CTRLED;

#ifndef NDEBUG
	if (key_pressed(KEY_DEBUG_KEY))
		shift_status |= KEY_DEBUGGED;
#else
	if (key_pressed(KEY_DEBUG_KEY)) {
		mprintf(("Cheats_enabled = %i, Key_normal_game = %i\n", Cheats_enabled, Key_normal_game));
		if ((Cheats_enabled) && Key_normal_game) {
			mprintf(("Debug key\n"));
			shift_status |= KEY_DEBUGGED1;
		}
	}
#endif

	return shift_status;
}

//	Returns amount of time key (specified by "keycode") has been down since last call.
//	Returns float, unlike key_down_time() which returns a fix.
float key_down_timef(int keycode)
{
	uint time_down, time;
	uint delta_time;
	SDL_Scancode scancode;

	if ( !key_inited )
		return 0.0f;

	scancode = SDL_GetScancodeFromKey(keycode & KEY_MASK);

	if (scancode == SDL_SCANCODE_UNKNOWN)
		return 0.0f;

	ENTER_CRITICAL_SECTION(&key_lock);		

	time = timer_get_milliseconds();
	delta_time = time - key_data.TimeKeyDownChecked[scancode];
	key_data.TimeKeyDownChecked[scancode] = time;

	if ( delta_time <= 1 ) {
		key_data.TimeKeyWentDown[scancode] = time;
		if (keyd_pressed[scancode])	{
			LEAVE_CRITICAL_SECTION(&key_lock);		
			return 1.0f;
		} else	{
			LEAVE_CRITICAL_SECTION(&key_lock);		
			return 0.0f;
		}
	}

	if ( !keyd_pressed[scancode] )	{
		time_down = key_data.TimeKeyHeldDown[scancode];
		key_data.TimeKeyHeldDown[scancode] = 0;
	} else	{
		time_down =  time - key_data.TimeKeyWentDown[scancode];
		key_data.TimeKeyWentDown[scancode] = time;
	}

	LEAVE_CRITICAL_SECTION(&key_lock);		

	return i2fl(time_down) / i2fl(delta_time);
}

// Returns number of times key has went from up to down since last call.
int key_down_count(int keycode)
{
	int n;
	SDL_Scancode scancode;

	if ( !key_inited )
		return 0;

	scancode = SDL_GetScancodeFromKey(keycode & KEY_MASK);

	if (scancode == SDL_SCANCODE_UNKNOWN)
		return 0;

	ENTER_CRITICAL_SECTION(&key_lock);		

	n = key_data.NumDowns[scancode];
	key_data.NumDowns[scancode] = 0;

	LEAVE_CRITICAL_SECTION(&key_lock);		

	return n;
}


// Returns number of times key has went from down to up since last call.
int key_up_count(int keycode)
{
	int n;
	SDL_Scancode scancode;

	if ( !key_inited )
		return 0;

	scancode = SDL_GetScancodeFromKey(keycode & KEY_MASK);

	if (scancode == SDL_SCANCODE_UNKNOWN)
		return 0;

	ENTER_CRITICAL_SECTION(&key_lock);		

	n = key_data.NumUps[scancode];
	key_data.NumUps[scancode] = 0;

	LEAVE_CRITICAL_SECTION(&key_lock);		

	return n;
}

int key_check(int keycode)
{
	SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode & KEY_MASK);

	return key_data.down_check[scancode];
}

//	Add a key up or down code to the key buffer.  state=1 -> down, state=0 -> up
// latency => time difference in ms between when key was actually pressed and now
//void key_mark( uint code, int state )
void key_mark(SDL_Scancode scancode, int state, ushort kmod, uint latency )
{
	uint breakbit, temp, event_time;
	ushort keycode;

	if ( !key_inited ) return;

	ENTER_CRITICAL_SECTION(&key_lock);		

	Assert( scancode < SDL_NUM_SCANCODES );

	event_time = timer_get_milliseconds() - latency;
	// event_time = timeGetTime() - latency;

	breakbit = !state;
	
	if (breakbit)	{
		// Key going up
		keyd_last_released = scancode;
		keyd_pressed[scancode] = false;
		key_data.NumUps[scancode]++;

		if (event_time < key_data.TimeKeyWentDown[scancode])
			key_data.TimeKeyHeldDown[scancode] = 0;
		else
			key_data.TimeKeyHeldDown[scancode] += event_time - key_data.TimeKeyWentDown[scancode];
	} else {
		// Key going down
		keyd_last_pressed = scancode;
		keyd_time_when_last_pressed = event_time;
		if (!keyd_pressed[scancode])	{
			// First time down
			key_data.TimeKeyWentDown[scancode] = event_time;
			keyd_pressed[scancode] = true;
			key_data.NumDowns[scancode]++;
			key_data.down_check[scancode]++;

//			mprintf(( "Scancode = %x\n", scancode ));

//			if ( scancode == KEY_BREAK )
//				Int3();
		} 

		keycode = (unsigned short)scancode;

		if (kmod & KMOD_SHIFT)
			keycode |= KEY_SHIFTED;

		if (kmod & KMOD_ALT)
			keycode |= KEY_ALTED;

		if (kmod & KMOD_CTRL)
			keycode |= KEY_CTRLED;

#ifndef NDEBUG
		if ( key_pressed(KEY_DEBUG_KEY) )
			keycode |= KEY_DEBUGGED;

//			if ( keycode == (KEY_BACKSP + KEY_DEBUGGED) )	{
//				keycode = 0;
//				keyd_pressed[KEY_DEBUG_KEY] = 0;
//				keyd_pressed[KEY_BACKSP] = 0;
//				Int3();
//			}
#else
		if ( keyd_pressed(KEY_DEBUG_KEY) ) {
			mprintf(("Cheats_enabled = %i, Key_normal_game = %i\n", Cheats_enabled, Key_normal_game));
			if (Cheats_enabled && Key_normal_game) {
				keycode |= KEY_DEBUGGED1;
			}
		}

#endif

		if ( keycode )	{
			temp = key_data.keytail+1;
			if ( temp >= KEY_BUFFER_SIZE ) temp=0;

			if (temp!=key_data.keyhead)	{
				int i, accept_key = 1;
				// Num_filter_keys will only be non-zero when a key filter has
				// been explicity set up via key_set_filter()
				for ( i = 0; i < Num_filter_keys; i++ ) {
					accept_key = 0;
					if ( Key_filter[i] == keycode ) {
						accept_key = 1;
						break;
					}
				}

				if ( accept_key ) {
					key_data.keybuffer[key_data.keytail] = keycode;
					key_data.time_pressed[key_data.keytail] = keyd_time_when_last_pressed;
					key_data.keytail = temp;
				}
			}
		}
	}

	LEAVE_CRITICAL_SECTION(&key_lock);		
}

void key_close()
{
	if ( !key_inited )
		return;

	if ( Key_numlock_was_on ) {
		key_turn_on_numlock();
		Key_numlock_was_on = 0;
	}

	key_inited = 0;
}

void key_init()
{
	// Initialize queue
	if (key_inited)
		return;

	key_inited = 1;

	keyd_time_when_last_pressed = timer_get_milliseconds();
	keyd_buffer_type = 1;

	// Clear the keyboard array
	key_flush();

	// Clear key filter
	key_clear_filter();

	atexit(key_close);
}

void key_level_init()
{
	int i;

	for (i=0; i<SDL_NUM_SCANCODES; i++)
		key_data.down_check[i] = 0;
}

void key_lost_focus()
{
	if ( !key_inited )
		return;

	key_flush();	
}

void key_got_focus()
{
	if ( !key_inited )
		return;
	
	key_flush();	
}

// Restricts the keys that are accepted from the keyboard
//
//	filter_array	=>		array of keys to act as a filter
//	num				=>		number of keys in filter_array
//
void key_set_filter(int *filter_array, int num)
{
	int i;

	if ( num >= MAX_FILTER_KEYS ) {
		Int3();
		num = MAX_FILTER_KEYS;
	}

	Num_filter_keys = num;

	for ( i = 0; i < num; i++ ) {
		Key_filter[i] = filter_array[i];
	}
}

// Clear the key filter, so all keypresses are accepted from keyboard 
//
void key_clear_filter()
{
	int i;

	Num_filter_keys = 0;
	for ( i = 0; i < MAX_FILTER_KEYS; i++ ) {
		Key_filter[i] = -1;
	}
}
