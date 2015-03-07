/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef __OAL_H__
#define __OAL_H__

#include "al.h"
#include "alc.h"

#include "pstypes.h"


#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM		1
#endif

#ifndef WAVE_FORMAT_ADPCM
#define WAVE_FORMAT_ADPCM	2
#endif

typedef struct WAVE_chunk {
	short code;
	ushort num_channels;
	uint sample_rate;
	uint bytes_per_second;
	ushort block_align;
	ushort bits_per_sample;

	ushort extra_size;
	ubyte *extra_data;

	WAVE_chunk() : code(0), num_channels(0), sample_rate(0), bytes_per_second(0),
			block_align(0), bits_per_sample(0), extra_size(0), extra_data(NULL)
	{
	}
} WAVE_chunk;

typedef struct sound_channel {
	int		sig;		// uniquely identifies the sound playing on the channel
	int		snd_id;		// identifies which kind of sound is playing
	ALuint	source_id;	// OpenAL source id
	int		buf_idx;		// currently bound buffer index (-1 if none)
	int		flags;		// looping, voice_msg, 3d, ...
	float	vol;
	int		priority;		// implementation dependant priority
	ALint	last_position;

	sound_channel() : sig(0), snd_id(0), source_id(0), buf_idx(-1), flags(0),
			vol(1.0f), priority(0), last_position(0)
	{
	}
} sound_channel;

typedef struct sound_info {
	int format;		// WAVE_FORMAT_* defines
	uint size;
	int sample_rate;
	int avg_bytes_per_sec;
	int n_block_align;
	int bits;
	int n_channels;
	int duration;	// time in ms for duration of sound
	ubyte *data;
} sound_info;


int oal_init();
void oal_close();

int oal_is_initted();

int oal_get_channel(int sig);
int oal_get_number_channels();

int oal_get_buffer_size(int sid, int *size);
int oal_get_channel_size(int channel);

void oal_stop_buffer(int sid);
void oal_stop_channel(int channel);
void oal_stop_channel_all();

void oal_set_volume(int channel, float volume);
void oal_set_pan(int channel, float pan);
void oal_set_pitch(int channel, float pitch);
void oal_set_play_position(int channel, int position);

float oal_get_pitch(int channel);
int oal_get_play_position(int channel);

int oal_is_channel_playing(int channel);

void oal_chg_loop_status(int channel, int loop);

int oal_parse_wave(const char *filename, ubyte **dest, uint *dest_size, WAVE_chunk **header);

int oal_load_buffer(int *sid, int *final_size, WAVE_chunk *header, sound_info *si, int flags);
void oal_unload_buffer(int sid);

int oal_create_buffer(int frequency, int bits_per_sample, int nchannels, int nseconds);
int oal_lock_data(int sid, ubyte *data, int size);

sound_channel *oal_get_free_channel(float volume, int snd_id, int priority);

void oal_do_frame();

int oal_update_source(int channel, int min, int max, vector *pos, vector *vel);
int oal_update_listener(vector *pos, vector *vel, matrix *orient);

int oal_play(int sid, int snd_id, int priority, float volume, float pan, int flags);
int oal_play_3d( int sid, int snd_id, vector *pos, vector *vel, int min, int max, int looping, float max_volume, float estimated_vol, int priority);

bool oal_check_for_errors(const char *location);

#endif // __OAL_H__
