/*
 * $Logfile: /Freespace2/src/movie/movie.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Frontend for MVE playing
 *
 * $Log$
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

int movie_play(char *filename, int cd_prompt)
{
	// mark the movie as viewable in the techroom if in a campaign
	if (Game_mode & GM_CAMPAIGN_MODE) {
		cutscene_mark_viewable(filename);
	}

	if (Cmdline_play_movies) {
		MVESTREAM *movie;

		printf("Playing movie: %s\n", filename);

		// umm, yeah
	//	if ( cd_prompt == -1 )
	//		cd_prompt = require_cd;

		// look for correct CD when viewing movies in the tech room
	//	if (gameseq_get_state() == GS_STATE_VIEW_CUTSCENES) {
	//		cutscenes_validate_cd(filename, cd_prompt);
	//	}

		movie = mve_open(filename);

		if (movie) {
			// kill all background sounds
			game_stop_looped_sounds();
			main_hall_stop_music();
			main_hall_stop_ambient();

			// clear the screen and hide the mouse cursor
			Mouse_hidden++;
			gr_reset_clip();
			gr_clear();
			gr_flip();
			gr_zbuffer_clear(1);	// G400, blah
			
			// ready to play...
			mve_init(movie);
			mve_play(movie);

			// ...done playing, close the mve and show the cursor again
			mve_shutdown();
			mve_close(movie);

			Mouse_hidden--;
			main_hall_start_ambient();
		} else {
			printf("Can't open movie file: '%s'\n", filename);
			return 0;
		}
	
	} else {
		mprintf(("Movies are disabled, skipping...\n"));
	}

	return 1;
}

int movie_play_two(char *filename1, char *filename2)
{
	// FIXME: part of the CD code which isn't included yet
	int require_cd = 0;

	// make sure the first movie played correctly, then play the second one
	if (movie_play(filename1, require_cd)) {
		movie_play(filename2, require_cd);
	} else {
		printf("Not playing second movie: %s\n", filename2);
		return 0;
	}

	return 1;
}
