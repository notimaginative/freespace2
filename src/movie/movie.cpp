/*
 * $Logfile: /Freespace2/src/movie/movie.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Frontend for MVE playing
 *
 * $Log$
 * Revision 1.5  2005/10/01 21:48:01  taylor
 * various cleanups
 * fix decoder to swap opcode 0xb since it screws up on PPC
 * the previous opcode 0xc change was wrong since we had already determined that it messes up FS1 movies
 *
 * Revision 1.4  2005/03/29 07:50:34  taylor
 * Update to newest movie code with much better video support and audio support from
 *   Pierre Willenbrock.  Movies are enabled always now (no longer a build option)
 *   and but can be skipped with the "--nomovies" or "-n" cmdline options.
 *
 *
 *
 * $NoKeywords: $
 *
 */

#include "pstypes.h"
#include "mvelib.h"
#include "movie.h"
#include "cutscenes.h"
#include "freespace.h"
#include "mouse.h"
#include "sound.h"
#include "cmdline.h"
#include "gamesequence.h"
#include "mainhallmenu.h"
#include "audiostr.h"


int movie_play(const char *filename)
{
	// mark the movie as viewable in the techroom if in a campaign
	if (Game_mode & GM_CAMPAIGN_MODE) {
		cutscene_mark_viewable(filename);
	}

	// always allow movies to play in cutscene viewer
	if (gameseq_get_state() != GS_STATE_VIEW_CUTSCENES) {
		if ( !Cmdline_play_movies ) {
			mprintf(("Movies are disabled, skipping playback of '%s'...\n", filename));
			return 1;
		}
	}

	MVESTREAM *movie = NULL;

	movie = mve_open(filename);

	if (movie == NULL) {
		mprintf(("Can't open movie file: '%s'\n", filename));
		return 0;
	}

	// kill all background sounds
	snd_stop_all();
	audiostream_pause_all();

	// clear the screen and hide the mouse cursor
	mouse_hide_cursor();
	gr_set_clear_color(0, 0, 0);
	gr_reset_clip();
	gr_clear();
	gr_flip();
	gr_clear();
	gr_zbuffer_clear(1);	// G400, blah

	// ready to play...
	mve_init(movie);
	mve_play(movie);

	// ...done playing, close the mve and show the cursor again
	mve_shutdown();
	mve_close(movie);

	mouse_show_cursor();

	audiostream_unpause_all();

	return 1;
}

int movie_play_two(const char *filename1, const char *filename2)
{
	// make sure the first movie played correctly, then play the second one
	if ( movie_play(filename1) ) {
		movie_play(filename2);
	} else {
		printf("Not playing second movie: %s\n", filename2);
		return 0;
	}

	return 1;
}
