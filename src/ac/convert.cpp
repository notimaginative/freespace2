/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/AC/convert.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * C module for the conversion of standard animation files to our own ANIM format.
 * This is where all the real code for this application is located really.
 *
 * $Log$
 * Revision 1.2  2002/06/09 04:41:15  relnev
 * added copyright header
 *
 * Revision 1.1.1.1  2002/05/03 03:28:11  root
 * Initial import.
 *
 * 
 * 2     10/23/98 6:03p Dave
 * 
 * 1     10/23/98 5:34p Dave
 * 
 * 3     10/22/98 6:14p Dave
 * Optimized some #includes in Anim folder. Put in the beginnings of
 * parse/localization support for externalized strings and tstrings.tbl
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:48a Dave
 * 
 * 12    6/23/98 2:52p Hoffoss
 * Changed code so AC compiles once again.
 * 
 * 11    7/20/97 6:57p Lawrance
 * supporting new RLE format
 * 
 * 10    6/25/97 11:57a Lawrance
 * forgot linefeed
 * 
 * 9     6/25/97 11:49a Lawrance
 * don't assert if PCX read fails, print warning
 * 
 * 8     5/21/97 2:26p Lawrance
 * bug fix: not using correct header size
 * 
 * 7     5/21/97 11:06a Lawrance
 * enabling a user-defined transparent value
 * 
 * 6     5/19/97 3:21p Lawrance
 * add fps parm, version num to anim header
 * 
 * 5     2/25/97 5:18p Lawrance
 * force_key_frame now numbering from 1 for PCXs as well as AVIs
 * 
 * 4     2/20/97 1:47p Lawrance
 * adding dots when making frames
 * 
 * 3     2/20/97 10:30a Hoffoss
 * Added in support for forcing a key frame.
 * 
 * 2     2/19/97 9:26p Lawrance
 * added force_key_frame global
 * 
 * 1     2/19/97 7:14p Lawrance
 * 
 * 14    2/19/97 4:21p Lawrance
 * took out unnecessary #include BmpMan.h
 * 
 * 13    2/19/97 3:59p Lawrance
 * using pcxutils to load bitmaps, not bmpman
 * 
 * 12    2/17/97 2:59p Lawrance
 * integrating into game
 * 
 * 11    2/17/97 3:02p Hoffoss
 * Added headers to files, and implemented key frame dialog stuff.
 * 
 * 10    2/14/97 10:38p Lawrance
 * fixing bugs
 * 
 * 9     2/14/97 9:47p Hoffoss
 * fixed bugs.
 * 
 * 8     2/14/97 8:44p Lawrance
 * fixed bug with saving header
 * 
 * 7     2/14/97 8:04p Hoffoss
 * Fixed bug.
 * 
 * 6     2/14/97 7:44p Lawrance
 * fixed some bugs in loading an AVI
 * 
 * 5     2/14/97 7:33p Lawrance
 * added convert_avi_to_anim() function
 * 
 * 4     2/14/97 5:38p Hoffoss
 * Changes to get AnimCoverter project to compile and link.
 * 
 * 3     2/14/97 3:28p Hoffoss
 * Wrote functions to save an anim file.
 * 
 * 2     2/13/97 5:55p Lawrance
 * reading AVI / decompressing AVI functions in
 *
 * $NoKeywords: $
 */


#include "pstypes.h"
#include "convert.h"
#include "pcxutils.h"
#include "animplay.h"
#include "packunpack.h"

#define AVI_STREAM_F_USED	( 1 << 0 )

typedef struct _frame_index {
	unsigned int offset;
	unsigned int size;
} FRAMEINDEX;

typedef struct AVI_STREAM_TYPE {
	FILE		*pfile;
	FRAMEINDEX	*frame_index;
	int			num_frames;
	int			current_frame;
	int			w,h,bpp;
	int			min_compressed_buffer_size;
	ubyte			palette[768];
	char			filename[255];
	//ubyte			pal_translation[256];		// palette translation look-up table
	int		flags;
}	AVI_STREAM_TYPE;

#define AVIF_HASINDEX 0x00000010

typedef struct _avimainheader {
	unsigned int fcc;
	unsigned int cb;
	unsigned int dwMicroSecPerFrame;
	unsigned int dwMaxBytesPerSec;
	unsigned int dwPaddingGranularity;
	unsigned int dwFlags;
	unsigned int dwTotalFrames;
	unsigned int dwInitialFrames;
	unsigned int dwStreams;
	unsigned int dwSuggestedBufferSize;
	unsigned int dwWidth;
	unsigned int dwHeight;
	unsigned int dwReserved[4];
} AVIMAINHEADER;

#define AVISF_VIDEO_PALCHANGES 0x00010000

typedef struct _avistreamheader {
	unsigned int fcc;
	unsigned int cb;
	unsigned int fccType;
	unsigned int fccHandler;
	unsigned int dwFlags;
	unsigned short wPriority;
	unsigned short wLanguage;
	unsigned int dwInitialFrames;
	unsigned int dwScale;
	unsigned int dwRate;
	unsigned int dwStart;
	unsigned int dwLength;
	unsigned int dwSuggestedBufferSize;
	unsigned int dwQuality;
	unsigned int dwSampleSize;
	struct {
		short left;
		short top;
		short right;
		short bottom;
	} rcFrame;
} AVISTREAMHEADER;

#define BI_RLE8 0x00000001

typedef struct _bitmapinfo {
	struct {
		unsigned int biSize;
		int biWidth;
		int biHeight;
		unsigned short biPlanes;
		unsigned short biBitCount;
		unsigned int biCompression;
		unsigned int biSizeImage;
		int biXPelsPerMeter;
		int biYPelsPerMeter;
		unsigned int biClrUsed;
		unsigned int biClrImportant;
	} bmiHeader;

	struct {
		ubyte b;
		ubyte g;
		ubyte r;
		ubyte p;
	} bmiColors[256];
} BITMAPINFO;


// Internal function prototypes
int	AVI_stream_open(char* filename);
void	AVI_stream_close();
int	AVI_stream_get_frame(ubyte* buffer, int frame_number);
void	AVI_decompress_RLE8(ubyte* src, ubyte* dest, int w, int h);

// Global to file
static		AVI_STREAM_TYPE AVI_stream;	
static int	AVI_stream_inited = 0;

// Globals 
char	*anim_save_filename;
ubyte *cur_frame, *last_frame;
ubyte	anim_buffer[ANIM_BUFFER_MAX];
int	key_frame_rate;
int	force_key_frame;
int	total_key_frames;
int	anim_offset;
int	cur_frame_num;
int	Default_fps;
int	Use_custom_xparent_color;
rgb_triple	Xparent_color;

int	Compression_type;		// what kind of RLE compression is going to be used 
int	Key_frame_compression, Regular_frame_compression;

key_frame *first_frame;
FILE *anim_fp = NULL;
anim Anim;

// AVI_stream_init() is called only to clear USED flag of the AVI_stream structure
// and reset the current frame.
//
//	This does not need to be called explicity, since it will be called by AVI_stream_open()
// anyways.
//
void AVI_stream_init()
{
	AVI_stream.flags &= ~AVI_STREAM_F_USED;
	AVI_stream.current_frame = 0;
	AVI_stream_inited = 1;

	AVI_stream.pfile = NULL;
	AVI_stream.frame_index = NULL;
}

// AVI_stream_open() will open the AVI file and prepare it for reading, but will not 
// store any of the frame data. 
//
//	returns:   0 ==> success
//           !0 ==> could not open the AVI stream
//
// The filename is expected to be an absolute pathname (or file in the current working directory)
//
int AVI_stream_open(char* filename)
{
	if ( !AVI_stream_inited )
		AVI_stream_init();

	FILE *pfile = NULL;
	int id = 0;
	unsigned int tag = 0, size = 0;
	unsigned int s_tag, tmp;
	AVIMAINHEADER avi_header;
	AVISTREAMHEADER stream_header;
	BITMAPINFO bitmap_header;
	long file_size = 0, movi_offset = 0, next_chunk;

	SDL_assert( !(AVI_stream.flags & AVI_STREAM_F_USED) );

	SDL_zero(avi_header);
	SDL_zero(stream_header);
	SDL_zero(bitmap_header);

	pfile = fopen(filename, "rb");

	if (pfile == NULL) {
		printf("AVI ==> Unable to open %s", filename);
		return -1;
	}

	// get file size
	fseek(pfile, 0, SEEK_END);
	file_size = ftell(pfile);
	fseek(pfile, 0, SEEK_SET);

	// check for valid file type
	fread(&id, 1, 4, pfile);
	id = INTEL_INT(id);

	// 'RIFF'
	if (id != 0x46464952) {
		printf("Not a RIFF file '%s'\n", filename);
		fclose(pfile);
		return -1;
	}

	// skip RIFF size
	fread(&id, 1, 4, pfile);

	// check for valid RIFF type
	fread(&id, 1, 4, pfile);
	id = INTEL_INT(id);

	// 'AVI '
	if (id != 0x20495641) {
		printf("Not an AVI file '%s'\n", filename);
		fclose(pfile);
		return -1;
	}

	// used for main 'LIST' chunks
	long offset_tmp = 0;

	// parse WAVE tags
	while ( ftell(pfile) < file_size ) {
		fread(&tag, 1, 4, pfile);
		fread(&size, 1, 4, pfile);

		tag = INTEL_INT(tag);
		size = INTEL_INT(size);

		next_chunk = ftell(pfile) + size;

		switch (tag) {
			// 'LIST'
			case 0x5453494c: {
				// sub tag
				fread(&s_tag, 1, 4, pfile);
				s_tag = INTEL_INT(s_tag);

				switch (s_tag) {
					// 'hdrl'
					case 0x6c726468: {
						fread(&avi_header.fcc, 1, sizeof(int), pfile);
						fread(&avi_header.cb, 1, sizeof(int), pfile);
						fread(&avi_header.dwMicroSecPerFrame, 1, sizeof(int), pfile);
						fread(&avi_header.dwMaxBytesPerSec, 1, sizeof(int), pfile);
						fread(&avi_header.dwPaddingGranularity, 1, sizeof(int), pfile);
						fread(&avi_header.dwFlags, 1, sizeof(int), pfile);
						fread(&avi_header.dwTotalFrames, 1, sizeof(int), pfile);
						fread(&avi_header.dwInitialFrames, 1, sizeof(int), pfile);
						fread(&avi_header.dwStreams, 1, sizeof(int), pfile);
						fread(&avi_header.dwSuggestedBufferSize, 1, sizeof(int), pfile);
						fread(&avi_header.dwWidth, 1, sizeof(int), pfile);
						fread(&avi_header.dwHeight, 1, sizeof(int), pfile);
						fread(&avi_header.dwReserved, 1, sizeof(avi_header.dwReserved), pfile);

						avi_header.fcc = INTEL_INT(avi_header.fcc);
						avi_header.cb = INTEL_INT(avi_header.cb);
						avi_header.dwMicroSecPerFrame = INTEL_INT(avi_header.dwMicroSecPerFrame);
						avi_header.dwMaxBytesPerSec = INTEL_INT(avi_header.dwMaxBytesPerSec);
						avi_header.dwPaddingGranularity = INTEL_INT(avi_header.dwPaddingGranularity);
						avi_header.dwFlags = INTEL_INT(avi_header.dwFlags);
						avi_header.dwTotalFrames = INTEL_INT(avi_header.dwTotalFrames);
						avi_header.dwInitialFrames = INTEL_INT(avi_header.dwInitialFrames);
						avi_header.dwStreams = INTEL_INT(avi_header.dwStreams);
						avi_header.dwSuggestedBufferSize = INTEL_INT(avi_header.dwSuggestedBufferSize);
						avi_header.dwWidth = INTEL_INT(avi_header.dwWidth);
						avi_header.dwHeight = INTEL_INT(avi_header.dwHeight);

						// check for 'avih'
						SDL_assert(avi_header.fcc == 0x68697661);

						// we're stupid, can only handle a single stream
						if (avi_header.dwStreams != 1) {
							printf("AVI has more than one stream '%s'\n", filename);
							fclose(pfile);
							return -1;
						}

						// require index chunk (should be flagged as availble)
						if ( !(avi_header.dwFlags & AVIF_HASINDEX) ) {
							printf("AVI does not have index '%s'\n", filename);
							fclose(pfile);
							return -1;
						}

						// update next_chunk offset for sub-chunk
						offset_tmp = next_chunk;
						next_chunk = ftell(pfile);

						break;
					}

					// 'strl' - subchunk of 'hdrl'
					case 0x6c727473: {
						fread(&stream_header.fcc, 1, sizeof(int), pfile);
						fread(&stream_header.cb, 1, sizeof(int), pfile);
						fread(&stream_header.fccType, 1, sizeof(int), pfile);
						fread(&stream_header.fccHandler, 1, sizeof(int), pfile);
						fread(&stream_header.dwFlags, 1, sizeof(int), pfile);
						fread(&stream_header.wPriority, 1, sizeof(short), pfile);
						fread(&stream_header.wLanguage, 1, sizeof(short), pfile);
						fread(&stream_header.dwInitialFrames, 1, sizeof(int), pfile);
						fread(&stream_header.dwScale, 1, sizeof(int), pfile);
						fread(&stream_header.dwRate, 1, sizeof(int), pfile);
						fread(&stream_header.dwStart, 1, sizeof(int), pfile);
						fread(&stream_header.dwLength, 1, sizeof(int), pfile);
						fread(&stream_header.dwSuggestedBufferSize, 1, sizeof(int), pfile);
						fread(&stream_header.dwQuality, 1, sizeof(int), pfile);
						fread(&stream_header.dwSampleSize, 1, sizeof(int), pfile);
						fread(&stream_header.rcFrame.left, 1, sizeof(short), pfile);
						fread(&stream_header.rcFrame.top, 1, sizeof(short), pfile);
						fread(&stream_header.rcFrame.right, 1, sizeof(short), pfile);
						fread(&stream_header.rcFrame.bottom, 1, sizeof(short), pfile);

						stream_header.fcc = INTEL_INT(stream_header.fcc);
						stream_header.cb = INTEL_INT(stream_header.cb);
						stream_header.fccType = INTEL_INT(stream_header.fccType);
						stream_header.fccHandler = INTEL_INT(stream_header.fccHandler);
						stream_header.dwFlags = INTEL_INT(stream_header.dwFlags);
						stream_header.wPriority = INTEL_SHORT(stream_header.wPriority);
						stream_header.wLanguage = INTEL_SHORT(stream_header.wLanguage);
						stream_header.dwInitialFrames = INTEL_INT(stream_header.dwInitialFrames);
						stream_header.dwScale = INTEL_INT(stream_header.dwScale);
						stream_header.dwRate = INTEL_INT(stream_header.dwRate);
						stream_header.dwStart = INTEL_INT(stream_header.dwStart);
						stream_header.dwLength = INTEL_INT(stream_header.dwLength);
						stream_header.dwSuggestedBufferSize = INTEL_INT(stream_header.dwSuggestedBufferSize);
						stream_header.dwQuality = INTEL_INT(stream_header.dwQuality);
						stream_header.dwSampleSize = INTEL_INT(stream_header.dwSampleSize);
						stream_header.rcFrame.left = INTEL_SHORT(stream_header.rcFrame.left);
						stream_header.rcFrame.top = INTEL_SHORT(stream_header.rcFrame.top);
						stream_header.rcFrame.right = INTEL_SHORT(stream_header.rcFrame.right);
						stream_header.rcFrame.bottom = INTEL_SHORT(stream_header.rcFrame.bottom);

						// check for 'strh'
						SDL_assert(stream_header.fcc == 0x68727473);

						// check stream type, can only handle 'vids'
						if (stream_header.fccType != 0x73646976) {
							printf("AVI => first stream must be video '%s'\n", filename);
							fclose(pfile);
							return -1;
						}
						SDL_assert(stream_header.fccType == 0x73646976);

						// only handle 'MRLE' encoding
						if (stream_header.fccHandler != 0x454c524d) {
							printf("AVI is not MRLE encoded '%s'\n", filename);
							fclose(pfile);
							return -1;
						}

						// no pal changes for you cowboy
						if (stream_header.dwFlags & AVISF_VIDEO_PALCHANGES) {
							printf("AVI cannot have palette changes '%s'\n", filename);
							fclose(pfile);
							return -1;
						}

						// next stream sub-chunk -------------------------------

						// check for 'strf'
						fread(&tmp, 1, 4, pfile);
						tmp = INTEL_INT(tmp);
						SDL_assert(tmp == 0x66727473);

						// size of 'strf'
						fread(&tmp, 1, 4, pfile);

						fread(&bitmap_header.bmiHeader.biSize, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biWidth, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biHeight, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biPlanes, 1, sizeof(short), pfile);
						fread(&bitmap_header.bmiHeader.biBitCount, 1, sizeof(short), pfile);
						fread(&bitmap_header.bmiHeader.biCompression, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biSizeImage, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biXPelsPerMeter, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biYPelsPerMeter, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biClrUsed, 1, sizeof(int), pfile);
						fread(&bitmap_header.bmiHeader.biClrImportant, 1, sizeof(int), pfile);

						bitmap_header.bmiHeader.biSize = INTEL_INT(bitmap_header.bmiHeader.biSize);
						bitmap_header.bmiHeader.biWidth = INTEL_INT(bitmap_header.bmiHeader.biWidth);
						bitmap_header.bmiHeader.biHeight = INTEL_INT(bitmap_header.bmiHeader.biHeight);
						bitmap_header.bmiHeader.biPlanes = INTEL_SHORT(bitmap_header.bmiHeader.biPlanes);
						bitmap_header.bmiHeader.biBitCount = INTEL_SHORT(bitmap_header.bmiHeader.biBitCount);
						bitmap_header.bmiHeader.biCompression = INTEL_INT(bitmap_header.bmiHeader.biCompression);
						bitmap_header.bmiHeader.biSizeImage = INTEL_INT(bitmap_header.bmiHeader.biSizeImage);
						bitmap_header.bmiHeader.biXPelsPerMeter = INTEL_INT(bitmap_header.bmiHeader.biXPelsPerMeter);
						bitmap_header.bmiHeader.biYPelsPerMeter = INTEL_INT(bitmap_header.bmiHeader.biYPelsPerMeter);
						bitmap_header.bmiHeader.biClrUsed = INTEL_INT(bitmap_header.bmiHeader.biClrUsed);
						bitmap_header.bmiHeader.biClrImportant = INTEL_INT(bitmap_header.bmiHeader.biClrImportant);

						// verify bpp is 8 and compression is RLE8
						if ( (bitmap_header.bmiHeader.biBitCount != 8) || (bitmap_header.bmiHeader.biCompression != BI_RLE8) ) {
							printf("AVI has wrong bpp or compression '%s'\n", filename);
							fclose(pfile);
							return -1;
						}

						// palette
						for (int i = 0; i < 256; i++) {
							fread(&bitmap_header.bmiColors[i].b, 1, 1, pfile);
							fread(&bitmap_header.bmiColors[i].g, 1, 1, pfile);
							fread(&bitmap_header.bmiColors[i].r, 1, 1, pfile);
							fread(&bitmap_header.bmiColors[i].p, 1, 1, pfile);
						}

						// reset next_chunk back to main 'LIST'
						next_chunk = offset_tmp;

						break;
					}

					// 'movi'
					case 0x69766f6d: {
						// need this for later
						movi_offset = ftell(pfile);

						// requiring an index, so don't mess with this anymore

						break;
					}

					default:
						printf("sub-something else: 0x%x\n", tag);
						break;
				}

				break;
			}

			// 'idx1'
			case 0x31786469: {
				SDL_assert(AVI_stream.frame_index == NULL);

				AVI_stream.frame_index = (FRAMEINDEX*) malloc(sizeof(FRAMEINDEX) * avi_header.dwTotalFrames);
				SDL_assert(AVI_stream.frame_index != NULL);

				memset(AVI_stream.frame_index, 0, sizeof(FRAMEINDEX) * avi_header.dwTotalFrames);

				unsigned int c_id, c_flags, c_offset, c_size, i = 0;

				while ( ftell(pfile) < next_chunk ) {
					fread(&c_id, 1, sizeof(int), pfile);
					fread(&c_flags, 1, sizeof(int), pfile);
					fread(&c_offset, 1, sizeof(int), pfile);
					fread(&c_size, 1, sizeof(int), pfile);

					c_id = INTEL_INT(c_id);
					c_flags = INTEL_INT(c_flags);
					c_offset = INTEL_INT(c_offset);
					c_size = INTEL_INT(c_size);

					// only interested in stream 0, compressed data: '00dc'
					if (c_id == 0x63643030) {
						SDL_assert(i < avi_header.dwTotalFrames);

						AVI_stream.frame_index[i].offset = c_offset;
						AVI_stream.frame_index[i].size = c_size;
						i++;
					}
				}

				// if we didn't get good data then clear it out
				if (i != avi_header.dwTotalFrames) {
					free(AVI_stream.frame_index);
					AVI_stream.frame_index = NULL;
				}

				break;
			}

			// drop everything else
			default:
				break;
		}

		fseek(pfile, next_chunk, SEEK_SET);
	}

	// make sure we have a frame index
	if (AVI_stream.frame_index == NULL) {
		printf("AVI => No valid frame index found '%s'\n", filename);
		fclose(pfile);
		return -1;
	}

	// fix up frame_index (assumes frame_index[0] is first frame in stream)
	long base_offset = 0;

	if (AVI_stream.frame_index[0].offset > (unsigned int)movi_offset) {
		base_offset = 0;
	} else if (AVI_stream.frame_index[0].offset == (unsigned int)movi_offset) {
		base_offset = 4;
	} else if (AVI_stream.frame_index[0].offset == 0) {
		base_offset = movi_offset + 4;
	} else if (AVI_stream.frame_index[0].offset == 4) {
		base_offset = movi_offset;
	} else {
		SDL_assert(0);
	}

	for (unsigned int i = 0; i < avi_header.dwTotalFrames; i++) {
		AVI_stream.frame_index[i].offset += base_offset;

		// maybe fix up size too
		if (avi_header.dwSuggestedBufferSize < AVI_stream.frame_index[i].size) {
			avi_header.dwSuggestedBufferSize = AVI_stream.frame_index[i].size;
		}
	}

	// now set up AVI_stream

	strcpy(AVI_stream.filename, filename);
	AVI_stream.pfile = pfile;

	AVI_stream.min_compressed_buffer_size = max(avi_header.dwSuggestedBufferSize, stream_header.dwSuggestedBufferSize);
	SDL_assert(AVI_stream.min_compressed_buffer_size > 0);

	AVI_stream.w = bitmap_header.bmiHeader.biWidth;
	AVI_stream.h = bitmap_header.bmiHeader.biHeight;
	AVI_stream.bpp = bitmap_header.bmiHeader.biBitCount;

	// store the number of frames in the AVI_info[] structure
	AVI_stream.num_frames = avi_header.dwTotalFrames;

	// Store the palette in the AVI stream structure
	for (int i = 0; i < 256; i++) {
		AVI_stream.palette[i*3]	  = bitmap_header.bmiColors[i].r;
		AVI_stream.palette[i*3+1] = bitmap_header.bmiColors[i].g;
		AVI_stream.palette[i*3+2] = bitmap_header.bmiColors[i].b;
	}

	// set the flag to used, so to make sure we only process one AVI stream at a time
	AVI_stream.flags |= AVI_STREAM_F_USED;	

	return 0;
}


// AVI_stream_close() should be called when you are finished reading all the frames of an AVI
//
void AVI_stream_close()
{	
//	SDL_assert( AVI_stream.flags & AVI_STREAM_F_USED);

	if (AVI_stream.pfile) {
		fclose(AVI_stream.pfile);
		AVI_stream.pfile = NULL;
	}

	if (AVI_stream.frame_index) {
		free(AVI_stream.frame_index);
		AVI_stream.frame_index = NULL;
	}

	AVI_stream.flags &= ~AVI_STREAM_F_USED;			// clear the used flag

	AVI_stream_inited = 0;
}




// AVI_stream_get_next_frame() will take the next RLE'd AVI frame and return the
// uncompressed data in the buffer pointer supplied as a parameter.  The caller is
// responsible for allocating the memory before-hand (the memory required is easily
// calculated by looking at the w and h members in AVI_stream).
// 
// returns:    0 ==> success
//            !0 ==> error
//
int AVI_stream_get_frame(ubyte* buffer, int frame_number)
{
	if ( frame_number > AVI_stream.num_frames ) {
		buffer = NULL;
		return -1;
	}

	SDL_assert( (frame_number - 1) >= 0 );

	ubyte* compressed_frame = (ubyte*)malloc(AVI_stream.min_compressed_buffer_size);
	SDL_assert( compressed_frame != NULL );
	memset(compressed_frame, 0, AVI_stream.min_compressed_buffer_size);

	fseek(AVI_stream.pfile, AVI_stream.frame_index[frame_number-1].offset, SEEK_SET);
	fread(compressed_frame, 1, AVI_stream.frame_index[frame_number-1].size, AVI_stream.pfile);

	AVI_decompress_RLE8(compressed_frame, buffer, AVI_stream.w, AVI_stream.h);

	free( compressed_frame );

	return 0;
}



// -------------------------------------------------------------------------------------------------
// AVI_decompress_RLE8() will decompress the data pointed to by src, and store in dest.
//
//	NOTE:  1. memory for dest must be already allocated before calling function
//

void AVI_decompress_RLE8(ubyte* src, ubyte* dest, int w, int h)
{
	int src_index = 0;
	int dest_index = 0;
	int i;

	SDL_assert( src != NULL);
	SDL_assert( dest != NULL);
	SDL_assert( w > 0 );
	SDL_assert( h > 0 );

	ubyte count = 0;
	ubyte run = 0;
	ubyte control_code = 0;
	ubyte x_off = 0;
	ubyte y_off = 0;

	int size_src = w * h + 1;

	int scan_line = h-1;
	int height_offset = scan_line * w;

	while ( src_index < size_src ) {
		
		count = src[src_index];
		
		if ( count == 0 ) {	// control code follows
			src_index++;
			control_code = src[src_index];
			if ( control_code == 1 ) {
				src_index++;
//				nprintf(("AVI","AVI ==> Reached end of compressed image\n"));
				break;
			}
			else if ( control_code == 0 ) {
				src_index++;
				scan_line--;
				height_offset = scan_line * w;	// only need to calc once per scanline
				dest_index = 0;
				if (height_offset < 0) {
					break;
				}
				//nprintf(("AVI","AVI ==> Reached end of line in compressed image\n"));
			}
			else if ( control_code == 2 ) {
				// delta - horizontal and veritcal offsets
				src_index++;
				x_off = src[src_index];

				if (x_off) {
					dest_index += x_off;

					if (dest_index >= w) {
						break;
					}
				}

				src_index++;
				y_off = src[src_index];

				if (y_off) {
					scan_line -= y_off;
					height_offset = scan_line * w;

					if (height_offset < 0) {
						break;
					}
				}

				src_index++;
			}
			else {
				// in absolute mode
				src_index++;
				//SDL_assert( (height_offset + dest_index) < (AVI_stream.w * AVI_stream.h) );
				for ( i = 0; i < control_code; i++ ) {
					if (dest_index >= w) {
						break;
					}
					dest[height_offset + dest_index] = src[src_index];
					dest_index++;
					src_index++;
				}
				// run must end on a word boundry
				if ( control_code & 1 )
					src_index++;
			}
		}
		else {
			src_index++;
			run = src[src_index];
			src_index++;
			// nprintf(("AVI","AVI ==> Got %d pixel run of %d\n", src[src_index], count));
			//SDL_assert( (height_offset + dest_index + count) <= (w * h) );
			//memset(&dest[height_offset+dest_index], run, count);
			//dest_index += count;
			for ( i = 0; i < count; i++ ) {
				if (dest_index >= w) {
					break;
				}
				dest[height_offset + dest_index] = run;
				dest_index++;
			}
		}

	}	// end while

}

int save_anim_header()
{
	int i, new_format_id = 0;

	SDL_assert(anim_fp);
	fclose(anim_fp);
	anim_fp = fopen(anim_save_filename, "r+b");

	if (!fwrite(&new_format_id, 2, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.version, 2, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.fps, 2, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.xparent_r, 1, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.xparent_g, 1, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.xparent_b, 1, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.width, 2, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.height, 2, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.total_frames, 2, 1, anim_fp))
		return -1;
	if (!fwrite(&Anim.packer_code, 1, 1, anim_fp))
		return -1;
	if (fwrite(&Anim.palette, 3, 256, anim_fp) != 256)
		return -1;
	if (!fwrite(&total_key_frames, 2, 1, anim_fp))
		return -1;

	for (i=0; i<Anim.num_keys; i++) {
		if (!fwrite(&Anim.keys[i].frame_num, 2, 1, anim_fp))
			return -1;

		if (!fwrite(&Anim.keys[i].offset, 4, 1, anim_fp))
			return -1;
	}

	if (!fwrite(&anim_offset, 4, 1, anim_fp))
		return -1;

	return 0;
}

// This function allocates a linked list of key frame headers.
// It is responsible for determining which frames in an anim
// should be key frames.
int allocate_key_frames(int total_frames)
{
	int count = 0, frame = 1, rate = key_frame_rate, last_frame;

	if (!rate)
		rate = total_frames;

	while (frame <= total_frames) {
		count++;
		frame += rate;
	}

	if (force_key_frame >= 0)
		count++;

	if (count)
		Anim.keys = (key_frame *) malloc(count * sizeof(key_frame));

	count = 0;
	frame = last_frame = 1;
	while (frame <= total_frames) {
		if ((force_key_frame > last_frame) && (force_key_frame < frame))
			Anim.keys[count++].frame_num = force_key_frame;

		Anim.keys[count++].frame_num = frame;
		frame += rate;
	}

	if (force_key_frame > last_frame)
		Anim.keys[count++].frame_num = force_key_frame;

	Anim.num_keys = count;
	return count;  // number of key frames
}

int anim_save_init(char *file, int width, int height, int frames)
{
	SDL_assert(file);
	anim_save_filename = file;
	anim_fp = fopen(file, "wb");
	if (!anim_fp)
		return -1;

	Anim.version = ANIM_VERSION;
	Anim.fps = Default_fps;
	Anim.width = width;
	Anim.height = height;
	Anim.packer_code = PACKER_CODE;
	Anim.xparent_r = Xparent_color.r;
	Anim.xparent_g = Xparent_color.g;
	Anim.xparent_b = Xparent_color.b;
	Anim.total_frames = frames;
	anim_offset = 0;
	cur_frame_num = 0;
	total_key_frames = allocate_key_frames(frames);
	fseek(anim_fp, ANIM_HEADER_SIZE + total_key_frames * 6, SEEK_SET);

	switch ( Compression_type ) {
		case CUSTOM_DELTA_RLE:
			Key_frame_compression = PACKING_METHOD_RLE_KEY;
			Regular_frame_compression = PACKING_METHOD_RLE;
			break;

		case STD_DELTA_RLE:
			Key_frame_compression = PACKING_METHOD_STD_RLE_KEY;
			Regular_frame_compression = PACKING_METHOD_STD_RLE;
			break;

		default:
			Int3();
			return -1;
			break;
	} // end switch

	return 0;
}

int anim_save_frame()
{
	ubyte *temp;
	int i, size;
	key_frame *keyp = NULL;

	SDL_assert(anim_fp);
	cur_frame_num++;
	SDL_assert(cur_frame_num <= Anim.total_frames);

	for (i=0; i<Anim.num_keys; i++)
		if (Anim.keys[i].frame_num == cur_frame_num) {
			keyp = &Anim.keys[i];
			break;
		}

	if (keyp) {
		fprintf(stdout, "*");
		fflush(stdout);
		keyp->offset = anim_offset;
		size = pack_key_frame(cur_frame, anim_buffer, Anim.width * Anim.height, ANIM_BUFFER_MAX, Key_frame_compression);

	} else {
		fprintf(stdout, ".");
		fflush(stdout);
		size = pack_frame(cur_frame, last_frame, anim_buffer, Anim.width * Anim.height, ANIM_BUFFER_MAX, Regular_frame_compression);
	}

	if (size < 0)
		return -1;

	if ((int) fwrite(anim_buffer, 1, size, anim_fp) != size)
		return -1;

	anim_offset += size;
	temp = cur_frame;
	cur_frame = last_frame;
	last_frame = temp;
	return 0;
}


// converts an avi file to an anim file
//
// returns:   0 ==> success
//	          !0 ==> failure
//
int convert_avi_to_anim(char* filename)
{
	char ani_filename[255];
	int ret = 1;
	int rc, i, xparent_pal_index;
	int avi_stream_opened = 0;

	rc = AVI_stream_open(filename);
	if ( rc != 0 ) {
		// could not open the AVI stream
		goto Finish;
	}
	avi_stream_opened = 1;
	
	SDL_assert(AVI_stream.bpp == 8);
	cur_frame = (ubyte*) malloc(AVI_stream.w * AVI_stream.h);
	last_frame = (ubyte*) malloc(AVI_stream.w * AVI_stream.h);
	SDL_assert(cur_frame && last_frame);

	strcpy(ani_filename, AVI_stream.filename);
	strcpy(ani_filename + strlen(ani_filename) - 3, "ani");

	memset(&Anim, 0, sizeof(anim));

	memcpy(Anim.palette, AVI_stream.palette, 768);

	if (Use_custom_xparent_color) {
		// Need to look at pixel in top-left 
		rc = AVI_stream_get_frame(cur_frame, 1);
		if ( rc != 0 )
			goto Finish;

		xparent_pal_index = cur_frame[0];
		Xparent_color.r = Anim.palette[xparent_pal_index * 3];
		Xparent_color.g = Anim.palette[xparent_pal_index * 3 + 1];
		Xparent_color.b = Anim.palette[xparent_pal_index * 3 + 2];

	} else {
		Xparent_color.r = 0;
		Xparent_color.g = 255;
		Xparent_color.b = 0;
	}
	
	rc = anim_save_init(ani_filename, AVI_stream.w, AVI_stream.h, AVI_stream.num_frames);
	if (rc == -1)
		goto Finish;

	for ( i=1; i <= AVI_stream.num_frames; i++ ) {
		// get uncompressed frame from the AVI
		rc = AVI_stream_get_frame(cur_frame, i);
		if ( rc != 0 )
			goto Finish;

		// pass to the anim compression
		rc = anim_save_frame();
		if ( rc != 0 )
			goto Finish;
	}

	rc = save_anim_header();
	if ( rc != 0 )
		goto Finish;

	ret = 0;

	Finish:
	// done with the AVI.. close the stream
	if ( avi_stream_opened )
		AVI_stream_close();

	if ( anim_fp )
		fclose(anim_fp);

	if (Anim.keys)
		free(Anim.keys);

	free(cur_frame);
	free(last_frame);
	fprintf(stdout,"\n");
	fflush(stdout);
	return ret;
}

int convert_frames_to_anim(char *filename)
{
	int first_frame, frame, pos, width, height, xparent_pal_index, r = -1;
	char ani_filename[255], name[255], temp[8];	
	int rc;
	FILE *fp;

	SDL_assert(strlen(filename) < 254);
	strcpy(name, filename);
	strcpy(ani_filename, filename);
	strcpy(ani_filename + strlen(ani_filename) - 8, ".ani");
	pos = strlen(name) - 8;
	frame = first_frame = atoi(&name[pos]);
	force_key_frame -= frame;

	memset(&Anim, 0, sizeof(anim));

	// first file
	fp = fopen(name, "rb");
	if(fp != NULL){
		do {
			fclose(fp);
			frame++;
			sprintf(temp, "%04d", frame);
			strncpy(&name[pos], temp, 4);	

			// next file
			fp = fopen(name, "rb");
		} while(fp != NULL);	
	}

	rc = pcx_read_header(filename, &width, &height, NULL);
	if (rc != PCX_ERROR_NONE) {
		fprintf(stdout, "An error reading the PCX file %s.  It may not exist.\n", filename);
		return -1;
	}

	cur_frame = (ubyte *) malloc(width * height);
	last_frame = (ubyte *) malloc(width * height);

	rc = pcx_read_bitmap_8bpp(filename, cur_frame, Anim.palette);
	if (rc != PCX_ERROR_NONE) {
		fprintf(stdout, "An error reading the PCX file %s.  It may not exist.\n", filename);
		return -1;
	}

	if (Use_custom_xparent_color) {
		// Need to look at pixel in top-left 
		xparent_pal_index = ((ubyte *) cur_frame)[0];
		Xparent_color.r = Anim.palette[xparent_pal_index * 3];
		Xparent_color.g = Anim.palette[xparent_pal_index * 3 + 1];
		Xparent_color.b = Anim.palette[xparent_pal_index * 3 + 2];

	} else {
		Xparent_color.r = 0;
		Xparent_color.g = 255;
		Xparent_color.b = 0;
	}

	if (anim_save_init(ani_filename, width, height, frame - first_frame))
		goto done;

	while (first_frame < frame) {
		sprintf(temp, "%04d", first_frame);
		strncpy(&name[pos], temp, 4);
		rc = pcx_read_bitmap_8bpp(name, cur_frame, Anim.palette);
		if (rc != PCX_ERROR_NONE)
			goto done;

		if (anim_save_frame())
			goto done;

		first_frame++;
	}

	if (save_anim_header())
		goto done;

	r = 0;

done:
	if (Anim.keys)
		free(Anim.keys);

	fclose(anim_fp);
	free(cur_frame);
	free(last_frame);
	fprintf(stdout, "\n");
	fflush(stdout);
	return r;
}

