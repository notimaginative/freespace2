#include <stdio.h>

int movie_play(char *filename, int unknown)
{
	fprintf(stderr, "STUB: if movie support existed, you'd be watching %s\n", filename);
	return 1;
}

int movie_play_two(char *filename1, char *filename2)
{
	fprintf(stderr, "STUB: if movie support existed, you'd be watching %s, followed by %s\n", filename1, filename2);	

	return 1;
}
