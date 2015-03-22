/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/

#ifndef _OAL_CAPTURE_H
#define _OAL_CAPTURE_H

void oal_capture_init();

int oal_capture_create_buffer(int freq, int bits_per_sample, int nchannels, int nseconds);
void oal_capture_release_buffer();

int oal_capture_supported();

int oal_capture_start_record();
int oal_capture_stop_record();

void oal_capture_close();

int oal_capture_max_buffersize();
int oal_capture_get_raw_data(ubyte *outbuf, uint max_size);

#endif	// _OAL_CAPTURE_H
