#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include "pstypes.h"

void ds3d_close()
{
	STUB_FUNCTION;
}

int ds3d_update_listener(vector *pos, vector *vel, matrix *orient)
{
	//STUB_FUNCTION;
	ALfloat posv[] = { pos->x, pos->y, pos->z };
	ALfloat velv[] = { vel->x, vel->y, vel->z };
	ALfloat oriv[] = { orient->a1d[0], 
			orient->a1d[1], orient->a1d[2],
			orient->a1d[3], orient->a1d[4],
			orient->a1d[5] };
	alListenerfv(AL_POSITION, posv);
	alListenerfv(AL_VELOCITY, velv);
	alListenerfv(AL_ORIENTATION, oriv);

	return -1;
}

int ds3d_init (int unused)
{
	ALfloat pos[] = { 0.0, 0.0, 0.0 },
		vel[] = { 0.0, 0.0, 0.0 },
		ori[] = { 0.0, 0.0, 1.0, 0.0, -1.0, 0.0 };

	alListenerfv (AL_POSITION, pos);
	alListenerfv (AL_VELOCITY, vel);
	alListenerfv (AL_ORIENTATION, ori);

	if(alGetError() != AL_NO_ERROR)
		return -1;

	return 0;
}
