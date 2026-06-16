/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Sound/rtvoice.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * C module file for real-time voice
 *
 * $Log$
 * Revision 1.4  2002/06/09 04:41:27  relnev
 * added copyright header
 *
 * Revision 1.3  2002/05/27 04:04:43  relnev
 * 155 undefined references left
 *
 * Revision 1.2  2002/05/07 03:16:52  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:10  root
 * Initial import.
 *
 * 
 * 2     10/07/98 10:54a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 24    4/24/98 2:17a Lawrance
 * Clear out record buffer when recording begins
 * 
 * 23    4/21/98 4:44p Dave
 * Implement Vasudan ships in multiplayer. Added a debug function to bash
 * player rank. Fixed a few rtvoice buffer overrun problems. Fixed ui
 * problem in options screen. 
 * 
 * 22    4/21/98 10:30a Dave
 * allocate 8 second buffers for rtvoice
 * 
 * 21    4/17/98 5:27p Dave
 * More work on the multi options screen. Fixed many minor ui todo bugs.
 * 
 * 20    4/17/98 10:38a Lawrance
 * reduce num output streams to 1
 * 
 * 19    3/25/98 9:56a Dave
 * Increase buffer size to handle 8 seconds of voice data.
 * 
 * 18    3/22/98 7:13p Lawrance
 * Get streaming of recording voice working
 * 
 * 17    3/09/98 5:22p Dave
 * Fixed a rtvoice bug which caused bogus output when given size 0 input.
 * 
 * 16    2/26/98 2:54p Lawrance
 * Don't recreate capture buffer each time recording starts... just use
 * one.
 * 
 * 15    2/24/98 11:56p Lawrance
 * Change real-time voice code to provide the uncompressed size on decode.
 * 
 * 14    2/24/98 10:13p Dave
 * Put in initial support for multiplayer voice streaming.
 * 
 * 13    2/24/98 10:47a Lawrance
 * Play voice through normal channels
 * 
 * 12    2/23/98 6:54p Lawrance
 * Make interface to real-time voice more generic and useful.
 * 
 * 11    2/19/98 12:47a Lawrance
 * Use a global code_info
 * 
 * 10    2/16/98 7:31p Lawrance
 * get compression/decompression of voice working
 * 
 * 9     2/15/98 11:59p Lawrance
 * Change the order of some code when opening a stream
 * 
 * 8     2/15/98 11:10p Lawrance
 * more work on real-time voice system
 * 
 * 7     2/15/98 4:43p Lawrance
 * work on real-time voice
 * 
 * 6     2/09/98 8:07p Lawrance
 * get buffer create working
 * 
 * 5     2/04/98 6:08p Lawrance
 * Read function pointers from dsound.dll, further work on
 * DirectSoundCapture.
 * 
 * 4     2/03/98 11:53p Lawrance
 * Adding support for DirectSoundCapture
 * 
 * 3     2/03/98 4:07p Lawrance
 * check return codes from waveIn calls
 * 
 * 2     1/31/98 5:48p Lawrance
 * Start on real-time voice recording
 *
 * $NoKeywords: $
 */

#include "pstypes.h"
#include "sound.h"
#include "codec1.h"
#include "rtvoice.h"

static const SDL_AudioSpec Rtv_audiospec = { SDL_AUDIO_U8, 1, 11025 };

static SDL_AudioStream *Rtv_recording_stream = nullptr;
static SDL_AudioStream *Rtv_playback_stream = nullptr;

static const int Rtv_do_compression=1;					// flag to indicate whether compression should be done

#define RTV_BUFFER_TIME		8						// length of buffer in seconds	

static int Rtv_recording_inited=0;				// The input stream has been inited
static int Rtv_playback_inited=0;				// The output stream has been inited

static int Rtv_playback_data_size = 0;				// To help determine playback position

static int Rtv_recording=0;						// Voice is currently being recorded

static struct	t_CodeInfo Rtv_code_info;		// Parms will need to be transmitted with packets

// recording timer data
static SDL_TimerID Rtv_record_timer_id;		// unique id for callback timer
static int Rtv_callback_time;			// callback time in ms

void (*Rtv_callback)();

// recording/encoding buffers
static unsigned char *Rtv_capture_raw_buffer;
static unsigned char *Rtv_capture_compressed_buffer;
static int Rtv_capture_compressed_buffer_size;
static int Rtv_capture_raw_buffer_size;

static unsigned char	*Encode_buffer1=NULL;
static unsigned char	*Encode_buffer2=NULL;

// playback/decoding buffers
static unsigned char *Rtv_playback_uncompressed_buffer;
static int Rtv_playback_uncompressed_buffer_size;

static unsigned char *Decode_buffer=NULL;
static int Decode_buffer_size;

/////////////////////////////////////////////////////////////////////////////////////////////////
// RECORD/ENCODE
/////////////////////////////////////////////////////////////////////////////////////////////////

Uint32 SDLCALL TimeProc(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
	if ( !Rtv_callback ) {
		SDL_RemoveTimer(Rtv_record_timer_id);
		Rtv_record_timer_id = 0;

		return 0;
	}

	Rtv_callback();

	if (Rtv_callback_time) {
		return interval;
	} else {
		SDL_RemoveTimer(Rtv_record_timer_id);
		Rtv_record_timer_id = 0;

		return 0;
	}
}

// input:	qos => new quality of service (1..10)
void rtvoice_set_qos(int qos)
{
	InitEncoder(e_cCodec1, qos, Encode_buffer1, Encode_buffer2);
}

// Init the recording portion of the real-time voice system
// input:	qos	=> quality of service (1..10) 1 is highest compression, 10 is highest quality
//	exit:	0	=>	success
//			!0	=>	failure, recording not possible
int rtvoice_init_recording(int qos)
{
	if ( !Rtv_recording_inited ) {
		Rtv_recording_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
														 &Rtv_audiospec,
														 nullptr, nullptr);

		if ( !Rtv_recording_stream ) {
			return -1;
		}

		Rtv_capture_raw_buffer_size = Rtv_audiospec.freq * (RTV_BUFFER_TIME) * SDL_AUDIO_BYTESIZE(Rtv_audiospec.format);

		if ( Encode_buffer1 ) {
			free(Encode_buffer1);
			Encode_buffer1=NULL;
		}

		Encode_buffer1 = (unsigned char*)malloc(Rtv_capture_raw_buffer_size);
		SDL_assert(Encode_buffer1);

		if ( Encode_buffer2 ) {
			free(Encode_buffer2);
			Encode_buffer2=NULL;
		}
		Encode_buffer2 = (unsigned char*)malloc(Rtv_capture_raw_buffer_size);
		SDL_assert(Encode_buffer2);

		// malloc out the voice data buffer for raw (uncompressed) recorded sound
		if ( Rtv_capture_raw_buffer ) {
			free(Rtv_capture_raw_buffer);
			Rtv_capture_raw_buffer=NULL;
		}
		Rtv_capture_raw_buffer = (unsigned char*)malloc(Rtv_capture_raw_buffer_size);

		// malloc out voice data buffer for compressed recorded sound
		if ( Rtv_capture_compressed_buffer ) {
			free(Rtv_capture_compressed_buffer);
			Rtv_capture_compressed_buffer=NULL;
		}
		Rtv_capture_compressed_buffer_size=Rtv_capture_raw_buffer_size;	// be safe and allocate same as uncompressed
		Rtv_capture_compressed_buffer = (unsigned char*)malloc(Rtv_capture_compressed_buffer_size);

		InitEncoder(e_cCodec1, qos, Encode_buffer1, Encode_buffer2);

		Rtv_recording_inited=1;
	}
	return 0;
}

// Stop a stream from recording
void rtvoice_stop_recording()
{
	if ( !Rtv_recording ) {
		return;
	}

	SDL_PauseAudioStreamDevice(Rtv_recording_stream);

	if ( Rtv_record_timer_id ) {
		SDL_RemoveTimer(Rtv_record_timer_id);
		Rtv_record_timer_id = 0;
	}

	Rtv_recording=0;
}

// Close down the real-time voice recording system
void rtvoice_close_recording()
{
	if ( Rtv_recording ) {
		rtvoice_stop_recording();
	}

	if ( Encode_buffer1 ) {
		free(Encode_buffer1);
		Encode_buffer1=NULL;
	}

	if ( Encode_buffer2 ) {
		free(Encode_buffer2);
		Encode_buffer2=NULL;
	}

	if ( Rtv_capture_raw_buffer ) {
		free(Rtv_capture_raw_buffer);
		Rtv_capture_raw_buffer=NULL;
	}

	if ( Rtv_capture_compressed_buffer ) {
		free(Rtv_capture_compressed_buffer);
		Rtv_capture_compressed_buffer=NULL;
	}

	if (Rtv_recording_stream) {
		SDL_DestroyAudioStream(Rtv_recording_stream);
		Rtv_recording_stream = nullptr;
	}

	Rtv_recording_inited=0;
}

// Open a stream for recording (recording begins immediately)
// exit:	0	=>	success
//			!0	=>	failure
int rtvoice_start_recording( void (*user_callback)(), int callback_time ) 
{
	if ( !Rtv_recording_inited ) {
		return -1;
	}

	if ( Rtv_recording ) {
		return -1;
	}

	SDL_ClearAudioStream(Rtv_recording_stream);

	if ( !SDL_ResumeAudioStreamDevice(Rtv_recording_stream) ) {
		return -1;
	}

	if ( user_callback ) {
		Rtv_record_timer_id = SDL_AddTimer(callback_time, TimeProc, NULL);

		if ( !Rtv_record_timer_id ) {
			SDL_PauseAudioStreamDevice(Rtv_recording_stream);
			return -1;
		}
		Rtv_callback = user_callback;
		Rtv_callback_time = callback_time;
	} else {
		Rtv_callback = NULL;
		Rtv_record_timer_id = 0;
	}

	Rtv_recording=1;
	return 0;
}

// compress voice data using specialized codec
int rtvoice_compress(unsigned char *data_in, int size_in, unsigned char *data_out, int size_out)
{
	int		compressed_size;

	Rtv_code_info.Code = e_cCodec1;
	Rtv_code_info.Gain = 0;

	compressed_size = 0;
	if(size_in <= 0){
		nprintf(("Network","RTVOICE => 0 bytes size in !\n"));		
	} else {
		compressed_size = Encode(data_in, data_out, size_in, size_out, &Rtv_code_info);

		nprintf(("SOUND","RTVOICE => Sound compressed to %d bytes (%0.2f percent)\n", compressed_size, (compressed_size*100.0f)/size_in));
	}

	return compressed_size;
}

// Retrieve the recorded voice data
// input:	outbuf					=>		output parameter, recorded voice stored here
//				compressed_size		=>		output parameter, size in bytes of recorded voice after compression
//				uncompressed_size		=>		output parameter, size in bytes of recorded voice before compression
//				gain						=>		output parameter, gain value which must be passed to decoder
//				outbuf_raw				=>		output optional parameter, pointer to the raw sound data making up the compressed chunk
//				outbuf_size_raw		=>		output optional parameter, size of the outbuf_raw buffer
//
// NOTE: function converts voice data into compressed format
void rtvoice_get_data(unsigned char **outbuf, int *compressed_size, int *uncompressed_size, double *gain, unsigned char **outbuf_raw, int *outbuf_size_raw)
{
	int raw_size, csize;

	*compressed_size=0;
	*uncompressed_size=0;
	*outbuf=NULL;

	raw_size = SDL_GetAudioStreamData(Rtv_recording_stream, Rtv_capture_raw_buffer,
									  Rtv_capture_raw_buffer_size);

	*uncompressed_size = raw_size;

	// compress voice data
	if ( Rtv_do_compression ) {
		csize = rtvoice_compress(Rtv_capture_raw_buffer, raw_size, Rtv_capture_compressed_buffer, Rtv_capture_compressed_buffer_size);
		*gain = Rtv_code_info.Gain;
		*compressed_size = csize;
		*outbuf = Rtv_capture_compressed_buffer;
	} else {
		*gain = Rtv_code_info.Gain;
		*compressed_size = raw_size;
		*outbuf = Rtv_capture_raw_buffer;
	}

	// NOTE : if we are not doing compression, then the raw buffer and size are going to be the same as the compressed
	//        buffer and size

	// assign the raw buffer and size if necessary
	if(outbuf_raw != NULL){
		*outbuf_raw = Rtv_capture_raw_buffer;
	}
	if(outbuf_size_raw != NULL){
		*outbuf_size_raw = raw_size;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// DECODE/PLAYBACK
/////////////////////////////////////////////////////////////////////////////////////////////////

// return the size that the decode buffer should be
int rtvoice_get_decode_buffer_size()
{
	return Decode_buffer_size;
}

// uncompress the data into PCM format
void rtvoice_uncompress(unsigned char *data_in, int size_in, double gain, unsigned char *data_out, int size_out)
{
	if (Rtv_do_compression) {
		Rtv_code_info.Gain = gain;
		Decode(&Rtv_code_info, data_in, data_out, size_in, size_out);
	} else {
		SDL_memcpy(data_out, data_in, SDL_min(size_in, size_out));
	}
}

// Close down the real-time voice playback system
void rtvoice_close_playback()
{
	if ( Decode_buffer ) {
		free(Decode_buffer);
		Decode_buffer=NULL;
	}

	if ( Rtv_playback_uncompressed_buffer ) {
		free(Rtv_playback_uncompressed_buffer);
		Rtv_playback_uncompressed_buffer=NULL;
	}

	if (Rtv_playback_stream) {
		SDL_DestroyAudioStream(Rtv_playback_stream);
		Rtv_playback_stream = nullptr;
	}

	Rtv_playback_inited=0;
}

// Init the playback portion of the real-time voice system
//	exit:	0	=>	success
//			!0	=>	failure, playback not possible
int rtvoice_init_playback()
{
	if ( !Rtv_playback_inited ) {
		Rtv_playback_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
														&Rtv_audiospec,
														nullptr, nullptr);

		if ( !Rtv_playback_stream ) {
			return -1;
		}

		Decode_buffer_size = Rtv_audiospec.freq * (RTV_BUFFER_TIME) * SDL_AUDIO_BYTESIZE(Rtv_audiospec.format);

		if ( Decode_buffer ) {
			free(Decode_buffer);
			Decode_buffer=NULL;
		}

		Decode_buffer = (unsigned char*)malloc(Decode_buffer_size);
		SDL_assert(Decode_buffer);

		if ( Rtv_playback_uncompressed_buffer ) {
			free(Rtv_playback_uncompressed_buffer);
			Rtv_playback_uncompressed_buffer=NULL;
		}

		Rtv_playback_uncompressed_buffer_size=Decode_buffer_size;
		Rtv_playback_uncompressed_buffer = (unsigned char*)malloc(Rtv_playback_uncompressed_buffer_size);
		SDL_assert(Rtv_playback_uncompressed_buffer);

		InitDecoder(1, Decode_buffer); 

		Rtv_playback_inited=1;
	}

	return 0;
}

bool rtvoice_is_playback_active(int /* index */)
{
	return (SDL_GetAudioStreamQueued(Rtv_playback_stream) > 0);
}

int rtvoice_get_playback_position(int /* index */)
{
	int offset = Rtv_playback_data_size - SDL_GetAudioStreamQueued(Rtv_playback_stream);

	// NEVER return a negative value from here
	return (offset > 0) ? offset : 0;
}

int rtvoice_find_free_output_buffer()
{
	return 0;
}

// Open a stream for real-time voice output
int rtvoice_create_playback_buffer()
{
	return Rtv_playback_stream ? 0 : -1;
}

void rtvoice_stop_playback(int /* index */)
{
	SDL_ClearAudioStream(Rtv_playback_stream);
}

void rtvoice_stop_playback_all()
{
	rtvoice_stop_playback(0);
}

// Close a stream that was opened for real-time voice output
void rtvoice_free_playback_buffer(int /* index */)
{
}

// Play compressed sound data
// exit:	>=0	=>	handle to playing sound
//			-1		=>	error, voice not played
int rtvoice_play_compressed(int /* index */, unsigned char *data, int size, int uncompressed_size, double gain)
{
	// Stop any currently playing voice output
	rtvoice_stop_playback_all();

	SDL_assert(uncompressed_size <= Rtv_playback_uncompressed_buffer_size);

	// uncompress the data into PCM format
	if ( Rtv_do_compression ) {
		rtvoice_uncompress(data, size, gain, Rtv_playback_uncompressed_buffer, uncompressed_size);
	}

	SDL_SetAudioStreamGain(Rtv_playback_stream, Master_voice_volume);

	SDL_PutAudioStreamData(Rtv_playback_stream, Rtv_playback_uncompressed_buffer, uncompressed_size);
	SDL_FlushAudioStream(Rtv_playback_stream);	// no more data will be added

	Rtv_playback_data_size = uncompressed_size;

	SDL_ResumeAudioStreamDevice(Rtv_playback_stream);

	return 0;
}

// Play uncompressed (raw) sound data
// exit:	>=0	=>	handle to playing sound
//			-1		=>	error, voice not played
int rtvoice_play_uncompressed(int /* index */, unsigned char *data, int size)
{
	// Stop any currently playing voice output
	rtvoice_stop_playback_all();

	SDL_SetAudioStreamGain(Rtv_playback_stream, Master_voice_volume);

	SDL_PutAudioStreamData(Rtv_playback_stream, data, size);
	SDL_FlushAudioStream(Rtv_playback_stream);	// no more data will be added

	Rtv_playback_data_size = size;

	SDL_ResumeAudioStreamDevice(Rtv_playback_stream);

	return 0;
}

