/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Popup/Popup.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Code for displaying pop-up dialog boxes
 *
 * $Log$
 * Revision 1.4  2003/05/25 02:30:43  taylor
 * Freespace 1 support
 *
 * Revision 1.3  2002/06/09 04:41:25  relnev
 * added copyright header
 *
 * Revision 1.2  2002/05/07 03:16:51  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:10  root
 * Initial import.
 *
 * 
 * 12    10/14/99 2:00p Jefff
 * added include for os_poll to fix build error
 * 
 * 11    10/14/99 10:18a Daveb
 * Fixed incorrect CD checking problem on standalone server.
 * 
 * 10    8/16/99 9:50a Jefff
 * added mouseover webcursor options to user-defined popup buttons.
 * 
 * 9     8/11/99 5:47p Jefff
 * fixed button bitmap loading
 * 
 * 8     8/04/99 10:53a Dave
 * Added title to the user tips popup.
 * 
 * 7     8/02/99 9:13p Dave
 * Added popup tips.
 * 
 * 6     6/18/99 5:16p Dave
 * Added real beam weapon lighting. Fixed beam weapon sounds. Added MOTD
 * dialog to PXO screen.
 * 
 * 5     6/01/99 3:52p Dave
 * View footage screen. Fixed xstrings to not display the & symbol. Popup,
 * dead popup, pxo find player popup, pxo private room popup.
 * 
 * 4     2/11/99 3:08p Dave
 * PXO refresh button. Very preliminary squad war support.
 * 
 * 3     10/13/98 9:29a Dave
 * Started neatening up freespace.h. Many variables renamed and
 * reorganized. Added AlphaColors.[h,cpp]
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 36    5/11/98 11:39p Dave
 * Stuff.
 * 
 * 35    3/22/98 10:59p Allender
 * don't stop time in multiplayer when "in mission"
 * 
 * 34    3/17/98 12:41a Lawrance
 * Support \n's between title and body
 * 
 * 33    3/12/98 4:11p Dave
 * AL: Check that max popup lines isn't exceeded
 * 
 * 32    2/22/98 12:19p John
 * Externalized some strings
 * 
 * 31    2/10/98 3:26p Hoffoss
 * Don't check for 'dead key set' unless in the mission.
 * 
 * 30    2/05/98 11:21p Lawrance
 * move death popup to below letterbox view
 * 
 * 29    2/05/98 11:09a Dave
 * Fixed an ingame join bug. Fixed a read-only file problem with
 * multiplauer file xfer.
 * 
 * 28    2/03/98 11:52p Lawrance
 * Don't highlight default choice if there is only one.
 * 
 * 27    2/03/98 8:18p Dave
 * More MT stats transfer stuff
 * 
 * 26    1/30/98 3:05p Lawrance
 * reposition text on death popup
 * 
 * 25    1/29/98 6:55p Lawrance
 * Integrate new art for the death popup
 * 
 * 24    1/28/98 6:24p Dave
 * Made standalone use ~8 megs less memory. Fixed multiplayer submenu
 * sequencing problem.
 * 
 * 23    1/27/98 5:52p Lawrance
 * support new "tiny" popups
 * 
 * 22    1/27/98 5:01p Dave
 * Put in support for leaving multiplayer games in the pause state. Fixed
 * a popup bug which freed saved screens incorrectly. Reworked scoring
 * kill and assist evaluation.
 * 
 * 21    1/27/98 3:25p Hoffoss
 * Made popups use the correct button icons for default positive and
 * negative buttons.
 * 
 * 20    1/26/98 6:28p Lawrance
 * Use '&' meta char for underlining, change how keyboard usage works so
 * fits better with mouse usage.
 * 
 * 19    1/23/98 12:01a Lawrance
 * Fix bug when in single-choice popups.
 * 
 * 18    1/22/98 10:45p Lawrance
 * Implment default selections and arrow key navigation.
 * 
 * 17    1/20/98 5:52p Lawrance
 * center popup text in X and Y directions.  Support single affirmative
 * icon special placement.
 * 
 * 16    1/19/98 11:37p Lawrance
 * Fixing Optimization build warnings
 * 
 * 15    1/17/98 10:04p Lawrance
 * fix errors in popup comments
 * 
 * 14    1/15/98 2:52p Lawrance
 * Fix problem with reading bogus buttons.
 * 
 * 13    1/14/98 6:55p Dave
 * Fixed a slew of multiplayer bugs. Made certain important popups ignore
 * the escape character.
 * 
 * 12    1/14/98 6:42p Hoffoss
 * Massive changes to UI code.  A lot cleaner and better now.  Did all
 * this to get the new UI_DOT_SLIDER to work properly, which the old code
 * wasn't flexible enough to handle.
 * 
 * 11    1/14/98 12:23p Lawrance
 * Support for three choice popups.
 * 
 * 10    1/13/98 5:37p Dave
 * Reworked a lot of standalone interface code. Put in single and
 * multiplayer popups for death sequence. Solidified multiplayer kick
 * code.
 * 
 * 9     1/13/98 4:06p Lawrance
 * Add underline char for shortcuts.
 * 
 * 8     1/11/98 11:14p Lawrance
 * Don't grey out background when popup is active.
 * 
 * 7     1/08/98 10:32a Lawrance
 * Grey out screen when a popup appears.
 * 
 * 6     1/02/98 9:08p Lawrance
 * Integrated art for popups, expanded options.
 * 
 * 5     12/30/97 4:30p Sandeep
 * Added conditional popups
 * 
 * 4     12/26/97 10:01p Lawrance
 * Allow keyboard shortcuts for popup buttons
 * 
 * 3     12/24/97 9:49p Lawrance
 * ensure mouse gets drawn when popup menu is up
 * 
 * 2     12/24/97 8:54p Lawrance
 * Integrating new popup code
 * 
 * 1     12/24/97 3:51p Lawrance
 *
 * $NoKeywords: $
 */

#include <stdarg.h>
#include <string.h>
#include "freespace.h"
#include "gamesequence.h"
#include "key.h"
#include "mouse.h"
#include "ui.h"
#include "parselo.h"
#include "popup.h"
#include "gamesnd.h"
#include "animplay.h"
#include "contexthelp.h"
#include "keycontrol.h"
#include "player.h"
#include "font.h"
#include "alphacolors.h"
#include "osapi.h"

#define POPUP_MAX_CHOICES			3					// max number of buttons allowed on popup

#define POPUP_MAX_LINE_CHARS		256				// max chars of msg text allowed per line
#define POPUP_MAX_LINES				30					// max lines of text allowed
#define POPUP_MAX_CHARS				2048				// total max chars 
#define POPUP_INPUT_MAX_CHARS		255				// max length of input string

#define POPUP_NOCHANGE				100
#define POPUP_ABORT					101

int Popup_max_display[GR_NUM_RESOLUTIONS] = {
	11,
	19
};

#ifndef MAKE_FS1  // to avoid trying to find the interface tool
const char *Popup_slider_name[GR_NUM_RESOLUTIONS] = {
	"slider",
	"2_slider"
};
#endif

int Popup_slider_coords[GR_NUM_RESOLUTIONS][4] = {
	{ // GR_640
		121, 109, 15, 105
	},
	{ // GR_1024
		195, 177, 30, 173
	}
};

////////////////////////////////////////////////////////////////
// Internal popup flags
////////////////////////////////////////////////////////////////
#define PF_INPUT						(1<<0)			// contents of the box is an inputbox and a caption

////////////////////////////////////////////////////////////////
// Popup data struct
////////////////////////////////////////////////////////////////
typedef struct popup_info
{
	int	nchoices;														// number of choices user can pick
	char	*button_text[POPUP_MAX_CHOICES];							// button text
	int	keypress[POPUP_MAX_CHOICES];								// button keypress shortcut
	int	shortcut_index[POPUP_MAX_CHOICES];						// what char should be underlines for shortcut
	char	raw_text[POPUP_MAX_CHARS];									// the unbroken text for the popup
	char	title[POPUP_MAX_LINE_CHARS];								// title text for popup (optional)
	int	nlines;
	char	msg_lines[POPUP_MAX_LINES][POPUP_MAX_LINE_CHARS];	// lines of text in popup
	char	input_text[POPUP_INPUT_MAX_CHARS];						// input box text (if this is an inputbox popup)
	int	max_input_text_len;
	int	web_cursor_flag[POPUP_MAX_CHOICES];						// flag for using web cursor over button
	void (*callback)(int);										// callback to call on user choice (optional)
	int flags;													// popup flags (PF_*)
	int choice;													// choice user made
	int screen_id;												// background screen id
	int (*condition)();											// test condition (optional)
} popup_info;

////////////////////////////////////////////////////////////////
// UI Data and constants
////////////////////////////////////////////////////////////////
UI_WINDOW	Popup_window;
UI_BUTTON	Popup_buttons[POPUP_MAX_CHOICES];			// actual lit buttons
UI_BUTTON	Popup_button_regions[POPUP_MAX_CHOICES];	// fake buttons used for mouse detection over text
UI_INPUTBOX	Popup_input;										// input box for the popup
#ifndef MAKE_FS1
UI_SLIDER2	Popup_slider;										// if we have more text in the popup than can be displayed at once
#endif

// extents for message portion of popup
int Popup_text_coords[GR_NUM_RESOLUTIONS][4] = {
	{ // GR_640
#ifdef MAKE_FS1
		154, 144, 331, 105
#else
		137, 106, 343, 113
#endif
	},
	{ // GR_1024
		219, 169, 558, 182
	}
};

// input popup info
// offset from the first y text line value to place the centered input box
int Popup_input_y_offset[GR_NUM_RESOLUTIONS] = {
	40, 
	40
};

// offset from the first y text line value to start drawing text
int Popup_input_text_y_offset[GR_NUM_RESOLUTIONS] = {
	30,
	30
};

typedef struct popup_background
{
	const char	*filename;							// filename for background
	int	coords[2];							// coords to draw background at
} popup_background;

////////////////////////////////////////////////////////////////
// Module globals
////////////////////////////////////////////////////////////////
static int Popup_is_active=0;
static int Popup_should_die=0;			// popup should quit during the next iteration of its loop

static popup_info Popup_info;

static int Title_coords[GR_NUM_RESOLUTIONS][5] =
{
	{ // GR_640
#ifdef MAKE_FS1
		154,	// x-left
		144,	// y-top
		331,	// width
		26,		// height
		320		// center
#else
		137,		// x-left
		106,		// y-top
		343,		// width
		26,		//	height
		308		// center
#endif
	},
	{ // GR_1024
		220,		// x-left
		169,		// y-top
		553,		// width
		26,		//	height
		496		// center
	}
};

static int Button_regions[GR_NUM_RESOLUTIONS][3][4] = {
	{ // GR_640		
#ifdef MAKE_FS1
		{464, 269, 505, 290},		// upper right pixel of text, lower right pixel of button
		{464, 297, 505, 320},
		{464, 323, 505, 342}
#else
		{464, 232, 510, 250},		// upper right pixel of text, lower right pixel of button
		{464, 262, 510, 279},		
		{464, 292, 510, 308}		
#endif
	},
	{ // GR_1024
		{752, 373, 806, 406},		// upper right pixel of text, lower right pixel of button
		{752, 421, 806, 461},		
		{752, 468, 806, 506}		
	}
};

static int Button_coords[GR_NUM_RESOLUTIONS][3][2] =
{
	{ // GR_640
#ifdef MAKE_FS1
		{474, 257},		// upper left pixel
		{474, 291},
		{474, 318}		
#else
		{474, 224},		// upper left pixel
		{474, 258},
		{474, 286}		
#endif
	},
	{ // GR_1024
		{758, 358},		// upper left pixel
		{758, 413},
		{758, 458},		
	}
};

static popup_background Popup_background[GR_NUM_RESOLUTIONS][4] = 
{
	{ // GR_640
#ifdef MAKE_FS1
        { "Pop2a",		{ 131, 122} },
        { "Pop2a",		{ 131, 122 } },
        { "Pop3",		{ 131, 122 } },
#else
		{ "Pop2",		{ 129, 99 } },
		{ "Pop2",		{ 129, 99 } },
		{ "Pop3",		{ 129, 99 } },		
#endif
	},
	{ // GR_1024
		{ "2_Pop2",		{ 206, 158 } },
		{ "2_Pop2",		{ 206, 158 } },
		{ "2_Pop3",		{ 206, 158 } },		
	}
};

#define BUTTON_NEGATIVE				0
#define BUTTON_POSITIVE				1
#define BUTTON_GENERIC_FIRST		2
#define BUTTON_GENERIC_SECOND		3
#define BUTTON_GENERIC_THIRD		4
static const char *Popup_button_filenames[GR_NUM_RESOLUTIONS][2][5] = 
{
	{ // GR_640
#ifdef MAKE_FS1
		{"Pop2a_00",			// negative
		"Pop2a_01",				// positive
		"Pop2a_02",				// first generic
		"Pop2a_03",				// second generic
		"Pop2a_04"},			// third generic

		{"Pop2a_00",			// negative
		"Pop2a_01",				// positive
		"PopD_00",				// first generic
		"PopD_01",				// second generic
		"PopD_02"},				// third generic
#else
		{"Pop_00",				// negative
		"Pop_01",				// positive
		"Pop_02",				// first generic
		"Pop_03",				// second generic
		"Pop_04"},				// third generic

		{"Pop_00",				// negative
		"Pop_01",				// positive
		"PopD_00",				// first generic
		"PopD_01",				// second generic
		"PopD_02"},				// third generic
#endif
	},
	{ // GR_1024
		{"2_Pop_00",			// negative
		"2_Pop_01",				// positive
		"2_Pop_02",				// first generic
		"2_Pop_03",				// second generic
		"2_Pop_04"},			// third generic

		{"2_Pop_00",			// negative
		"2_Pop_01",				// positive
		"2_PopD_00",			// first generic
		"2_PopD_01",			// second generic
		"2_PopD_02"},			// third generic
	}
};

int Popup_running_state;
int Popup_default_choice;	// which choice is highlighted (ie what gets choosen when enter is pressed)

// see if any popup buttons have been pressed
// exit: POPUP_NOCHANGE		=> no buttons pressed
//			>=0					=>	button index that was pressed
int popup_check_buttons(popup_info *pi)
{
	int			i;
	UI_BUTTON	*b;

	for ( i = 0; i < pi->nchoices; i++ ) {
		b = &Popup_button_regions[i];
		if ( b->pressed() ) {
			return i;
		}

		b = &Popup_buttons[i];
		if ( b->pressed() ) {
			return i;
		}
	}

	return POPUP_NOCHANGE;
}

// maybe play a sound when key up/down is pressed to switch default choice
void popup_play_default_change_sound(popup_info *pi)
{
	if ( pi->nchoices > 1 ) {
		int i, mouse_over=0;
		UI_BUTTON *br, *b;

		// only play if mouse not currently highlighting a choice

		for ( i = 0; i < pi->nchoices; i++ ) {
			br = &Popup_button_regions[i];
			b = &Popup_buttons[i];
			if ( br->button_down() ) {
				mouse_over=1;
				break;
			}

			if ( br->button_hilighted() && !b->button_down() ) {
				mouse_over=1;
				break;
			}

			if ( b->button_hilighted() ) {
				mouse_over=1;
			}
		}

		if (!mouse_over) {
			gamesnd_play_iface(SND_USER_SELECT);
		}
	}
}

// do any key processing here
// input:	pi					=>	data about the popup
//				k					=> key that was pressed
//
// exit: 0 .. nchoices-1	=> choice selected through keypress
//			POPUP_ABORT			=>	abort the popup
//			POPUP_NOCHANGE		=> nothing happenned
int popup_process_keys(popup_info *pi, int k)
{
	int i, masked_k;

	if ( k <= 0 ) {
		return POPUP_NOCHANGE;
	}

	for ( i = 0; i < pi->nchoices; i++ ) {
		if ( pi->keypress[i] == key_get_text_input() ) {
			Popup_default_choice=i;
			Popup_buttons[i].press_button();
			return i;
		}
	}
	
	switch(k) {

	case SDLK_RETURN:
		// select the current default choice
		return Popup_default_choice;
		break;

	case SDLK_ESCAPE:
		// only process the escape key if this flag is not set
		if(!(pi->flags & PF_IGNORE_ESC)){
			return POPUP_ABORT;
		}
		break;

	case SDLK_DOWN:
	case SDLK_KP_2:
	case SDLK_TAB:
		popup_play_default_change_sound(pi);
		Popup_default_choice++;
		if ( Popup_default_choice >= pi->nchoices ) {
			Popup_default_choice=0;
		}
		break;

	case SDLK_UP:
	case SDLK_KP_8:
	case KEY_SHIFTED+SDLK_TAB:
		popup_play_default_change_sound(pi);
		Popup_default_choice--;
		if ( Popup_default_choice < 0 ) {
			Popup_default_choice=pi->nchoices-1;
		}
		break;

	default:
		break;
	} // end switch


	masked_k = k & ~KEY_CTRLED;	// take out CTRL modifier only
	if ( /* (PF_ALLOW_DEAD_KEYS) && */ (Game_mode & GM_IN_MISSION) ) {
		process_set_of_keys(masked_k, Dead_key_set_size, Dead_key_set);
		button_info_do(&Player->bi);	// call functions based on status of button_info bit vectors
	}

	return POPUP_NOCHANGE;
}

// Split off the title and break up the body lines
void popup_split_lines(popup_info *pi)
{
	int	nlines, i, body_offset = 0;
	int	n_chars[POPUP_MAX_LINES];
	char	*p_str[POPUP_MAX_LINES];
	int len;

	gr_set_font(FONT1);
	n_chars[0]=0;

	nlines = split_str(pi->raw_text, 1000, n_chars, p_str, POPUP_MAX_LINES);
	SDL_assert(nlines >= 0 && nlines <= POPUP_MAX_LINES );

	if ( pi->flags & (PF_TITLE | PF_TITLE_BIG) ) {
		// get first line out
		len = SDL_min(n_chars[0] + 1, POPUP_MAX_LINE_CHARS);
		SDL_strlcpy(pi->title, p_str[0], len);
		body_offset = 1;
	}

	if ( pi->flags & PF_BODY_BIG ) {
		gr_set_font(FONT2);
	}

	nlines = split_str(pi->raw_text, Popup_text_coords[gr_screen.res][2], n_chars, p_str, POPUP_MAX_LINES);
	SDL_assert(nlines >= 0 && nlines <= POPUP_MAX_LINES );

	pi->nlines = nlines - body_offset;

	for ( i = 0; i < pi->nlines; i++ ) {
		SDL_assert(n_chars[i+body_offset] < POPUP_MAX_LINE_CHARS);
		len = SDL_min(n_chars[i+body_offset] + 1, POPUP_MAX_LINE_CHARS);
		SDL_strlcpy(pi->msg_lines[i], p_str[i+body_offset], len);
	}

	gr_set_font(FONT1);
}

// figure out what filename to use for the button icon
const char *popup_get_button_filename(popup_info *pi, int i)
{
	const char *fname = NULL;
	int is_tiny=0;	

	// check for special button texts and if found, use specialized buttons for them.
	if ((!SDL_strcasecmp(pi->button_text[i], POPUP_OK + 1) || !SDL_strcasecmp(pi->button_text[i], POPUP_YES + 1)) && !(pi->flags & PF_NO_SPECIAL_BUTTONS)){
		return Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_POSITIVE];
	}

	if ((!SDL_strcasecmp(pi->button_text[i], POPUP_CANCEL + 1) || !SDL_strcasecmp(pi->button_text[i], POPUP_NO + 1)) && !(pi->flags & PF_NO_SPECIAL_BUTTONS)){
		return Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_NEGATIVE];
	}

	switch (pi->nchoices) {
	case 0:
		fname = "";
		break;
	case 1:
		if ( (pi->flags & PF_USE_AFFIRMATIVE_ICON) && !(pi->flags & PF_NO_SPECIAL_BUTTONS) ) {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_POSITIVE];
		} else if ( pi->flags & PF_USE_NEGATIVE_ICON && !(pi->flags & PF_NO_SPECIAL_BUTTONS) ) {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_NEGATIVE];
		} else {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_GENERIC_FIRST];
		}
		break;

	case 2:
		if ( pi->flags & PF_USE_NEGATIVE_ICON && i==0 ) {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_NEGATIVE];
			break;
		} 

		if ( pi->flags & PF_USE_AFFIRMATIVE_ICON && i==1 ) {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_POSITIVE];
			break;
		} 

		if ( i == 0 ) {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_GENERIC_FIRST];
		} else {
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_GENERIC_SECOND];
		}

		break;

	case 3:
		switch(i) {
		case 0:
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_GENERIC_FIRST];
			break;
		case 1:
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_GENERIC_SECOND];
			break;
		case 2:
			fname = Popup_button_filenames[gr_screen.res][is_tiny][BUTTON_GENERIC_THIRD];
			break;
		}
		break;
	}

	return fname;
}

// bogus handling function for slider (we don't need it to do anything)
void popup_slider_bogus()
{
}

// init the Popup window
int popup_init(popup_info *pi)
{
	int					i;
	UI_BUTTON			*b;
	popup_background	*pbg;
	const char			*fname;

	pi->screen_id = -1;
	pi->choice = POPUP_NOCHANGE;

	if(pi->nchoices == 0){
		pbg = &Popup_background[gr_screen.res][0];
	} else {
		pbg = &Popup_background[gr_screen.res][pi->nchoices-1];
	}

	// anytime in single player, and multiplayer, not in mission, go ahead and stop time
	if ( (Game_mode & GM_NORMAL) || ((Game_mode & GM_MULTIPLAYER) && !(Game_mode & GM_IN_MISSION)) ){
		game_stop_time();
	}

	// create base window
	Popup_window.create(pbg->coords[0], pbg->coords[1], Popup_text_coords[gr_screen.res][2]+100, Popup_text_coords[gr_screen.res][3]+50, 0);
	Popup_window.set_foreground_bmap(pbg->filename);

	// create buttons
	for (i=0; i<pi->nchoices; i++) {
		b = &Popup_buttons[i];
		// accommodate single-choice positive icon being positioned differently
		if ( (pi->nchoices == 1) && (pi->flags&PF_USE_AFFIRMATIVE_ICON) ) {
			b->create(&Popup_window, "", Button_coords[gr_screen.res][i+1][0], Button_coords[gr_screen.res][i+1][1], 30, 25, 0, 1);
		} else {
			b->create(&Popup_window, "", Button_coords[gr_screen.res][i][0], Button_coords[gr_screen.res][i][1], 30, 25, 0, 1);
		}

		fname = popup_get_button_filename(pi, i);
		b->set_bmaps(fname, 3, 0);
		b->set_highlight_action(common_play_highlight_sound);
		if ( pi->keypress[i] >= 0 ) {
			b->set_hotkey(pi->keypress[i]);
		}

		// create invisible buttons to detect mouse presses... can't use mask since button region is dynamically sized
		int lx, w, h;

		gr_get_string_size(&w, &h, pi->button_text[i]);
		lx = Button_regions[gr_screen.res][i][0] - w;
		b = &Popup_button_regions[i];	

		// accommodate single-choice positive icon being positioned differently
		if ( (pi->nchoices == 1) && (pi->flags&PF_USE_AFFIRMATIVE_ICON) ) {
			b->create(&Popup_window, "", lx, Button_regions[gr_screen.res][i+1][1], Button_regions[gr_screen.res][i+1][2]-lx, Button_regions[gr_screen.res][i+1][3]-Button_regions[gr_screen.res][i+1][1], 0, 1);
		} else {
			b->create(&Popup_window, "", lx, Button_regions[gr_screen.res][i][1], Button_regions[gr_screen.res][i][2]-lx, Button_regions[gr_screen.res][i][3]-Button_regions[gr_screen.res][i][1], 0, 1);
		}

		b->hide();
	}

	// webcursor setup
	if (Web_cursor_bitmap >= 0) {
		if (pi->flags & PF_WEB_CURSOR_1) {
			Popup_buttons[1].set_custom_cursor_bmap(Web_cursor_bitmap);
		}
		if (pi->flags & PF_WEB_CURSOR_2) {
			Popup_buttons[2].set_custom_cursor_bmap(Web_cursor_bitmap);
		}
	}

	// if this is an input popup, create and center the popup
	if(pi->flags & PF_INPUT){
		Popup_input.create(&Popup_window, Popup_text_coords[gr_screen.res][0], pbg->coords[1] + Popup_input_y_offset[gr_screen.res], Popup_text_coords[gr_screen.res][2], pi->max_input_text_len, "", UI_INPUTBOX_FLAG_INVIS | UI_INPUTBOX_FLAG_ESC_CLR | UI_INPUTBOX_FLAG_ESC_FOC | UI_INPUTBOX_FLAG_KEYTHRU | UI_INPUTBOX_FLAG_TEXT_CEN);
		Popup_input.set_focus();
	}	
	
	Popup_default_choice=0;
	Popup_should_die = 0;

	if (pi->flags & PF_RUN_STATE) {
		Popup_running_state = 1;
	} else {
		Popup_running_state = 0;
	}

	popup_split_lines(pi);

#ifndef MAKE_FS1
	// create the popup slider (which we may not need to use
	Popup_slider.create(&Popup_window, Popup_slider_coords[gr_screen.res][0], Popup_slider_coords[gr_screen.res][1], Popup_slider_coords[gr_screen.res][2], Popup_slider_coords[gr_screen.res][3], pi->nlines > Popup_max_display[gr_screen.res] ? pi->nlines - Popup_max_display[gr_screen.res] : 0,
								Popup_slider_name[gr_screen.res], popup_slider_bogus, popup_slider_bogus, NULL);
#endif

	return 0;
}

// called when a popup goes away
void popup_close(popup_info *pi)
{
	int i;
	
	gamesnd_play_iface(SND_POPUP_DISAPPEAR); 	// play sound when popup disappears

	for (i=0; i<pi->nchoices; i++ )	{
		if ( pi->button_text[i] != NULL ) {
			free(pi->button_text[i]);
			pi->button_text[i] = NULL;
		}
	}

	if(pi->screen_id >= 0){
		gr_free_screen(pi->screen_id);
		pi->screen_id = -1;
	}

	pi->callback = NULL;
	pi->condition = NULL;

	Popup_window.destroy();
	anim_ignore_next_frametime();					// to avoid skips in animation since next frametime is saturated
	game_flush();

	Popup_is_active = 0;
	Popup_running_state = 0;

	// anytime in single player, and multiplayer, not in mission, go ahead and stop time
	if ( (Game_mode & GM_NORMAL) || ((Game_mode & GM_MULTIPLAYER) && !(Game_mode & GM_IN_MISSION)) )
		game_start_time();
}

void popup_done()
{
	if ( !Popup_is_active ) {
		return;
	}

	popup_info *pi = &Popup_info;

	popup_close(pi);
}

// set the popup text color
void popup_set_text_color(int flags)
{
	if ( flags & PF_BODY_RED ) {
		gr_set_color_fast(&Color_red);
		return;
	}

	if ( flags & PF_BODY_GREEN ) {
		gr_set_color_fast(&Color_green);
		return;
	}

	if ( flags & PF_BODY_BLUE ) {
		gr_set_color_fast(&Color_blue);
		return;
	}

	gr_set_color_fast(&Color_bright_blue);
}

// set the popup text color
void popup_set_title_color(int flags)
{
	if ( flags & PF_TITLE_RED ) {
		gr_set_color_fast(&Color_red);
		return;
	}

	if ( flags & PF_TITLE_GREEN ) {
		gr_set_color_fast(&Color_green);
		return;
	}

	if ( flags & PF_TITLE_BLUE ) {
		gr_set_color_fast(&Color_blue);
		return;
	}

	if ( flags & PF_TITLE_WHITE ) {
		gr_set_color_fast(&Color_bright_white);
		return;
	}

	gr_set_color_fast(&Color_bright_blue);
}

// Draw the title centered within the popup
void popup_draw_title(int sy, const char *line, int flags)
{
	int w, h, sx;

	if ( flags & PF_TITLE_BIG ) {
		gr_set_font(FONT2);
	} else {
		gr_set_font(FONT1);
	}

	gr_get_string_size(&w, &h, line);
	sx = fl2i(Title_coords[gr_screen.res][4] - w/2.0f + 0.5f);

	popup_set_title_color(flags);
	gr_string(sx,sy,line);
}

// calculate the starting display index
int popup_calc_starting_index(popup_info *pi)
{
	// we're basically ignoring any titles here. 
	if(pi->nlines <= Popup_max_display[gr_screen.res]){
		return 0;
	}

#ifndef MAKE_FS1
	// otherwise, we want to see what item index the slider is on
	return Popup_slider.get_currentItem();
#else
	return 0;
#endif
}

// Figure out the y-coord to start drawing the popup text.  The text
// is centered vertically within the popup.
int popup_calc_starting_y(popup_info *pi)
{
	int sy, total_h=0;
	int num_lines = pi->nlines > Popup_max_display[gr_screen.res] ? Popup_max_display[gr_screen.res] : pi->nlines;

	if ( pi->flags & (PF_TITLE | PF_TITLE_BIG) ) {
		if ( pi->flags & PF_TITLE_BIG ) {
			gr_set_font(FONT2);
		} else {
			gr_set_font(FONT1);
		}
		total_h += gr_get_font_height();
	}

	if ( pi->flags & PF_BODY_BIG ) {
		gr_set_font(FONT2);
	} else {
		gr_set_font(FONT1);
	}

	total_h += num_lines * gr_get_font_height();
	sy = fl2i((Popup_text_coords[gr_screen.res][1] + Popup_text_coords[gr_screen.res][3]/2.0f) - total_h/2.0f + 0.5f);

	// if this is an input style box, add in some y
	if(pi->flags & PF_INPUT){
		sy += Popup_input_text_y_offset[gr_screen.res];
	}

	return sy;
}

// Draw the message text nicely formatted in the popup
void popup_draw_msg_text(popup_info *pi)
{
	int sx, sy, i, w, h;
	int line_index;
	int line_count;

	// figure out the starting display 
	line_index = popup_calc_starting_index(pi);

	// figure out the starting y:
	sy = popup_calc_starting_y(pi);

	// draw title if required
	if ( pi->flags & (PF_TITLE | PF_TITLE_BIG) ) {
		popup_draw_title(sy, pi->title, pi->flags);
		sy += gr_get_font_height();
	}

	// draw body 
	if ( pi->flags & PF_BODY_BIG ) {
		gr_set_font(FONT2);
	} else {
		gr_set_font(FONT1);
	}

	popup_set_text_color(pi->flags);
	line_count = 0;
	for ( i = line_index; i < pi->nlines; i++, line_count++ ) {
		// if we've already displayed the max # of lines
		if(line_count >= Popup_max_display[gr_screen.res]){
			break;
		}

		gr_get_string_size(&w, &h, pi->msg_lines[i]);
		sx = fl2i(Title_coords[gr_screen.res][4] - w/2.0f + 0.5f);
		gr_string(sx, sy + line_count * h, pi->msg_lines[i]);
	}

	// maybe draw "more"
	h = 10;
	if(i < pi->nlines){
		gr_set_color_fast(&Color_bright_red);
		gr_string(Title_coords[gr_screen.res][4], sy + (Popup_max_display[gr_screen.res]) * h, XSTR("More", 459));
	}

	gr_set_font(FONT1);	// reset back to regular font size
}

// Draw the button text nicely formatted in the popup
void popup_draw_button_text(popup_info *pi)
{
	int w, h, i, sx, sy;

	gr_set_color_fast(&Color_bright_blue);

	for ( i=0; i < pi->nchoices; i++ ) {
		gr_get_string_size(&w, &h, pi->button_text[i]);

		if ( (pi->nchoices == 1) && (pi->flags & PF_USE_AFFIRMATIVE_ICON) ) {
			sx = Button_regions[gr_screen.res][i+1][0]-w;
			sy = Button_regions[gr_screen.res][i+1][1]+4;
		} else {
			sx = Button_regions[gr_screen.res][i][0]-w;
			sy = Button_regions[gr_screen.res][i][1]+4;
		}

		gr_string(sx, sy, pi->button_text[i]);

		// figure out where to draw underline char
		if ( pi->shortcut_index[i] > 0 ) {
			int	cut=pi->shortcut_index[i];
			char	save_char=pi->button_text[i][cut];
			pi->button_text[i][cut] = 0;
			gr_get_string_size(&w, &h, pi->button_text[i]);
			pi->button_text[i][cut] = save_char;
			sx += w;
		}
		
		if ( pi->shortcut_index[i] >= 0 ) {
			gr_printf(sx, sy, NOX("%c"), 95);
		}
	}
}

// See if any of the button should change appearance based on mouse position
void popup_force_draw_buttons(popup_info *pi)
{
	int i,mouse_is_highlighting=0;
	UI_BUTTON *br, *b;

	for ( i = 0; i < pi->nchoices; i++ ) {
		br = &Popup_button_regions[i];
		b = &Popup_buttons[i];
		if ( br->button_down() ) {
			b->draw_forced(2);
			mouse_is_highlighting=1;
			continue;
		}

		if ( (b->button_hilighted()) || (br->button_hilighted() && !b->button_down()) ) {
			Popup_default_choice=i;
			mouse_is_highlighting=1;
			b->draw_forced(1);
		}
	}

	// Only if mouse is not highlighting an option, let the default choice be drawn highlighted
	if ( (!mouse_is_highlighting) && (pi->nchoices>1) ) {
		for ( i = 0; i < pi->nchoices; i++ ) {
			b = &Popup_buttons[i];
			// highlight the default choice
			if ( i == Popup_default_choice ) {
				b->draw_forced(1);
			}
		}
	}
}

static void popup_do(popup_info *pi)
{
	int test = -1;

	os_poll();

	// if we were killed by a call to popup_kill_any_active(), kill the popup
	if (Popup_should_die) {
		pi->choice = -1;

		if (pi->callback) {
			(*(pi->callback))(-1);
		} else {
			popup_close(pi);
		}

		return;
	}

	// if we're flagged as should be running the state underneath, then do so
	if (pi->flags & PF_RUN_STATE) {
		game_do_state(gameseq_get_state());
	}
	// otherwise just run the common functions (for networking,etc)
	else {
		game_set_frametime(-1);
		game_do_state_common(gameseq_get_state(), pi->flags & PF_NO_NETWORKING);	// do stuff common to all states
	}

	// test the condition function or process for the window
	if (pi->condition && ((test = (*(pi->condition))()) > 0)) {
		pi->choice = test;

		if (pi->callback) {
			(*(pi->callback))(pi->choice);
		} else {
			popup_close(pi);
		}

		return;
	} else {
		int k = Popup_window.process();						// poll for input, handle mouse

		pi->choice = popup_process_keys(pi, k);

		if (pi->choice != POPUP_NOCHANGE) {
			if (pi->callback) {
				(*(pi->callback))(pi->choice);
			} else {
				popup_close(pi);
			}

			return;
		}

		pi->choice = popup_check_buttons(pi);

		if (pi->choice != POPUP_NOCHANGE) {
			if (pi->callback) {
				(*(pi->callback))(pi->choice);
			} else {
				popup_close(pi);
			}

			return;
		}
	}

	// don't draw anything
	if ( !(pi->flags & PF_RUN_STATE) ) {
		gr_restore_screen(pi->screen_id);
	}

	// if this is an input popup, store the input text
	if (pi->flags & PF_INPUT) {
		Popup_input.get_text(pi->input_text);
	}

	Popup_window.draw();
	popup_force_draw_buttons(pi);
	popup_draw_msg_text(pi);
	popup_draw_button_text(pi);

	gr_flip();
}

void popup_do_frame()
{
	if ( !Popup_is_active ) {
		return;
	}

	popup_info *pi = &Popup_info;

	popup_do(pi);
}

static int popup_do_sync()
{
	popup_info *pi = &Popup_info;

	SDL_assert(Popup_is_active);

	popup_do(pi);

	return pi->choice;
}

// maybe assign a keyboard shortcut to this button
// input:	pi		=>	popup information so far
//				i		=>	number of choice
//				str	=> string for button press	
void popup_maybe_assign_keypress(popup_info *pi, int n, char *str)
{
	int i,j,len=0,next_char_is_shortcut=0;

	pi->keypress[n]=-1;
	pi->shortcut_index[n]=-1;

	len=static_cast<int>(SDL_strlen(str)+1);

	pi->button_text[n] = (char*)malloc(len);
	memset(pi->button_text[n], 0, len);

	j=0;
	// copy chars over, watching for underline meta-char '&'
	for (i=0; i<len-1; i++) {
		if ( str[i] == '&' ) {
			pi->shortcut_index[n]=i;
			next_char_is_shortcut=1;
		} else {
			if ( next_char_is_shortcut ) {
				next_char_is_shortcut=0;
				char first_char_string[2];
				first_char_string[0]=str[i];
				first_char_string[1]=0;
				SDL_strlwr(first_char_string);
				pi->keypress[n] = first_char_string[0];
			}
			pi->button_text[n][j++]=str[i];	
		}
	}
}

static void popup_internal(void (*callback)(int), int (*condition)(), int flags, int nchoices, va_list args)
{
	int			i;
	char			*format, *s;
	popup_info *pi = &Popup_info;

	pi->flags = flags;
	pi->callback = callback;
	pi->condition = condition;

	SDL_assert( nchoices > 0 && nchoices <= POPUP_MAX_CHOICES );
	pi->nchoices = nchoices;

	// get button text
	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		pi->button_text[i] = NULL;
		popup_maybe_assign_keypress(pi, i, s);
	}

	// get msg text
	format = va_arg( args, char * );
	pi->raw_text[0] = 0;
	SDL_vsnprintf(pi->raw_text, SDL_arraysize(pi->raw_text), format, args);

	if ( popup_init(pi) == -1 ) {
		if (callback) {
			(*callback)(-1);
		}

		pi->choice = POPUP_ABORT;
		popup_close(pi);
		return;
	}

	pi->screen_id = gr_save_screen();

	gamesnd_play_iface(SND_POPUP_APPEAR); 	// play sound when popup appears

	Mouse_hidden = 0;
	Popup_is_active = 1;
}

// input:		callback		=>		function to call on user action (popup done, etc.)
//				flags			=>		formatting specificatons (PF_... shown above)
//				nchoices		=>		number of choices popup has
//				text_1		=>		text for first button
//				...			=>
//				text_n		=>		text for last button
//				msg text		=>		text msg for popup (can be of form "%s",pl->text)
//
// typical usage:
//	popup(NULL, 0, 2, POPUP_YES, POPUP_NO, "Hey %s, do you want to quit", pl->callsign);
void popup_callback(void (*callback)(int), int flags, int nchoices, ... )
{
	va_list args;

	if ( Popup_is_active ) {
		return;
	}

	va_start(args, nchoices);

	popup_internal(callback, NULL, flags, nchoices, args);

	va_end(args);
}

void popup(int flags, int nchoices, ...)
{
	va_list args;

	if ( Popup_is_active ) {
		return;
	}

	va_start(args, nchoices);

	popup_internal(NULL, NULL, flags, nchoices, args);

	va_end(args);
}

int popup_sync(int flags, int nchoices, ...)
{
	popup_info *pi = &Popup_info;
	va_list args;

	if ( Popup_is_active ) {
		Int3();		// should never happen
		return -1;
	}

	va_start(args, nchoices);

	popup_internal(NULL, NULL, flags, nchoices, args);

	va_end(args);

	int rval = pi->choice;	// if init failed, this will be ABORT

	while (rval == POPUP_NOCHANGE) {
		rval = popup_do_sync();
	}

	popup_done();

	if (rval == POPUP_ABORT) {
		return -1;
	}

	return rval;
}

// determine if a popup is being drawn
int popup_active()
{
	return Popup_is_active;
}

// This function displays a popup message box and every frame it checks the condition() function
// which is passed in as an argument.
// If the condition() returns TRUE, the message box ends itself.  This function returns whatever
// the condition() did if the condition() occurred, and FALSE if the cancel button was pressed.
int popup_till_condition(int (*condition)(), ...) 
{
	popup_info *pi = &Popup_info;
	va_list args;

	if ( Popup_is_active ) {
		Int3();		// should never happen
		return -1;
	}

	va_start(args, condition);

	popup_internal(NULL, condition, 0, 1, args);

	va_end(args);

	int rval = pi->choice;	// if init failed, this will be ABORT

	while (rval == POPUP_NOCHANGE) {
		rval = popup_do_sync();
	}

	popup_done();

	if (rval == POPUP_ABORT) {
		return 0;
	}

	return rval;
}

// popup to return the value from an input box
void popup_input(void (*callback)(int), int flags, const char *caption, int max_output_len)
{
	popup_info *pi = &Popup_info;

	if ( Popup_is_active ) {
		return;
	}

	// make it an inputbox type popup
	pi->flags = flags | PF_INPUT;

	pi->callback = NULL;
	pi->condition = NULL;

	// add a cancel button
	pi->nchoices = 0;
	// popup_maybe_assign_keypress(&Popup_info, 0, "&Cancel");	

	// get msg text
	SDL_assert(caption != NULL);
	SDL_strlcpy(pi->raw_text, caption, SDL_arraysize(pi->raw_text));
	SDL_assert(SDL_strlen(pi->raw_text) < POPUP_MAX_CHARS );

	// set input text length
	if((max_output_len > POPUP_INPUT_MAX_CHARS) || (max_output_len == -1)){
		pi->max_input_text_len = POPUP_INPUT_MAX_CHARS - 1;
	} else {
		pi->max_input_text_len = max_output_len;
	}

	if ( popup_init(pi) == -1 ) {
		if (callback) {
			(*callback)(-1);
		}

		pi->choice = POPUP_ABORT;
		popup_close(pi);
		return;
	}

	pi->screen_id = gr_save_screen();

	// zero the popup input text
	SDL_zero(pi->input_text);

	gamesnd_play_iface(SND_POPUP_APPEAR); 	// play sound when popup appears

	Mouse_hidden = 0;
	Popup_is_active = 1;
}

const char *popup_input_sync(int flags, const char *caption, int max_output_len)
{
	popup_info *pi = &Popup_info;

	if ( Popup_is_active ) {
		Int3();		// should never happen
		return NULL;
	}

	popup_input(NULL, flags, caption, max_output_len);

	int rval = pi->choice;	// if init failed, this will be ABORT

	while (rval == POPUP_NOCHANGE) {
		rval = popup_do_sync();
	}

	popup_done();

	if (rval == POPUP_ABORT) {
		return NULL;
	}

	return pi->input_text;
}

const char *popup_get_input_text()
{
	popup_info *pi = &Popup_info;

	return pi->input_text;
}

int popup_running_state()
{
	return Popup_running_state;
}

// kill any active popup, forcing it to return -1 (similar to ESC)
void popup_kill_any_active()
{
	if(Popup_is_active){
		Popup_should_die = 1;
	}
}

// change the text inside of the popup 
void popup_change_text(const char *new_text)
{
	// copy the raw text
	SDL_strlcpy(Popup_info.raw_text, new_text, SDL_arraysize(Popup_info.raw_text));

	// recalculate all display information
	popup_split_lines(&Popup_info);
}

