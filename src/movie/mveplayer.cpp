/*
 * $Logfile: /Freespace2/code/movie/mveplayer.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * MVE movie playing routines
 *
 * $Log$
 * Revision 1.7  2005/10/01 21:48:01  taylor
 * various cleanups
 * fix decoder to swap opcode 0xb since it screws up on PPC
 * the previous opcode 0xc change was wrong since we had already determined that it messes up FS1 movies
 *
 * Revision 1.6  2005/08/12 08:47:24  taylor
 * use new audiostr code rather than old windows/unix version
 * update all OpenAL commands with new error checking macros
 * fix play_position to properly account for real position, fixes the talking heads and message text cutting out early
 * movies will now use better filtering when scaled
 *
 * Revision 1.5  2005/03/31 21:26:02  taylor
 * s/alGetSourceiv/alGetSourcei/
 *
 * Revision 1.4  2005/03/31 00:06:20  taylor
 * go back to more accurate timer and allow video scaling for movies
 *
 * Revision 1.3  2005/03/29 07:50:34  taylor
 * Update to newest movie code with much better video support and audio support from
 *   Pierre Willenbrock.  Movies are enabled always now (no longer a build option)
 *   and but can be skipped with the "--nomovies" or "-n" cmdline options.
 *
 *
 * $NoKeywords: $
 */

#include "pstypes.h"
#include "mvelib.h"
#include "movie.h"
#include "2d.h"
#include "key.h"
#include "gamepad.h"
#include "osapi.h"
#include "timer.h"
#include "sound.h"
#include "bmpman.h"
#include "osregistry.h"
#include "oal.h"
#include <vector>

static int mve_playing;


// timer variables
static int micro_frame_delay = 0;
static int timer_started = 0;
static int timer_created = 0;
static unsigned int timer_expire;
static Uint64 micro_timer_start = 0;
static Uint64 micro_timer_freq = 0;

// audio variables
#define MVE_AUDIO_BUFFERS 8  // total buffers to interact with stream

static std::vector<ALuint> mve_audio_bufl_free;
static ubyte *mve_audio_buf = NULL;
static int mve_audio_buf_size = 0;
static int mve_audio_buf_offset = 0;

static int mve_audio_playing = 0;
static int mve_audio_canplay = 0;
static int mve_audio_compressed = 0;
static int audiobuf_created = 0;

// struct for the audio stream information
struct mve_audio_t {
	sound_channel *chan;
	ALenum format;
	int sample_rate;
	int bytes_per_sec;
	int channels;
	int bitsize;
	ALuint buffers[MVE_AUDIO_BUFFERS];
};

mve_audio_t *mas = NULL;  // mve_audio_stream



// video variables
int g_width, g_height;
void *g_vBuffers = NULL;
void *g_vBackBuf1, *g_vBackBuf2;
ushort *pixelbuf = NULL;
static ubyte *g_pCurMap=NULL;
static int g_nMapLength=0;
static int videobuf_created;


// the decoder
void decodeFrame16(ubyte *pFrame, ubyte *pMap, int mapRemain, ubyte *pData, int dataRemain);

/*************************
 * general handlers
 *************************/
void mve_end_movie()
{
	mve_playing = 0;
}

/*************************
 * timer handlers
 *************************/

int mve_timer_create(ubyte *data)
{
	micro_frame_delay = mve_get_int(data) * (int)mve_get_short(data+4);

	micro_timer_start = SDL_GetPerformanceCounter();
	micro_timer_freq = SDL_GetPerformanceFrequency();

	timer_created = 1;

	return 1;
}

static unsigned int mve_timer_get_microseconds()
{
	Uint64 us = SDL_GetPerformanceCounter() - micro_timer_start;

	us *= 1000000;
	us /= micro_timer_freq;

	return (unsigned int)us;
}

static void mve_timer_start(void)
{
	if (!timer_created)
		return;

	timer_expire = mve_timer_get_microseconds();
	timer_expire += micro_frame_delay;

	timer_started = 1;
}

static int mve_do_timer_wait(void)
{
	if (!timer_started)
		return 0;

	unsigned int tv, ts;

	tv = mve_timer_get_microseconds();

	if (tv > timer_expire)
		goto end;

	ts = timer_expire - tv;

	SDL_Delay(ts / 1000);

end:
	timer_expire += micro_frame_delay;

	return 0;
}

static void mve_timer_stop()
{
	timer_expire = 0;
	timer_started = 0;
	timer_created = 0;

	micro_frame_delay = 0;

	micro_timer_start = 0;
	micro_timer_freq = 0;
}

/*************************
 * audio handlers
 *************************/

// setup the audio information from the data stream
void mve_audio_createbuf(ubyte minor, ubyte *data)
{
	if (audiobuf_created)
		return;

	// if game sound disabled don't try and play movie audio
	if ( !Sound_enabled ) {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	int flags, desired_buffer, sample_rate;

	mas = (mve_audio_t *) malloc ( sizeof(mve_audio_t) );

	if (mas == NULL) {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	memset(mas, 0, sizeof(mve_audio_t));

	mas->format = AL_INVALID;

	flags = mve_get_ushort(data + 2);
	sample_rate = mve_get_ushort(data + 4);
	desired_buffer = mve_get_int(data + 6);

	if (desired_buffer > 0) {
		mve_audio_buf = (ubyte*) malloc (desired_buffer);

		if (mve_audio_buf == NULL) {
			mve_audio_canplay = 0;
			audiobuf_created = 1;
			return;
		}

		mve_audio_buf_size = desired_buffer;
		mve_audio_buf_offset = 0;
	} else {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	mas->channels = (flags & 0x0001) ? 2 : 1;
	mas->bitsize = (flags & 0x0002) ? 16 : 8;

	mas->sample_rate = sample_rate;

	if (minor > 0) {
		mve_audio_compressed = flags & 0x0004 ? 1 : 0;
	} else {
		mve_audio_compressed = 0;
	}

	if (mas->bitsize == 16) {
		if (mas->channels == 2) {
			mas->format = AL_FORMAT_STEREO16;
		} else if (mas->channels == 1) {
			mas->format = AL_FORMAT_MONO16;
		}
	} else if (mas->bitsize == 8) {
		if (mas->channels == 2) {
			mas->format = AL_FORMAT_STEREO8;
		} else if (mas->channels == 1) {
			mas->format = AL_FORMAT_MONO8;
		}
	}

	// somethings wrong, bail now
	if (mas->format == AL_INVALID) {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	oal_check_for_errors("mve_audio_createbuf() begin");

	for (int i = 0; i < MVE_AUDIO_BUFFERS; i++) {
		alGenBuffers(1, &mas->buffers[i]);

		if ( !mas->buffers[i] ) {
			mve_audio_canplay = 0;
			audiobuf_created = 1;
			return;
		}
	}

	mve_audio_bufl_free.assign(mas->buffers, mas->buffers+MVE_AUDIO_BUFFERS);

	mas->chan = oal_get_free_channel(1.0f, -1, SND_PRIORITY_MUST_PLAY);

	if (mas->chan == NULL) {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	alSourcef(mas->chan->source_id, AL_GAIN, 1.0f);
	alSource3f(mas->chan->source_id, AL_POSITION, 0.0f, 0.0f, 0.0f);
	alSource3f(mas->chan->source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	alSource3f(mas->chan->source_id, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
	alSourcef(mas->chan->source_id, AL_ROLLOFF_FACTOR, 0.0f);
	alSourcei(mas->chan->source_id, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcei(mas->chan->source_id, AL_LOOPING, AL_FALSE);

	oal_check_for_errors("mve_audio_createbuf() end");

	audiobuf_created = 1;
	mve_audio_canplay = 1;
}

// play and stream the audio
void mve_audio_play()
{
	if (mve_audio_canplay) {
		ALint queued = 0;
		ALint status = AL_INVALID;

		oal_check_for_errors("mve_audio_play() begin");

		alGetSourcei(mas->chan->source_id, AL_BUFFERS_QUEUED, &queued);
		alGetSourcei(mas->chan->source_id, AL_SOURCE_STATE, &status);

		if ( (status != AL_PLAYING) && (queued > 0) ) {
			alSourcePlay(mas->chan->source_id);
			mve_audio_playing = 1;
		}

		oal_check_for_errors("mve_audio_play() end");
	}
}

static void mve_audio_pause(bool paused)
{
	if (mve_audio_canplay && mve_audio_playing) {
		if (paused) {
			alSourcePause(mas->chan->source_id);
		} else {
			alSourcePlay(mas->chan->source_id);
		}
	}
}

// call this in shutdown to stop and close audio
static void mve_audio_stop()
{
	if (!audiobuf_created)
		return;

	oal_check_for_errors("mve_audio_stop() begin");

	mve_audio_playing = 0;
	mve_audio_canplay = 0;
	mve_audio_compressed = 0;

	audiobuf_created = 0;

	mve_audio_bufl_free.clear();

	if (mas) {
		if (mas->chan) {
			alSourceStop(mas->chan->source_id);

			// detach buffers from source so that we can delete them
			alSourcei(mas->chan->source_id, AL_BUFFER, 0);
		}

		for (int i = 0; i < MVE_AUDIO_BUFFERS; i++) {
			if ( alIsBuffer(mas->buffers[i]) ) {
				alDeleteBuffers(1, &mas->buffers[i]);
			}
		}
	}

	if (mas != NULL) {
		free(mas);
		mas = NULL;
	}

	if (mve_audio_buf != NULL) {
		free(mve_audio_buf);
		mve_audio_buf = NULL;
	}

	mve_audio_buf_size = 0;
	mve_audio_buf_offset = 0;

	oal_check_for_errors("mve_audio_stop() end");
}

int mve_audio_data(ubyte major, ubyte *data)
{
	static const int selected_chan = 1;
	int chan;
	int nsamp;
	ALint processed = 0;
	ALuint bid;

	if (mve_audio_canplay) {
		chan = mve_get_ushort(data + 2);
		nsamp = mve_get_ushort(data + 4);

		if (chan & selected_chan) {
			oal_check_for_errors("mve_audio_data() begin");

			if ( (mve_audio_buf_offset+nsamp+4) <= mve_audio_buf_size ) {
				if (major == 8) {
					if (mve_audio_compressed) {
						/* HACK: +4 mveaudio_uncompress adds 4 more bytes */
						nsamp += 4;

						mveaudio_uncompress(mve_audio_buf+mve_audio_buf_offset, data, -1);
					} else {
						nsamp -= 8;
						data += 8;

						memcpy(mve_audio_buf+mve_audio_buf_offset, data, nsamp);
					}
				} else {
					// silence
					memset(mve_audio_buf+mve_audio_buf_offset, 0, nsamp);
				}

				mve_audio_buf_offset += nsamp;
			} else {
				mprintf(("MVE audio_buf overrun!!\n"));
			}

			alGetSourcei(mas->chan->source_id, AL_BUFFERS_PROCESSED, &processed);

			while (processed) {
				alSourceUnqueueBuffers(mas->chan->source_id, 1, &bid);

				mve_audio_bufl_free.push_back(bid);
				--processed;
			}

			if ( !mve_audio_bufl_free.empty() ) {
				bid = mve_audio_bufl_free.back();

				alBufferData(bid, mas->format, mve_audio_buf, mve_audio_buf_offset, mas->sample_rate);
				alSourceQueueBuffers(mas->chan->source_id, 1, &bid);

				mve_audio_buf_offset = 0;
				mve_audio_bufl_free.pop_back();
			}

			if ( !mve_audio_playing ) {
				mve_audio_play();
			}

			oal_check_for_errors("mve_audio_data() end");
		}
	}

	return 1;
}

/*************************
 * video handlers
 *************************/

int mve_video_createbuf(ubyte minor, ubyte *data)
{
	if (videobuf_created)
		return 1;

	short w, h;

	w = mve_get_short(data);
	h = mve_get_short(data+2);

	g_width = w << 3;
	g_height = h << 3;

	// with Pierre's decoder16 fix in opcode 0xc, 8 should no longer be needed
	g_vBackBuf1 = g_vBuffers = malloc(g_width * g_height * 4);

	if (g_vBackBuf1 == NULL) {
		mprintf(("MVE-ERROR: Can't allocate video buffer\n"));
		videobuf_created = 1;
		return 0;
	}

	g_vBackBuf2 = (ushort *)g_vBackBuf1 + (g_width * g_height);

	memset(g_vBackBuf1, 0, g_width * g_height * 4);

	// DDOI - Allocate RGB565 pixel buffer
	pixelbuf = (ushort *)malloc (g_width * g_height * 2);

	if (pixelbuf == NULL) {
		mprintf(("MVE-ERROR: Can't allocate memory for pixelbuf\n"));
		videobuf_created = 1;
		return 0;
	}

	memset(pixelbuf, 0, g_width * g_height * 2);

	gr_stream_start(-1, -1, g_width, g_height);

	videobuf_created = 1;

	return 1;
}

static void mve_convert_and_draw()
{
	ushort *pDests;
	ushort *pSrcs;
	ushort *pixels = (ushort *)g_vBackBuf1;
	ushort px;
	int x, y;
	ubyte r, g, b, a;

	pSrcs = pixels;

	pDests = pixelbuf;

	for (y=0; y<g_height; y++) {
		for (x = 0; x < g_width; x++) {
			// convert from abgr to rgba
			px = (1<<15)|*pSrcs;

			r = ubyte((px & 0x7C00) >> 10);
			g = ubyte((px & 0x3E0) >> 5);
			b = ubyte((px & 0x1F) >> 0);
			a = ubyte((px & 0x8000) >> 15);

			pDests[x] = (r << 11) | (g << 6) | (b << 1) | (a << 0);

			pSrcs++;
		}
		pDests += g_width;
	}
}

void mve_video_display()
{
	static uint mve_video_skiptimer = 0;

	fix t1 = timer_get_fixed_seconds();

	// micro_frame_delay is divided by 10 to match mve_video_skiptimer overflow catch
	if ( mve_video_skiptimer > (uint)(micro_frame_delay/10) ) {
		// we are running slow so subtract desired time from actual and skip this frame
		mve_video_skiptimer -= (micro_frame_delay/10);
		return;
	} else {
		// zero out so we can get a new count
		mve_video_skiptimer = 0;
	}

	mve_convert_and_draw();

	gr_stream_frame( (ubyte*)pixelbuf );

	gr_flip();

	fix t2 = timer_get_fixed_seconds();

	// only get a new count if we are definitely through with old count
	if ( mve_video_skiptimer == 0 ) {
		// for a more accurate count convert the frame rate to a float and multiply
		// by one-hundred-thousand before converting to an uint.
		mve_video_skiptimer = (uint)(f2fl(t2-t1) * 100000);
	}
}

void mve_video_codemap(ubyte *data, int len)
{
	g_pCurMap = data;
	g_nMapLength = len;
}

void mve_video_data(ubyte *data, int len)
{
	ushort nFlags;
	ubyte *temp;

	nFlags = mve_get_ushort(data+12);

	if (nFlags & 1) {
		temp = (ubyte *)g_vBackBuf1;
		g_vBackBuf1 = g_vBackBuf2;
		g_vBackBuf2 = temp;
	}

	decodeFrame16((ubyte *)g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
}

void mve_end_chunk()
{
	g_pCurMap = NULL;
}

void mve_init(MVESTREAM *mve)
{
	// reset to default values
	mve_audio_playing = 0;
	mve_audio_canplay = 0;
	mve_audio_compressed = 0;
	mve_audio_buf_offset = 0;
	audiobuf_created = 0;

	videobuf_created = 0;

	mve_playing = 1;
}

void mve_play(MVESTREAM *mve)
{
	int init_timer = 0, timer_error = 0;
	int cont = 1;
	bool mve_paused = false;
	int k = -1;

	if (!timer_started)
		mve_timer_start();

	while (cont && mve_playing && !timer_error) {
		if (mve_paused) {
			// just redraw current frame while paused
			mve_video_display();
		} else {
			cont = mve_play_next_chunk(mve);
		}

		if (micro_frame_delay && !init_timer) {
			mve_timer_start();
			init_timer = 1;
		}

		timer_error = mve_do_timer_wait();

		os_poll();

		k = key_inkey();

		if (k == SDLK_ESCAPE || k == SDLK_RETURN || gamepad_action_or_cancel()) {
			mve_playing = 0;
		} else if (k == SDLK_SPACE) {
			mve_paused = !mve_paused;
			mve_audio_pause(mve_paused);
		}
	}
}

void mve_shutdown()
{
	mve_audio_stop();

	mve_timer_stop();

	if (pixelbuf != NULL) {
		free(pixelbuf);
		pixelbuf = NULL;
	}

	if (g_vBuffers != NULL) {
		free(g_vBuffers);
		g_vBuffers = NULL;
	}

	gr_stream_stop();
}
