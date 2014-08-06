/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <vector>
#include <list>

#include "pstypes.h"
#include "oal.h"
#include "oal_efx.h"
#include "cfile.h"
#include "sound.h"
#include "acm.h"


static int OAL_inited = 0;

static ALCdevice *al_device = NULL;
static ALCcontext *al_context = NULL;


struct sound_buffer {
	ALuint buf_id;		// OpenAL buffer id
	int chan_idx;		// channel index this buffer is currently bound to

	int frequency;
	int bits_per_sample;
	int nchannels;
	int nseconds;
	int nbytes;

	sound_buffer() : buf_id(0), chan_idx(-1), frequency(0), bits_per_sample(0),
			nchannels(0), nseconds(0), nbytes(0)
	{
	}
};

static std::vector<sound_buffer> Buffers;

static std::vector<sound_channel> Channels;
static int channel_next_sig = 1;

extern void oal_efx_attach(ALuint source_id);


bool oal_check_for_errors(const char *location)
{
	ALenum err = alGetError();

	if (err != AL_NO_ERROR) {
#ifndef NDEBUG
		if (location) {
			const char *str = alGetString(err);

			nprintf(("OpenAL", "AL-ERROR (%s) => 0x%x: %s\n", location, err, (str) ? str : "??"));
		} else {
			nprintf(("OpenAL", "AL-ERROR => 0x%x: %s\n", err, alGetString(err)));
		}
#endif
		return true;
	}

	return false;
}

static void oal_init_channels()
{
	const int MAX_SOURCES = 32;

	Channels.reserve(MAX_SOURCES);

	for (int n = 0; n < MAX_SOURCES; n++) {
		sound_channel n_channel;

		alGenSources(1, &n_channel.source_id);

		if ( !n_channel.source_id || (alGetError() != AL_NO_ERROR) ) {
			break;
		}

		Channels.push_back(n_channel);
	}
}

int oal_init(int use_eax)
{
	ALint ver_major = 0, ver_minor = 0;

	if (OAL_inited) {
		return 0;
	}

	nprintf(( "Sound", "SOUND ==> Initializing OpenAL...\n" ));

	alcGetIntegerv(NULL, ALC_MAJOR_VERSION, 1, &ver_major);
	alcGetIntegerv(NULL, ALC_MINOR_VERSION, 1, &ver_minor);

	if ( (ver_major < 1) || (ver_minor < 1) ) {
		nprintf(("Sound", "SOUND ==> Minimum supported OpenAL version is 1.1\n"));
		return -1;
	}

	al_device = alcOpenDevice(NULL);

	if (al_device == NULL) {
		nprintf(("Sound", "SOUND ==> Unable to open device!\n"));
		nprintf(("Sound", "SOUND ==>    %s", alcGetString(al_device, alcGetError(al_device))));
		return -1;
	}

	al_context = alcCreateContext(al_device, NULL);

	if (al_context == NULL) {
		nprintf(("Sound", "SOUND ==> Unable to create context!\n"));
		nprintf(("Sound", "SOUND ==>    %s", alcGetString(al_device, alcGetError(al_device))));

		alcCloseDevice(al_device);
		al_device = NULL;

		return -1;
	}

	alcMakeContextCurrent(al_context);

	OAL_inited = 1;

	oal_init_channels();

	Buffers.reserve(64);

	if (use_eax) {
		oal_efx_init();
	}

	oal_check_for_errors("oal_init() end");

	return 0;
}

void oal_close()
{
	if ( !OAL_inited ) {
		return;
	}

	oal_check_for_errors("oal_close() begin");

	while ( !Channels.empty() ) {
		ALuint sid = Channels.back().source_id;

		alSourceStop(sid);

		alSourcei(sid, AL_BUFFER, 0);

		alDeleteSources(1, &sid);

		Channels.pop_back();
	}

	while ( !Buffers.empty() ) {
		ALuint bid = Buffers.back().buf_id;

		if ( alIsBuffer(bid) ) {
			alDeleteBuffers(1, &bid);
		}

		Buffers.pop_back();
	}

	oal_efx_close();

	oal_check_for_errors("oal_close() end");

	Channels.clear();
	Buffers.clear();

	alcMakeContextCurrent(NULL);
	alcDestroyContext(al_context);
	alcCloseDevice(al_device);

	al_context = NULL;
	al_device = NULL;

	OAL_inited = 0;
}

int oal_get_channel(int sig)
{
	int i;

	if ( !OAL_inited ) {
		return -1;
	}

	int size = (int)Channels.size();

	for (i = 0; i < size; i++) {
		if (Channels[i].sig == sig) {
			ALint status;

			alGetSourcei(Channels[i].source_id, AL_SOURCE_STATE, &status);

			if (status == AL_PLAYING) {
				return i;
			} else {
				return -1;
			}
		}
	}

	return -1;
}

int oal_get_number_channels()
{
	int i;
	ALint status;
	int count = 0;

	if ( !OAL_inited ) {
		return -1;
	}

	int size = (int)Channels.size();

	for (i = 0; i < size; i++) {
		alGetSourcei(Channels[i].source_id, AL_SOURCE_STATE, &status);

		if (status == AL_PLAYING) {
			count++;
		}
	}

	return count;
}

void oal_stop_buffer(int sid)
{
	if ( !OAL_inited ) {
		return;
	}

	oal_check_for_errors("oal_stop_buffer() begin");

	SDL_assert( sid >= 0 );
	SDL_assert( sid < (int)Buffers.size() );

	int cid = Buffers[sid].chan_idx;

	if (cid != -1) {
		ALuint source_id = Channels[cid].source_id;

		alSourceStop(source_id);
		alSourcei(source_id, AL_BUFFER, 0);
	}

	oal_check_for_errors("oal_stop_buffer() end");
}

void oal_stop_channel(int channel)
{
	if ( !OAL_inited ) {
		return;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_stop_channel() begin");

	alSourceStop(Channels[channel].source_id);

	alSourcei(Channels[channel].source_id, AL_BUFFER, 0);
	Channels[channel].buf_idx = -1;

	oal_check_for_errors("oal_stop_channel() end");
}

void oal_stop_channel_all()
{
	if ( !OAL_inited ) {
		return;
	}

	int size = (int)Channels.size();

	for (int i = 0; i < size; i++) {
		oal_stop_channel(i);
	}
}

int oal_get_buffer_size(int sid, int *size)
{
	if ( !OAL_inited ) {
		return -1;
	}

	if ( (sid < 0) || (sid >= (int)Buffers.size()) ) {
		return -1;
	}

	*size = Buffers[sid].nbytes;

	return 0;
}

int oal_get_channel_size(int channel)
{
	if ( !OAL_inited ) {
		return -1;
	}

	if ( (channel < 0) || (channel >= (int)Channels.size()) ) {
		return -1;
	}

	if (Channels[channel].buf_idx >= 0) {
		return Buffers[Channels[channel].buf_idx].nbytes;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Source properties *set functions
// -----------------------------------------------------------------------------

void oal_set_volume(int channel, float volume)
{
	if ( !OAL_inited ) {
		return;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_set_volume() begin");

	alSourcef(Channels[channel].source_id, AL_GAIN, volume);

	oal_check_for_errors("oal_set_volume() end");
}

void oal_set_pan(int channel, float pan)
{
	if ( !OAL_inited ) {
		return;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_set_pan() begin");

	alSource3f(Channels[channel].source_id, AL_POSITION, pan, 0.0f, 0.0f);

	oal_check_for_errors("oal_set_pan() end");
}

void oal_set_pitch(int channel, float pitch)
{
	if ( !OAL_inited ) {
		return;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_set_pitch() begin");

	alSourcef(Channels[channel].source_id, AL_PITCH, pitch);

	oal_check_for_errors("oal_set_pitch() end");
}

void oal_set_play_position(int channel, int position)
{
	if ( !OAL_inited ) {
		return;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_set_play_position() begin");

	alSourcei(Channels[channel].source_id, AL_BYTE_OFFSET, position);

	oal_check_for_errors("oal_set_play_position() end");
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Source properties *get functions
// -----------------------------------------------------------------------------

float oal_get_pitch(int channel)
{
	float pitch = 1.0f;

	if ( !OAL_inited ) {
		return 1.0f;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_get_pitch() begin");

	alGetSourcef(Channels[channel].source_id, AL_PITCH, &pitch);

	oal_check_for_errors("oal_get_pitch() end");

	return pitch;
}

int oal_get_play_position(int channel)
{
	ALint offset = 0;

	if ( !OAL_inited ) {
		return 0;
	}

	if (channel < 0) {
		return 0;
	}

	//SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_get_play_position() begin");

	alGetSourcei(Channels[channel].source_id, AL_BYTE_OFFSET, &offset);

	if (alGetError() != AL_NO_ERROR) {
		return offset;
	}

	return 0;
}

// -----------------------------------------------------------------------------

int oal_is_initted()
{
	return OAL_inited;
}

int oal_is_channel_playing(int channel)
{
	ALint status;

	if ( !OAL_inited ) {
		return 0;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_is_channel_playing() begin");

	alGetSourcei(Channels[channel].source_id, AL_SOURCE_STATE, &status);

	oal_check_for_errors("oal_is_channel_playing() end");

	if (status == AL_PLAYING) {
		return 1;
	}

	return 0;
}

void oal_chg_loop_status(int channel, int loop)
{
	if ( !OAL_inited ) {
		return;
	}

	SDL_assert( channel >= 0 );
	SDL_assert( channel < (int)Channels.size() );

	oal_check_for_errors("oal_chg_loop_status() begin");

	alSourcei(Channels[channel].source_id, AL_LOOPING, (loop) ? AL_TRUE : AL_FALSE);

	if (loop) {
		Channels[channel].flags |= SND_FLAG_LOOPING;
	} else {
		Channels[channel].flags &= ~SND_FLAG_LOOPING;
	}

	oal_check_for_errors("oal_chg_loop_status() end");
}

static int oal_get_free_channel_idx(float new_volume, int snd_id, int priority)
{
	int	i, limit;
	float lowest_vol = 0.0f;
	int lowest_vol_index = -1;
	int status;
	int instance_count = 0;	// number of instances of sound already playing
	float lowest_instance_vol = 1.0f;
	int lowest_instance_vol_index = -1;

	int first_free_channel = -1;

	if ( !OAL_inited ) {
		return -1;
	}

	int size = (int)Channels.size();

	oal_check_for_errors("oal_get_free_channel_idx() begin");

	// Look for a channel to use to play this sample
	for (i = 0; i < size; i++) {
		sound_channel *chp = &Channels[i];
		int looping = chp->flags & SND_FLAG_LOOPING;

		if (chp->snd_id == 0) {
			if (first_free_channel == -1) {
				first_free_channel = i;
			}

			continue;
		}

		alGetSourcei(chp->source_id, AL_SOURCE_STATE, &status);

		if ( (status != AL_PLAYING) && (status != AL_PAUSED) ) {
			if (first_free_channel == -1) {
				first_free_channel = i;
			}

			continue;
		} else {
			if (chp->snd_id == snd_id) {
				instance_count++;

				if ( (chp->vol < lowest_instance_vol) && !looping ) {
					lowest_instance_vol = chp->vol;
					lowest_instance_vol_index = i;
				}
			}

			if ( (chp->vol < lowest_vol) && !looping ) {
				lowest_vol_index = i;
				lowest_vol = chp->vol;
			}
		}
	}

	// determine the limit of concurrent instances of this sound
	switch (priority) {
		case SND_PRIORITY_MUST_PLAY:
			limit = 100;
			break;

		case SND_PRIORITY_SINGLE_INSTANCE:
			limit = 1;
			break;

		case SND_PRIORITY_DOUBLE_INSTANCE:
			limit = 2;
			break;

		case SND_PRIORITY_TRIPLE_INSTANCE:
			limit = 3;
			break;

		default:
			Int3();			// get Alan
			limit = 100;
			break;
	}


	// If we've exceeded the limit, then maybe stop the duplicate if it is lower volume
	if (instance_count >= limit) {
		// If there is a lower volume duplicate, stop it.... otherwise, don't play the sound
		if ( (lowest_instance_vol_index >= 0) && (Channels[lowest_instance_vol_index].vol <= new_volume) ) {
			first_free_channel = lowest_instance_vol_index;
		} else {
			first_free_channel = -1;
		}
	} else {
		// there is no limit barrier to play the sound, so see if we've ran out of channels
		if (first_free_channel == -1) {
			// stop the lowest volume instance to play our sound if priority demands it
			if ( (lowest_vol_index != -1) && (priority == SND_PRIORITY_MUST_PLAY) ) {
				// Check if the lowest volume playing is less than the volume of the requested sound.
				// If so, then we are going to trash the lowest volume sound.
				if (Channels[lowest_vol_index].vol <= new_volume) {
					first_free_channel = lowest_vol_index;
				}
			}
		}
	}

	oal_check_for_errors("oal_get_free_channel_idx() end");

	return first_free_channel;
}

// get a channel for use elsewhere (MVE playback, streaming audio, etc.)
sound_channel *oal_get_free_channel(float volume, int snd_id, int priority)
{
	if ( !OAL_inited ) {
		return NULL;
	}

	int chan = oal_get_free_channel_idx(volume, snd_id, priority);

	if (chan < 0) {
		return NULL;
	}

	SDL_assert( Channels[chan].source_id != 0 );

	alSourceStop(Channels[chan].source_id);
	alSourcei(Channels[chan].source_id, AL_BUFFER, 0);

	if (Channels[chan].buf_idx >= 0) {
		Buffers[Channels[chan].buf_idx].chan_idx = -1;
	}

	Channels[chan].vol = volume;
	Channels[chan].priority = priority;
	Channels[chan].last_position = 0;
	Channels[chan].flags = 0;
	Channels[chan].buf_idx = -1;
	Channels[chan].snd_id = snd_id;
	Channels[chan].sig = channel_next_sig++;

	if (channel_next_sig < 0) {
		channel_next_sig = 1;
	}

	return &Channels[chan];
}

int oal_parse_wave(const char *filename, ubyte **dest, uint *dest_size, WAVE_chunk **header)
{
	CFILE *cfp = NULL;
	int id = 0;
	unsigned int tag = 0, size = 0, next_chunk;
	WAVE_chunk hdr;

	if ( !OAL_inited ) {
		return 0;
	}

	cfp = cfopen(filename, "rb");

	if (cfp == NULL) {
		nprintf(("Error", "Couldn't open '%s'\n", filename ));
		return -1;
	}

	// check for valid file type
	id = cfread_int(cfp);

	// 'RIFF'
	if (id != 0x46464952) {
		nprintf(("Error", "Not a WAVE file '%s'\n", filename));
		cfclose(cfp);
		return -1;
	}

	// skip RIFF size
	cfread_int(cfp);

	// check for valid RIFF type
	id = cfread_int(cfp);

	// 'WAVE'
	if (id != 0x45564157) {
		nprintf(("Error", "Not a WAVE file '%s'\n", filename));
		cfclose(cfp);
		return -1;
	}

	// parse WAVE tags
	while ( !cfeof(cfp) ) {
		tag = cfread_uint(cfp);
		size = cfread_uint(cfp);

		next_chunk = cftell(cfp) + size;

		switch (tag) {
			// 'fmt '
			case 0x20746d66: {
				hdr.code = cfread_short(cfp);
				hdr.num_channels = cfread_ushort(cfp);
				hdr.sample_rate = cfread_uint(cfp);
				hdr.bytes_per_second = cfread_uint(cfp);
				hdr.block_align = cfread_ushort(cfp);
				hdr.bits_per_sample = cfread_ushort(cfp);

				if (hdr.code != 1) {
					hdr.extra_size = cfread_ushort(cfp);
				}

				(*header) = (WAVE_chunk*) malloc (sizeof(WAVE_chunk));
				SDL_assert( (*header) != NULL );

				memcpy((*header), &hdr, sizeof(WAVE_chunk));

				if (hdr.extra_size) {
					(*header)->extra_data = (ubyte*) malloc (hdr.extra_size);
					SDL_assert( (*header)->extra_data != NULL );

					cfread((*header)->extra_data, hdr.extra_size, 1, cfp);
				}

				break;
			}

			// 'data'
			case 0x61746164: {
				*dest_size = size;

				(*dest) = (ubyte*) malloc (size);
				SDL_assert( (*dest) != NULL );

				cfread((*dest), size, 1, cfp);

				break;
			}

			// drop everything else
			default:
				break;
		}

		cfseek(cfp, next_chunk, CF_SEEK_SET);
	}

	cfclose(cfp);

	return 0;
}

static int oal_get_free_buffer()
{
	if ( !OAL_inited ) {
		return -1;
	}

	int size = (int)Buffers.size();

	for (int i = 0; i < size; i++) {
		if (Buffers[i].buf_id == 0) {
			return i;
		}
	}

	sound_buffer nbuf;

	Buffers.push_back(nbuf);

	return (int)(Buffers.size()-1);
}

int oal_load_buffer(int *sid, int *final_size, WAVE_chunk *header, sound_info *si, int flags)
{
	SDL_assert( final_size != NULL );
	SDL_assert( header != NULL );
	SDL_assert( si != NULL );

	if ( !OAL_inited ) {
		return 0;
	}

	int buf_idx = oal_get_free_buffer();

	if (buf_idx < 0) {
		return -1;
	}

	*sid = buf_idx;

	sound_buffer *buf = &Buffers[buf_idx];

	oal_check_for_errors("oal_load_buffer() begin");

	alGenBuffers(1, &buf->buf_id);

	if ( !buf->buf_id ) {
		return -1;
	}

	ALenum format = AL_INVALID;
	ALsizei size;
	ALint bits, bps;
	ALuint frequency;
	ALvoid *data = NULL;

	// the below conversion variables are only used when the wav format is not PCM.
	ubyte *convert_buffer = NULL;		// storage for converted wav file
	int convert_len;					// num bytes of converted wav file
	uint src_bytes_used;				// number of source bytes actually converted (should always be equal to original size)


	switch (si->format) {
		case WAVE_FORMAT_PCM: {
			SDL_assert( si->data != NULL );

			bits = si->bits;
			bps  = si->avg_bytes_per_sec;
			size = si->size;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			// swap 16-bit sound data
			if (bits == 16) {
				ushort *swap_tmp;

				for (uint i=0; i<size; i=i+2) {
					swap_tmp = (ushort*)(si->data + i);
					*swap_tmp = INTEL_SHORT(*swap_tmp);
				}
			}
#endif

			data = si->data;

			break;
		}

		case WAVE_FORMAT_ADPCM: {
			SDL_assert( si->data != NULL );

			// this ADPCM decoder decodes to 16-bit only so keep that in mind
			nprintf(( "Sound", "SOUND ==> converting sound from ADPCM to PCM\n" ));

			int rc = ACM_convert_ADPCM_to_PCM(header, si->data, si->size, &convert_buffer, 0, &convert_len, &src_bytes_used, 16);

			// ACM conversion failed?
			if ( (rc == -1) || (src_bytes_used != si->size) ) {
				alDeleteBuffers(1, &buf->buf_id);
				buf->buf_id = 0;

				if (convert_buffer != NULL) {
					free(convert_buffer);
				}

				return -1;
			}

			bits = 16;
			bps  = (((si->n_channels * bits) / 8) * si->sample_rate);
			size = convert_len;
			data = convert_buffer;

			nprintf(( "Sound", "SOUND ==> Coverted sound from ADPCM to PCM successfully\n" ));

			break;
		}

		default:
			nprintf(( "Sound", "Unsupported sound encoding\n" ));
			alDeleteBuffers(1, &buf->buf_id);
			buf->buf_id = 0;
			return -1;
	}

	// format is now in pcm
	frequency = si->sample_rate;

	if (bits == 16) {
		if (si->n_channels == 2) {
			format = AL_FORMAT_STEREO16;
		} else if (si->n_channels == 1) {
			format = AL_FORMAT_MONO16;
		}
	} else if (bits == 8) {
		if (si->n_channels == 2) {
			format = AL_FORMAT_STEREO8;
		} else if (si->n_channels == 1) {
			format = AL_FORMAT_MONO8;
		}
	}

	if (format == AL_INVALID) {
		alDeleteBuffers(1, &buf->buf_id);
		buf->buf_id = 0;

		if (convert_buffer != NULL) {
			free(convert_buffer);
		}

		return -1;
	}

	Snd_sram += size;
	*final_size = size;

	alBufferData(buf->buf_id, format, data, size, frequency);

	buf->chan_idx = -1;
	buf->frequency = frequency;
	buf->bits_per_sample = bits;
	buf->nchannels = si->n_channels;
	buf->nseconds = size / bps;
	buf->nbytes = size;

	if (convert_buffer != NULL) {
		free(convert_buffer);
	}

	oal_check_for_errors("oal_load_buffer() end");

	return 0;
}

void oal_unload_buffer(int sid)
{
	if ( !OAL_inited ) {
		return;
	}

	if ( (sid < 0) || (sid >= (int)Buffers.size()) ) {
		return;
	}

	oal_check_for_errors("oal_unload_buffer() begin");

	sound_buffer *buf = &Buffers[sid];

	if (buf->buf_id) {
		if (buf->chan_idx >= 0) {
			alSourceStop(Channels[buf->chan_idx].source_id);
			alSourcei(Channels[buf->chan_idx].source_id, AL_BUFFER, 0);
			buf->chan_idx = -1;
		}

		alDeleteBuffers(1, &buf->buf_id);
		buf->buf_id = 0;
	}

	oal_check_for_errors("oal_unload_buffer() end");
}

int oal_create_buffer(int frequency, int bits_per_sample, int nchannels, int nseconds)
{
	if ( !OAL_inited ) {
		return -1;
	}

	int buf_idx = oal_get_free_buffer();

	if (buf_idx < 0) {
		return -1;
	}

	sound_buffer *buf = &Buffers[buf_idx];

	oal_check_for_errors("oal_load_buffer() begin");

	alGenBuffers(1, &buf->buf_id);

	if ( !buf->buf_id ) {
		return -1;
	}

	buf->chan_idx = -1;
	buf->frequency = frequency;
	buf->bits_per_sample = bits_per_sample;
	buf->nchannels = nchannels;
	buf->nseconds = nseconds;
	buf->nbytes = nseconds * (bits_per_sample / 8) * nchannels * frequency;

	return buf_idx;
}

int oal_lock_data(int sid, ubyte *data, int size)
{
	if ( !OAL_inited ) {
		return -1;
	}

	oal_check_for_errors("oal_lock_data() begin");

	SDL_assert( sid >= 0 );
	SDL_assert( sid < (int)Buffers.size() );

	ALuint buf_id = Buffers[sid].buf_id;
	ALenum format;

	if (Buffers[sid].bits_per_sample == 16) {
		if (Buffers[sid].nchannels == 2) {
			format = AL_FORMAT_STEREO16;
		} else if (Buffers[sid].nchannels == 1) {
			format = AL_FORMAT_MONO16;
		} else {
			return -1;
		}
	} else if (Buffers[sid].bits_per_sample == 8) {
		if (Buffers[sid].nchannels == 2) {
			format = AL_FORMAT_STEREO8;
		} else if (Buffers[sid].nchannels == 1) {
			format = AL_FORMAT_MONO8;
		} else {
			return -1;
		}
	} else {
		return -1;
	}

	Buffers[sid].nbytes = size;

	alBufferData(buf_id, format, data, size, Buffers[sid].frequency);

	if ( oal_check_for_errors("oal_lock_data() end") ) {
		return -1;
	}

	return 0;
}

int oal_play(int sid, int snd_id, int priority, float volume, float pan, int flags)
{
	if ( !OAL_inited ) {
		return -1;
	}

	SDL_assert( sid >= 0 );
	SDL_assert( sid < (int)Buffers.size() );

	oal_check_for_errors("oal_play() begin");

	int channel = oal_get_free_channel_idx(volume, snd_id, priority);

	if (channel < 0) {
		return -1;
	}

	sound_channel *chan = &Channels[channel];

	ALint status;
	alGetSourcei(chan->source_id, AL_SOURCE_STATE, &status);

	if (status == AL_PLAYING) {
		oal_stop_channel(channel);
	}

	// set all the things
	chan->vol = volume;
	chan->flags = flags;
	chan->priority = priority;
	chan->last_position = 0;
	chan->buf_idx = sid;
	chan->snd_id = snd_id;
	chan->sig = channel_next_sig++;

	Buffers[sid].chan_idx = channel;

	alSource3f(chan->source_id, AL_POSITION, pan, 0.0f, -1.0f);
	alSource3f(chan->source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	alSourcef(chan->source_id, AL_PITCH, 1.0f);
	alSourcef(chan->source_id, AL_GAIN, volume);
	alSourcei(chan->source_id, AL_BUFFER, Buffers[sid].buf_id);
	alSourcei(chan->source_id, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcei(chan->source_id, AL_LOOPING, (flags & SND_FLAG_LOOPING) ? AL_TRUE : AL_FALSE);

	// maybe attach source to reverb effect
	oal_efx_attach(chan->source_id);

	// Actually play it
	alSourcePlay(chan->source_id);

	if (channel_next_sig < 0) {
		channel_next_sig = 1;
	}

	oal_check_for_errors("oal_play() end");

	return chan->sig;
}

int oal_play_3d( int sid, int snd_id, vector *pos, vector *vel, int min, int max, int looping, float max_volume, float estimated_vol, int priority)
{
	if ( !OAL_inited ) {
		return -1;
	}

	SDL_assert( sid >= 0 );
	SDL_assert( sid < (int)Buffers.size() );

	oal_check_for_errors("oal_play_3d() begin");

	int channel = oal_get_free_channel_idx(estimated_vol, snd_id, priority);

	if (channel < 0) {
		return -1;
	}

	sound_channel *chan = &Channels[channel];

	ALint status;
	alGetSourcei(chan->source_id, AL_SOURCE_STATE, &status);

	if (status == AL_PLAYING) {
		oal_stop_channel(channel);
	}

	int flags = SND_FLAG_3D;

	if (looping) {
		flags |= SND_FLAG_LOOPING;
	}

	// set all the things
	chan->vol = max_volume;
	chan->flags = flags;
	chan->priority = priority;
	chan->last_position = 0;
	chan->buf_idx = sid;
	chan->snd_id = snd_id;
	chan->sig = channel_next_sig++;

	Buffers[sid].chan_idx = channel;

	oal_update_source(channel, min, max, pos, vel);

	alSourcef(chan->source_id, AL_PITCH, 1.0f);
	alSourcef(chan->source_id, AL_GAIN, max_volume);
	alSourcei(chan->source_id, AL_BUFFER, Buffers[sid].buf_id);
	alSourcei(chan->source_id, AL_SOURCE_RELATIVE, AL_FALSE);
	alSourcei(chan->source_id, AL_LOOPING, (looping) ? AL_TRUE : AL_FALSE);

	// maybe attach source to reverb effect
	oal_efx_attach(chan->source_id);

	// Actually play it
	alSourcePlay(chan->source_id);

	if (channel_next_sig < 0) {
		channel_next_sig = 1;
	}

	oal_check_for_errors("oal_play_3d() end");

	return chan->sig;
}

void oal_do_frame()
{
	ALint state, current_position;

	if ( !OAL_inited ) {
		return;
	}

	oal_check_for_errors("oal_do_frame() begin");

	int size = (int)Channels.size();

	// make sure there aren't any looping voice messages
	for (int i = 0; i < size; i++) {
		if ( (Channels[i].flags & SND_FLAG_VOICE) && (Channels[i].flags & SND_FLAG_LOOPING) ) {
			alGetSourcei(Channels[i].source_id, AL_SOURCE_STATE, &state);

			if (state != AL_PLAYING) {
				continue;
			}

			alGetSourcei(Channels[i].source_id, AL_BYTE_OFFSET, &current_position);

			if (current_position != 0) {
				if (current_position < Channels[i].last_position) {
					alSourceStop(Channels[i].source_id);
				} else {
					Channels[i].last_position = current_position;
				}
			}
		}
	}

	oal_check_for_errors("oal_do_frame() end");
}

int oal_update_source(int channel, int min, int max, vector *pos, vector *vel)
{
	if ( !OAL_inited ) {
		return 0;
	}

	if (channel < 0) {
		return 0;
	}

	if ( !Channels[channel].flags & SND_FLAG_3D ) {
		return 1;
	}

	ALuint source_id = Channels[channel].source_id;
	ALfloat rolloff = 1.0f;

	oal_check_for_errors("oal_update_source() begin");

	if (pos) {
		alSource3f(source_id, AL_POSITION, pos->xyz.x, pos->xyz.y, -pos->xyz.z);
	}

	if (vel) {
		alSource3f(source_id, AL_VELOCITY, vel->xyz.x, vel->xyz.y, vel->xyz.z);
	} else {
		alSource3f(source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	}

	if (min >= 0) {
		if (max <= min) {
			rolloff = 0.0f;
		} else {
			const float MIN_GAIN = 0.05f;
			float minf = i2fl(min);
			float maxf = i2fl(max);

			// yep, just making this shit up
			rolloff = (minf / (minf + (maxf - minf))) / MIN_GAIN;

			if (rolloff < 0.0f) {
				rolloff = 0.0f;
			}
		}

		alSourcef(source_id, AL_ROLLOFF_FACTOR, rolloff);

		alSourcei(source_id, AL_REFERENCE_DISTANCE, min);
		alSourcei(source_id, AL_MAX_DISTANCE, max);
	}

	oal_check_for_errors("oal_update_source() end");

	return 0;
}

int oal_update_listener(vector *pos, vector *vel, matrix *orient)
{
	if ( !OAL_inited ) {
		return 0;
	}

	oal_check_for_errors("oal_update_listener() begin");

	if (pos) {
		alListener3f(AL_POSITION, pos->xyz.x, pos->xyz.y, -pos->xyz.z);
	}

	if (vel) {
		alListener3f(AL_VELOCITY, vel->xyz.x, vel->xyz.y, vel->xyz.z);
	}

	if (orient) {
		ALfloat alOrient[6];

		alOrient[0] =  orient->v.fvec.xyz.x;
		alOrient[1] =  orient->v.fvec.xyz.y;
		alOrient[2] = -orient->v.fvec.xyz.z;

		alOrient[3] =  orient->v.uvec.xyz.x;
		alOrient[4] =  orient->v.uvec.xyz.y;
		alOrient[5] = -orient->v.uvec.xyz.z;

		alListenerfv(AL_ORIENTATION, alOrient);
	}

	oal_check_for_errors("oal_update_listener() end");

	return 0;
}
