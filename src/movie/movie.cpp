#include <stdio.h>
#include "mvelib.h"
#include "mve_audio.h"

extern void initializeMovie(MVESTREAM *mve);
extern void playMovie(MVESTREAM *mve);
extern void shutdownMovie(MVESTREAM *mve);

int movie_play(char *filename, int unknown)
{
	fprintf (stderr, "Playing MVE file %s\n",filename);
#ifdef MVE
	char file[200];
	snprintf (file, 200, "movies/%s", filename);
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
