/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/

#include <string>

#include "pstypes.h"
#include "oal.h"
#include "osregistry.h"


static int OAL_capture_recording = 0;

struct capture_buffer {
	uint samples_per_second;
	uint bits_per_sample;
	uint n_channels;
	uint block_align;

	ALenum format;
	ALsizei buffer_size;
};

static capture_buffer Capture;

static ALCdevice *al_capture_device = NULL;
static std::string CaptureDevice;


void oal_capture_release_buffer()
{
	if (al_capture_device != NULL) {
		alcCaptureCloseDevice(al_capture_device);
		al_capture_device = NULL;
	}
}

// create a capture buffer with the specified format
// exit:	0	->		buffer created successfully
//			!0	->		error creating the buffer
int oal_capture_create_buffer(int freq, int bits_per_sample, int nchannels, int nseconds)
{
	ALenum al_format = AL_FORMAT_MONO8;
	ALsizei buf_size = freq * nseconds;

	SDL_assert( (nchannels == 1) || (nchannels == 2) );
	SDL_assert( (bits_per_sample == 8) || (bits_per_sample == 16) );

	if ( !oal_is_initted() ) {
		return -1;
	}

	if (nchannels == 1) {
		if (bits_per_sample == 8)  {
			al_format = AL_FORMAT_MONO8;
		} else if (bits_per_sample == 16) {
			al_format = AL_FORMAT_MONO16;
		}
	} else if (nchannels == 2) {
		if (bits_per_sample == 8) {
			al_format = AL_FORMAT_STEREO8;
		} else if (bits_per_sample == 16) {
			al_format = AL_FORMAT_STEREO16;
		}
	}

	al_capture_device = alcCaptureOpenDevice(CaptureDevice.c_str(), freq, al_format, buf_size);

	if (al_capture_device == NULL) {
		return -1;
	}

	// this gets around hang-on-close bug on Windows
	alcCaptureStart(al_capture_device);
	alcCaptureStop(al_capture_device);

	if ( alcGetError(al_capture_device) != ALC_NO_ERROR ) {
		alcCaptureCloseDevice(al_capture_device);

		return -1;
	}

	Capture.format = al_format;
	Capture.bits_per_sample = bits_per_sample;
	Capture.n_channels = nchannels;
	Capture.samples_per_second = freq;
	Capture.block_align = (nchannels * bits_per_sample) / 8;

	return 0;
}

void oal_capture_init()
{
	const char *ptr = NULL;
	ALCdevice *tdevice = NULL;

	ptr = os_config_read_string("Audio", "CaptureDevice", "default");

	if ( ptr && !SDL_strcasecmp(ptr, "default") ) {
		ptr = NULL;
	}

	tdevice = alcCaptureOpenDevice(ptr, 11025, AL_FORMAT_MONO8, 11025 * 2);

	if (tdevice == NULL) {
		tdevice = alcCaptureOpenDevice(NULL, 11025, AL_FORMAT_MONO8, 11025 * 2);

		if (tdevice == NULL) {
			mprintf(("  Capture device  : * Unavailable *\n"));

			return;
		}
	}

	if ( alcGetError(tdevice) != ALC_NO_ERROR ) {
		mprintf(("  Capture device  : * Unavailable *\n"));
		alcCaptureCloseDevice(tdevice);

		return;
	}

	ptr = alcGetString(tdevice, ALC_CAPTURE_DEVICE_SPECIFIER);
	SDL_assert( ptr );

	mprintf(("  Capture device  : %s\n", ptr));

	CaptureDevice = ptr;

	// this gets around hang-on-close bug on Windows
	alcCaptureStart(tdevice);
	alcCaptureStop(tdevice);

	alcCaptureCloseDevice(tdevice);
}

int oal_capture_supported()
{
	return oal_is_initted();
}

// start recording into the buffer
int oal_capture_start_record()
{
	if ( !oal_is_initted() ) {
		return -1;
	}

	if (OAL_capture_recording) {
		return -1;
	}

	alcCaptureStart(al_capture_device);

	OAL_capture_recording = 1;

//	nprintf(("Alan","RTVOICE => start record\n"));

	return 0;
}

// stop recording into the buffer
int oal_capture_stop_record()
{
	if ( !oal_is_initted() ) {
		return -1;
	}

	if ( !OAL_capture_recording ) {
		return -1;
	}

	alcCaptureStop(al_capture_device);

	OAL_capture_recording = 0;

//	nprintf(("Alan","RTVOICE => stop record\n"));

	return 0;
}

void oal_capture_close()
{
	oal_capture_stop_record();

	if (al_capture_device != NULL) {
		alcCaptureCloseDevice(al_capture_device);
		al_capture_device = NULL;
	}
}

// return the max buffer size
int oal_capture_max_buffersize()
{
	if ( !oal_is_initted() ) {
		return 0;
	}

	ALCsizei num_samples = 0;

	alcGetIntegerv(al_capture_device, ALC_CAPTURE_SAMPLES, sizeof(ALCsizei), &num_samples);

	if (alcGetError(al_capture_device) != ALC_NO_ERROR) {
		return 0;
	}

	return (num_samples * Capture.block_align);
}

// retrieve the recorded voice data
int oal_capture_get_raw_data(ubyte *outbuf, uint max_size)
{
	if ( !oal_is_initted() ) {
		return 0;
	}

	if (outbuf == NULL) {
		return 0;
	}

	ALCsizei num_samples = 0;

	alcGetIntegerv(al_capture_device, ALC_CAPTURE_SAMPLES, sizeof(ALCsizei), &num_samples);

	if (num_samples <= 0) {
		return 0;
	}

	ALCsizei max_buf_size = min(num_samples, ALsizei(max_size / Capture.block_align));

	alcCaptureSamples(al_capture_device, outbuf, max_buf_size);

	if (alcGetError(al_capture_device) != ALC_NO_ERROR) {
		return 0;
	}

	return (int)max_buf_size * Capture.block_align;
}
