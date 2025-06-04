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
static SDL_AudioStream *mve_audio_stream = nullptr;
static ubyte *mve_audio_buf = nullptr;
static int mve_audio_buf_size = 0;
static int mve_audio_buf_offset = 0;

static int mve_audio_playing = 0;
static int mve_audio_canplay = 0;
static int mve_audio_compressed = 0;
static int audiobuf_created = 0;


// video variables
int g_width, g_height;
void *g_vBuffers = NULL;
void *g_vBackBuf1, *g_vBackBuf2;
static SDL_Surface *pixelbuf = nullptr;
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

	int flags = mve_get_ushort(data + 2);
	int sample_rate = mve_get_ushort(data + 4);
	int desired_buffer = mve_get_int(data + 6);

	int channels = (flags & 0x0001) ? 2 : 1;
	int bitsize = (flags & 0x0002) ? 16 : 8;

	if (desired_buffer <= 0) {
		desired_buffer = sample_rate * channels * (bitsize >> 3);
	}

	if (desired_buffer > 0) {
		mve_audio_buf = (ubyte*) malloc (desired_buffer);

		if (mve_audio_buf == nullptr) {
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

	if (minor > 0) {
		mve_audio_compressed = flags & 0x0004 ? 1 : 0;
	} else {
		mve_audio_compressed = 0;
	}

	if ( !SDL_InitSubSystem(SDL_INIT_AUDIO) ) {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	SDL_AudioSpec spec{};

	spec.channels = channels;
	spec.format = (bitsize == 16) ? SDL_AUDIO_S16LE : SDL_AUDIO_U8;
	spec.freq = sample_rate;

	mve_audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
												 &spec, nullptr, nullptr);

	if ( !mve_audio_stream ) {
		mve_audio_canplay = 0;
		audiobuf_created = 1;
		return;
	}

	mve_audio_playing = 0;

	audiobuf_created = 1;
	mve_audio_canplay = 1;
}

// play and stream the audio
void mve_audio_play()
{
	if ( !mve_audio_canplay ) {
		return;
	}

	if (mve_audio_buf_offset > 0) {
		SDL_PutAudioStreamData(mve_audio_stream, mve_audio_buf, mve_audio_buf_offset);
		mve_audio_buf_offset = 0;
	}

	if ( !mve_audio_playing ) {
		SDL_ResumeAudioStreamDevice(mve_audio_stream);
		mve_audio_playing = 1;
	}
}

static void mve_audio_pause(bool paused)
{
	if (mve_audio_canplay && mve_audio_playing) {
		if (paused) {
			SDL_PauseAudioStreamDevice(mve_audio_stream);
		} else {
			SDL_ResumeAudioStreamDevice(mve_audio_stream);
		}
	}
}

// call this in shutdown to stop and close audio
static void mve_audio_stop()
{
	if (!audiobuf_created)
		return;

	mve_audio_playing = 0;
	mve_audio_canplay = 0;
	mve_audio_compressed = 0;

	audiobuf_created = 0;

	if (mve_audio_buf != nullptr) {
		free(mve_audio_buf);
		mve_audio_buf = nullptr;
	}

	mve_audio_buf_size = 0;
	mve_audio_buf_offset = 0;

	if (mve_audio_stream) {
		SDL_DestroyAudioStream(mve_audio_stream);
		mve_audio_stream = nullptr;

		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}

int mve_audio_data(ubyte major, ubyte *data)
{
	static const int selected_chan = 1;
	int chan;
	int nsamp;

	if (mve_audio_canplay) {
		chan = mve_get_ushort(data + 2);
		nsamp = mve_get_ushort(data + 4);

		if (chan & selected_chan) {
			// if we're going to overrun then go ahead and offload the buffer
			if ((mve_audio_buf_offset+nsamp+4) >= mve_audio_buf_size) {
				SDL_PutAudioStreamData(mve_audio_stream, mve_audio_buf, mve_audio_buf_offset);
				mve_audio_buf_offset = 0;
			}

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
	pixelbuf = SDL_CreateSurface(g_width, g_height, SDL_PIXELFORMAT_RGB565);

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
	ubyte r, g, b;

	pSrcs = pixels;

	pDests = reinterpret_cast<ushort *>(pixelbuf->pixels);

	for (y=0; y<g_height; y++) {
		for (x = 0; x < g_width; x++) {
			// convert from abgr to rgb565
			px = (*pSrcs) | 0x8000;

			r = (px >> 10) & 0x1F;
			g = (px >> 5) & 0x1F;
			b = (px >> 0) & 0x1F;
			// upscale green to 6 bits
			g = (g << 1) | (g >> 4);

			pDests[x] = (r << 11) | (g << 5) | b;

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

	gr_stream_frame(pixelbuf);

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

	if (pixelbuf) {
		SDL_DestroySurface(pixelbuf);
		pixelbuf = nullptr;
	}

	if (g_vBuffers != NULL) {
		free(g_vBuffers);
		g_vBuffers = NULL;
	}

	gr_stream_stop();
}
