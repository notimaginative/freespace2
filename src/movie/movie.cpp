#include <stdio.h>
#include <ctype.h>
#include "mvelib.h"
#include "mve_audio.h"
#include "cutscenes.h"
#include "freespace.h"

extern void initializeMovie(MVESTREAM *mve);
extern void playMovie(MVESTREAM *mve);
extern void shutdownMovie(MVESTREAM *mve);

int movie_play(char *filename, int unknown)
{
	fprintf (stderr, "Playing MVE file %s\n",filename);

	// mark the mve as viewable in the techroom if in a campaign
	if (Game_mode & GM_CAMPAIGN_MODE) {
		cutscene_mark_viewable(filename);
	}

#ifdef MVE
	int i;
	char lower_name[MAX_FILENAME_LEN] = "";

	// lowercase filename to avoid loading problems from mixed case calls
	strcpy( lower_name, filename );
	for (i=0; i<(int)strlen(lower_name); i++ ){
		lower_name[i] = char(tolower(lower_name[i]));
	}

	char file[200];
//	snprintf (file, 200, "movies/%s", filename);
	snprintf (file, 200, "Data/Movies/%s", lower_name);
	MVESTREAM *mve = mve_open(file);
	if (mve == NULL)
	{
		fprintf(stderr, "can't open MVE file '%s'\n", file);
		return -1;
	}

	initializeMovie(mve);
	playMovie(mve);
	shutdownMovie(mve);

	mve_close(mve);
#else
	fprintf(stderr, "STUB: if movie support existed, you'd be watching %s\n", filename);
#endif
	return 1;
}

int movie_play_two(char *filename1, char *filename2)
{
	fprintf(stderr, "STUB: if movie support existed, you'd be watching %s, followed by %s\n", filename1, filename2);	

	return 1;
}
