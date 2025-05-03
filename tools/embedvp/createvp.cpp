/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */


#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>
#include <cstdio>
#include <vector>


typedef struct VP_HEADER {
	char id[4];
	Sint32 version;
	Sint32 index_offset;
	Sint32 num_files;
} VP_HEADER;

typedef struct VP_FILE {
	Sint32	offset;
	Sint32	size;
	char	filename[32];
	Sint32	write_time;
} VP_FILE;

SDL_COMPILE_TIME_ASSERT(VP_HEADER, sizeof(VP_HEADER) == 16);
SDL_COMPILE_TIME_ASSERT(VP_FILE, sizeof(VP_FILE) == 44);

SDL_IOStream *vp_out = nullptr;

Sint32 Total_size = SDL_static_cast(Sint32, sizeof(VP_HEADER));	// start with header size

#define BLOCK_SIZE (1024*1024)
#define VERSION_NUMBER 2

uint8_t data_block[BLOCK_SIZE];

std::vector<VP_FILE> file_index;


void write_header()
{
	// jump to start of file
	SDL_SeekIO(vp_out, 0, SDL_IO_SEEK_SET);

	// write header
	SDL_IOprintf(vp_out, "%s", "VPVP");
	SDL_WriteS32LE(vp_out, VERSION_NUMBER);
	SDL_WriteS32LE(vp_out, Total_size);
	SDL_WriteS32LE(vp_out, SDL_static_cast(Sint32, file_index.size()));
}

void write_index()
{
	// jump to end of file
	SDL_SeekIO(vp_out, 0, SDL_IO_SEEK_END);

	// write index
	for (const auto &item : file_index) {
		SDL_WriteS32LE(vp_out, item.offset);
		SDL_WriteS32LE(vp_out, item.size);
		SDL_WriteIO(vp_out, item.filename, sizeof(item.filename));
		SDL_WriteS32LE(vp_out, item.write_time);
	}
}

void add_index_entry(const VP_FILE &entry)
{
	file_index.push_back(entry);
}

void add_file(const char *filespec, const char *filename, const SDL_PathInfo &pinfo)
{
	VP_FILE item{};
	size_t nbytes;

	if (pinfo.size == 0) {
		return;
	}

	if (SDL_strlen(filename) > 31) {
		return;
	}

	auto fp = SDL_IOFromFile(filespec, "rb");

	if ( !fp ) {
		return;
	}

	// add file data to vp
	do {
		nbytes = SDL_ReadIO(fp, data_block, SDL_arraysize(data_block));

		if (nbytes > 0) {
			SDL_WriteIO(vp_out, data_block, nbytes);
		}
	} while (nbytes > 0);

	SDL_CloseIO(fp);

	// add index entry
	SDL_strlcpy(item.filename, filename, SDL_arraysize(item.filename));
	item.offset = Total_size;
	item.size = SDL_static_cast(Sint32, pinfo.size);
	item.write_time = SDL_static_cast(Sint32, SDL_NS_TO_SECONDS(pinfo.modify_time));

	add_index_entry(item);

	// increment total size
	Total_size += SDL_static_cast(Sint32, pinfo.size);
}

void add_directory(const char *dirname)
{
	VP_FILE item{};

	SDL_strlcpy(item.filename, dirname, SDL_arraysize(item.filename));
	item.offset = Total_size;

	add_index_entry(item);
}

int compare_strings(const void *a, const void *b) {
	return SDL_strcasecmp(*(const char **)a, *(const char **)b);
}

void pack_directory(const char *filespec, const char *dirname)
{
	SDL_PathInfo pinfo{};
	char fpath[512]{};
	int count = 0;

	auto list = SDL_GlobDirectory(filespec, "*", 0, &count);

	if ( !list ) {
		return;
	}

	// sort list results so they're always in alphabetical order
	SDL_qsort(list, count, sizeof(char*), compare_strings);

	// add index entry
	add_directory(dirname);

	for (int i = 0; list[i]; i++) {
#ifdef SDL_PLATFORM_WINDOWS
		SDL_snprintf(fpath, SDL_arraysize(fpath), "%s\\%s", filespec, list[i]);
#else
		SDL_snprintf(fpath, SDL_arraysize(fpath), "%s/%s", filespec, list[i]);
#endif

		if ( !SDL_GetPathInfo(fpath, &pinfo) ) {
			continue;
		}

		if ( SDL_strcasestr(list[i], ".vp") ) {
			continue;
		}

		if (pinfo.type == SDL_PATHTYPE_FILE) {
			add_file(fpath, list[i], pinfo);
		} else if (pinfo.type == SDL_PATHTYPE_DIRECTORY) {
			pack_directory(fpath, list[i]);
		}
	}

	if (SDL_strlen(dirname)) {
		add_directory("..");
	}

	SDL_free(list);
}

bool data_specified(const char *filespec)
{
	if (SDL_strcasestr(filespec, "/data")) {
		return true;
	}

#ifdef SDL_PLATFORM_WINDOWS
	if (SDL_strcasestr(filespec, "\\data")) {
		return true;
	}
#endif

	return false;
}

extern "C"
int main(int argc, char *argv[])
{
	SDL_PathInfo pinfo{};

	if (argc != 3) {
		printf("Usage: createvp <in_path> <out_vp_file>\n");
		return EXIT_FAILURE;
	}

	if ( !SDL_GetPathInfo(argv[1], &pinfo) ) {
		printf("Unable to process input directory!\n");
		return EXIT_FAILURE;
	}

	if (pinfo.type != SDL_PATHTYPE_DIRECTORY) {
		printf("Input path is not a directory!\n");
		return EXIT_FAILURE;
	}

	const char *input = argv[1];
	const char *output = argv[2];

	vp_out = SDL_IOFromFile(output, "wb");

	if (vp_out == NULL) {
		printf("Unable to open output file for writing!\n");
		return EXIT_FAILURE;
	}

	write_header();
	pack_directory(input, data_specified(input) ? "data" : "");
	write_header();

	write_index();

	SDL_FlushIO(vp_out);
	SDL_CloseIO(vp_out);

	return EXIT_SUCCESS;
}
