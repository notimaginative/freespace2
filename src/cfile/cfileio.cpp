/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "cfile.h"

static Sint64 cf_io_size(void *userdata)
{
	auto filep = reinterpret_cast<CFILE *>(userdata);

	if (filep) {
		return cfilelength(filep);
	}

	return -1;
}

static Sint64 cf_io_seek(void *userdata, Sint64 offset, SDL_IOWhence whence)
{
	auto filep = reinterpret_cast<CFILE *>(userdata);

	if (filep) {
		if (cfseek(filep, offset, whence)) {
			return cftell(filep);
		}
	}

	return -1;
}

static size_t cf_io_read(void *userdata, void *ptr, size_t size, SDL_IOStatus *status)
{
	auto filep = reinterpret_cast<CFILE *>(userdata);

	if (filep) {
		const size_t bytes = static_cast<size_t>(cfread(ptr, 1, size, filep));

		if (bytes < size) {
			*status = cfeof(filep) ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_ERROR;
		}

		return bytes;
	}

	return 0;
}

static size_t cf_io_write(void *userdata, const void *ptr, size_t size, SDL_IOStatus *status)
{
	auto filep = reinterpret_cast<CFILE *>(userdata);

	if (filep) {
		size_t bytes = static_cast<size_t>(cfwrite(ptr, 1, size, filep));

		if (bytes < size) {
			*status = SDL_IO_STATUS_ERROR;
		}

		return bytes;
	}

	return 0;
}

static bool cf_io_flush(void *userdata, SDL_IOStatus *status)
{
	auto filep = reinterpret_cast<CFILE *>(userdata);

	if (filep) {
		return cflush(filep);
	}

	return false;
}

static bool cf_io_close(void *userdata)
{
	auto filep = reinterpret_cast<CFILE *>(userdata);

	if (filep) {
		bool rval = cfclose(filep);
		SDL_free(userdata);

		return rval;
	}

	return false;
}

/// Opens a file for use with SDL3 IO stream
/// - Parameters:
///   - file_path: name of file to open (may be path+name)
///   - mode: how the file should be opened (see fopen())
///   - dir_type: location option (one of CF_TYPE_* defines)
///   - localize: get localized version of file
/// - Returns: SDL_IOStream ptr on success or nullptr on failure
SDL_IOStream *cfopen_io(const char *file_path, const char *mode, int dir_type, bool localize)
{
	SDL_IOStreamInterface iface;
	CFILE *userdata = nullptr;

	auto filep = cfopen(file_path, mode, dir_type, localize);

	if ( !filep ) {
		return nullptr;
	}

	userdata = reinterpret_cast<CFILE*>(SDL_malloc(sizeof(CFILE)));

	SDL_memcpy(userdata, filep, sizeof(CFILE));

	SDL_zero(iface);
	SDL_INIT_INTERFACE(&iface);

	iface.size = cf_io_size;
	iface.seek = cf_io_seek;
	iface.read = cf_io_read;
	iface.write = cf_io_write;
	iface.flush = cf_io_flush;
	iface.close = cf_io_close;

	auto stream = SDL_OpenIO(&iface, userdata);

	if ( !stream ) {
		cfclose(filep);
		SDL_free(userdata);
		return nullptr;
	}

	return stream;
}
