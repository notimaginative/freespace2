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
	STUB_FUNCTION;
	
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
