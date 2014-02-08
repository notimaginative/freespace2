/*
 * $Logfile: $
 * $Revision$
 * $Date$
 * $Author$
 *
 * OpenAL based audio streaming
 *
 * $Log$
 * Revision 1.4  2005/10/01 21:53:06  taylor
 * include file cleanup
 * byte-swap streaming PCM to avoid the endless, loud, static
 *
 * Revision 1.3  2005/08/13 16:59:23  taylor
 * type check
 *
 * Revision 1.2  2005/08/12 20:21:06  taylor
 * woorps!
 *
 * Revision 1.1  2005/08/12 08:44:39  taylor
 * import of FS2_Open audio code which is now *nix only, does not include windows or ogg support that FS2_Open has
 *
 * Revision 1.12  2005/06/24 19:36:49  taylor
 * we only want to have m_data_offset be 0 for oggs since the seeking callback will account for the true offset
 * only extern the one int we need for the -nosound speech fix rather than including the entire header
 *
 * Revision 1.11  2005/06/19 02:45:55  taylor
 * OGG streaming fixes to get data reading right and avoid skipping
 * properly handle seeking in OGG streams
 * compiler warning fix in OpenAL builds
 *
 * Revision 1.10  2005/06/01 09:41:14  taylor
 * bit of cleanup for audiostr-openal and fix a Windows-only enum error
 * bunch of OGG related fixes for Linux and Windows (DirectSound and OpenAL), fixes audio related TBP 3.2 crashes
 * gracefully handle OGG logical bitstream changes, shouldn't even load if there is more than 1
 *
 * Revision 1.9  2005/05/28 19:43:28  taylor
 * debug message fixing
 * a little bit of code clarity
 *
 * Revision 1.8  2005/05/24 03:11:38  taylor
 * an extra bounds check in sound.cpp
 * fix audiostr error when filename is !NULL but 0 in len might hit on SDL debug code
 *
 * Revision 1.7  2005/05/15 06:47:57  taylor
 * don't let the ogg callbacks close the file handle on us, let us do it ourselves to keep things straight
 *
 * Revision 1.6  2005/05/13 23:09:28  taylor
 * Ooops!  Added the wrong version of the streaming patch from Jens
 *
 * Revision 1.5  2005/05/12 17:47:57  taylor
 * use vm_malloc(), vm_free(), vm_realloc(), vm_strdup() rather than system named macros
 *   fixes various problems and is past time to make the switch
 * fix a few streaming errors in OpenAL code (Jens Granseuer)
 * temporary change to help deal with missing music in OpenAL Windows builds
 * don't assert when si->data is NULL unless we really need to check (OpenAL only)
 *
 * Revision 1.4  2005/04/05 11:48:22  taylor
 * remove acm-unix.cpp, replaced by acm-openal.cpp since it's properly cross-platform now
 * better error handling for OpenAL functions
 * Windows can now build properly with OpenAL
 * extra check to make sure we don't try and use too many hardware bases sources
 * fix memory error from OpenAL extension list in certain instances
 *
 * Revision 1.3  2005/04/01 07:33:08  taylor
 * fix hanging on exit with OpenAL
 * some better error handling on OpenAL init and make it more Windows friendly too
 * basic 3d sound stuff for OpenAL, not working right yet
 *
 * Revision 1.2  2005/03/27 08:51:24  taylor
 * this is what coding on an empty stomach will get you
 *
 * Revision 1.1  2005/03/27 05:48:58  taylor
 * initial import of OpenAL streaming (many thanks to Pierre Willenbrock for the missing parts)
 *
 *
 * $NoKeywords: $
 */


#include <vector>

#include "pstypes.h"
#include "audiostr.h"
#include "oal.h"
#include "acm.h"
#include "cfile.h"
#include "sound.h"
#include "timer.h"


#define MAX_STREAM_BUFFERS 4

// Constants
#define BIGBUF_SIZE					180000			// This can be reduced to 88200 once we don't use any stereo
//#define BIGBUF_SIZE					88300			// This can be reduced to 88200 once we don't use any stereo
static ubyte *Wavedata_load_buffer = NULL;		// buffer used for cueing audiostreams
static ubyte *Wavedata_service_buffer = NULL;	// buffer used for servicing audiostreams

CRITICAL_SECTION Global_service_lock;

typedef bool (*TIMERCALLBACK)(ptr_u);

#define COMPRESSED_BUFFER_SIZE	88300
static ubyte *Compressed_buffer = NULL;				// Used to load in compressed data during a cueing interval
static ubyte *Compressed_service_buffer = NULL;	// Used to read in compressed data during a service interval

#define AS_HIGHEST_MAX	999999999	// max uncompressed filesize supported is 999 meg


static int Audiostream_inited = 0;


class Timer
{
public:
    void constructor();
    void destructor();

    bool Create(uint nPeriod, ptr_u dwUser, TIMERCALLBACK pfnCallback);

protected:
	static Uint32 TimeProc(Uint32 interval, void *dwUser);

	TIMERCALLBACK m_pfnCallback;
    ptr_u m_dwUser;
    uint m_nPeriod;
    SDL_TimerID m_nIDTimer;
};

class WaveFile
{
public:
	void Init();
	void Close();
	bool Open(const char *pszFilename);
	bool Cue();
	int	Read(ubyte *pbDest, uint cbSize, int service = 1);
	ubyte GetSilenceData();

	uint GetNumBytesRemaining()
	{
		return (m_nDataSize - m_nBytesPlayed);
	}

	uint GetUncompressedAvgDataRate()
	{
		return m_nUncompressedAvgDataRate;
	}

	uint GetDataSize()
	{
		return m_nDataSize;
	}

	uint GetNumBytesPlayed()
	{
		return m_nBytesPlayed;
	}

	ALenum GetOALFormat()
	{
		return m_oal_format;
	}

	WAVE_chunk m_wfmt;					// format of wave file
	WAVE_chunk *m_pwfmt_original;	// foramt of wave file from actual wave source
	uint m_total_uncompressed_bytes_read;
	uint m_max_uncompressed_bytes_to_read;
	uint m_bits_per_sample_uncompressed;

protected:
	uint m_data_offset;						// number of bytes to actual wave data
	int  m_data_bytes_left;
	CFILE *cfp;

	ALenum m_oal_format;
	uint m_wave_format;						// format of wave source (ie WAVE_FORMAT_PCM, WAVE_FORMAT_ADPCM)
	uint m_nBlockAlign;						// wave data block alignment spec
	uint m_nUncompressedAvgDataRate;		// average wave data rate
	uint m_nDataSize;							// size of data chunk
	uint m_nBytesPlayed;						// offset into data chunk
	bool m_abort_next_read;

	void *m_hStream;
	int m_hStream_open;
	WAVE_chunk m_wfxDest;
};

class AudioStream
{
public:
	AudioStream();
	~AudioStream();

	bool Create(const char *pszFilename);
	bool Destroy();
	void Play(float volume, int looping);
	void Stop(int paused = 0);
	void Stop_and_Rewind();
	void Fade_and_Destroy();
	void Fade_and_Stop();
	void Set_Volume(float vol);
	float Get_Volume();
	void Set_Byte_Cutoff(uint num_bytes_cutoff);
	uint Get_Bytes_Committed();

	int Is_Playing()
	{
		return m_fPlaying;
	}

	int Is_Paused()
	{
		return m_bIsPaused;
	}

	int Is_Past_Limit()
	{
		return m_bPastLimit;
	}

	void Set_Default_Volume(float _volume)
	{
		m_lDefaultVolume = _volume;
	}

	float Get_Default_Volume()
	{
		return m_lDefaultVolume;
	}

	int	Is_looping()
	{
		return m_bLooping;
	}

	int	type;
	ushort m_bits_per_sample_uncompressed;

protected:
	void Cue();
	bool WriteWaveData(uint cbSize, uint* num_bytes_written,int service=1);
	uint GetMaxWriteSize();
	bool ServiceBuffer();
	static bool TimerCallback(ptr_u dwUser);

	ALuint m_source_id;   // name of openAL source
	ALuint m_buffer_ids[MAX_STREAM_BUFFERS]; //names of buffers
	int m_play_buffer_id;

	Timer m_timer;              // ptr to Timer object
	WaveFile *m_pwavefile;        // ptr to WaveFile object
	bool m_fCued;                  // semaphore (stream cued)
	bool m_fPlaying;               // semaphore (stream playing)
	long m_lInService;             // reentrancy semaphore
	uint m_cbBufOffset;            // last write position
	uint m_nBufLength;             // length of sound buffer in msec
	uint m_cbBufSize;              // size of sound buffer in bytes
	uint m_nBufService;            // service interval in msec
	uint m_nTimeStarted;           // time (in system time) playback started

	bool	m_bLooping;						// whether or not to loop playback
	bool	m_bFade;							// fade out music
	bool	m_bDestroy_when_faded;
	float	m_lVolume;						// volume of stream ( 0 -> -10 000 )
	float	m_lCutoffVolume;
	bool	m_bIsPaused;					// stream is stopped, but not rewinded
	ushort	m_silence_written;			// number of bytes of silence written to buffer
	ushort	m_bReadingDone;				// no more bytes to be read from disk, still have remaining buffer to play
	uint	m_fade_timer_id;				// timestamp so we know when to start fade
	uint	m_finished_id;					// timestamp so we know when we've played #bytes required
	bool	m_bPastLimit;					// flag to show we've played past the number of bytes requred
	float	m_lDefaultVolume;
};


// Timer class implementation
//
////////////////////////////////////////////////////////////

void Timer::constructor()
{
	m_nIDTimer = 0;
}

void Timer::destructor()
{
	if (m_nIDTimer) {
		SDL_RemoveTimer(m_nIDTimer);
		m_nIDTimer = 0;
	}
}

bool Timer::Create(uint nPeriod, ptr_u dwUser, TIMERCALLBACK pfnCallback)
{
	SDL_assert( pfnCallback != NULL );
	SDL_assert( nPeriod > 10 );

	m_nPeriod = nPeriod;
	m_dwUser = dwUser;
	m_pfnCallback = pfnCallback;

	m_nIDTimer = SDL_AddTimer(m_nPeriod, TimeProc, (void*)this);

	if ( !m_nIDTimer ) {
		nprintf(("SOUND", "SOUND ==> Error, unable to create timer\n"));
		return false;
	}

	return true;
}

// Calls procedure specified when Timer object was created. The 
// dwUser parameter contains "this" pointer for associated Timer object.
// 
Uint32 Timer::TimeProc(Uint32 interval, void *dwUser)
{
    // dwUser contains ptr to Timer object
	Timer *ptimer = (Timer *)dwUser;

    // Call user-specified callback and pass back user specified data
    (ptimer->m_pfnCallback)(ptimer->m_dwUser);

    if (ptimer->m_nPeriod) {
		return interval;
    } else {
		SDL_RemoveTimer(ptimer->m_nIDTimer);
		ptimer->m_nIDTimer = 0;

		return 0;
    }
}


// WaveFile class implementation
//
////////////////////////////////////////////////////////////

void WaveFile::Init()
{
	// Init data members
	m_data_offset = 0;
	cfp = NULL;
	m_pwfmt_original = NULL;
	m_nBlockAlign= 0;
	m_nUncompressedAvgDataRate = 0;
	m_nDataSize = 0;
	m_nBytesPlayed = 0;
	m_total_uncompressed_bytes_read = 0;
	m_max_uncompressed_bytes_to_read = AS_HIGHEST_MAX;

	m_hStream_open = 0;
	m_abort_next_read = false;
}

void WaveFile::Close()
{
	// Free memory
	if (m_pwfmt_original) {
		free(m_pwfmt_original);
		m_pwfmt_original = NULL;
	}

	if (m_hStream_open) {
		ACM_stream_close((void*)m_hStream);
		m_hStream_open = 0;
	}

	// Close file
	if (cfp) {
		cfclose(cfp);
		cfp = NULL;
	}
}

bool WaveFile::Open(const char *pszFilename)
{
	int done = false;
	bool fRtn = true;    // assume success
	int id = 0;
	uint tag = 0, size = 0, next_chunk;

	m_total_uncompressed_bytes_read = 0;
	m_max_uncompressed_bytes_to_read = AS_HIGHEST_MAX;

	m_pwfmt_original = (WAVE_chunk*) malloc (sizeof(WAVE_chunk));

	if (m_pwfmt_original == NULL) {
		goto OPEN_ERROR;
	}

	cfp = cfopen(pszFilename, "rb");

	if (cfp == NULL) {
		goto OPEN_ERROR;
	}

	// check for valid file type
	id = cfread_int(cfp);

	// 'RIFF'
	if (id != 0x46464952) {
		nprintf(("Error", "Not a WAVE file '%s'\n", pszFilename));
		goto OPEN_ERROR;
	}

	// skip RIFF size
	cfread_int(cfp);

	// check for valid RIFF type
	id = cfread_int(cfp);

	// 'WAVE'
	if (id != 0x45564157) {
		nprintf(("Error", "Not a WAVE file '%s'\n", pszFilename));
		goto OPEN_ERROR;
	}

	while ( !done )	{
		tag = cfread_uint(cfp);
		size = cfread_uint(cfp);

		next_chunk = cftell(cfp) + size;

		switch (tag) {
			// 'fmt '
			case 0x20746d66: {
				m_pwfmt_original->code = cfread_short(cfp);
				m_pwfmt_original->num_channels = cfread_ushort(cfp);
				m_pwfmt_original->sample_rate = cfread_uint(cfp);
				m_pwfmt_original->bytes_per_second = cfread_uint(cfp);
				m_pwfmt_original->block_align = cfread_ushort(cfp);
				m_pwfmt_original->bits_per_sample = cfread_ushort(cfp);

				if (m_pwfmt_original->code != 1) {
					m_pwfmt_original->extra_size = cfread_ushort(cfp);
				}

				if (m_pwfmt_original->extra_size) {
					m_pwfmt_original->extra_data = (ubyte*) malloc (m_pwfmt_original->extra_size);
					SDL_assert( m_pwfmt_original->extra_data != NULL );

					if (m_pwfmt_original->extra_data == NULL) {
						goto OPEN_ERROR;
					}

					cfread(m_pwfmt_original->extra_data, m_pwfmt_original->extra_size, 1, cfp);
				}

				break;
			}

			// 'data'
			case 0x61746164: {
				m_nDataSize = size;	// size of data, compressed size if ADPCM
				m_data_bytes_left = size;
				m_data_offset = cftell(cfp);

				done = true;

				break;
			}

			// drop everything else
			default:
				break;
		}

		cfseek(cfp, next_chunk, CF_SEEK_SET);
	}

	// we force PCM format, so keep track of original format for later
	switch (m_pwfmt_original->code) {
		case WAVE_FORMAT_PCM:
			m_wave_format = WAVE_FORMAT_PCM;
			m_wfmt.bits_per_sample = m_pwfmt_original->bits_per_sample;
			break;

		case WAVE_FORMAT_ADPCM:
			m_wave_format = WAVE_FORMAT_ADPCM;
			m_wfmt.bits_per_sample = 16;
			m_bits_per_sample_uncompressed = 16;
			break;

		default:
			nprintf(("SOUND", "SOUND => Not supporting %d format for playing wave files\n", m_pwfmt_original->code));
			//Int3();
			goto OPEN_ERROR;
			break;

	}
            
	m_wfmt.code = WAVE_FORMAT_PCM;
	m_wfmt.num_channels = m_pwfmt_original->num_channels;
	m_wfmt.sample_rate = m_pwfmt_original->sample_rate;
	m_wfmt.extra_size = 0;
	m_wfmt.block_align = (ushort)(( m_wfmt.num_channels * m_wfmt.bits_per_sample ) / 8);
	m_wfmt.bytes_per_second = m_wfmt.block_align * m_wfmt.sample_rate;

	// set OpenAL format
	m_oal_format = AL_FORMAT_MONO8;

	if (m_wfmt.num_channels == 1) {
		if (m_wfmt.bits_per_sample == 8) {
			m_oal_format = AL_FORMAT_MONO8;
		} else if (m_wfmt.bits_per_sample == 16) {
			m_oal_format = AL_FORMAT_MONO16;
		}
	} else if (m_wfmt.num_channels == 2) {
		if (m_wfmt.bits_per_sample == 8) {
			m_oal_format = AL_FORMAT_STEREO8;
		} else if (m_wfmt.bits_per_sample == 16) {
			m_oal_format = AL_FORMAT_STEREO16;
		}
	}

	// Init some member data from format chunk
	m_nBlockAlign = m_pwfmt_original->block_align;
	m_nUncompressedAvgDataRate = m_wfmt.bytes_per_second;

	// Successful open
	goto OPEN_DONE;

OPEN_ERROR:
	// Handle all errors here
	nprintf(("SOUND","SOUND ==> Could not open wave file %s for streaming\n", pszFilename));

	fRtn = false;

	if (m_pwfmt_original) {
		free(m_pwfmt_original);
		m_pwfmt_original = NULL;
	}

	if (cfp != NULL) {
		cfclose(cfp);
		cfp = NULL;
	}

OPEN_DONE:
	return (fRtn);
}

// Cue
//
// Set the file pointer to the start of wave data
//
bool WaveFile::Cue()
{
	bool fRtn = true;    // assume success
	int rval = -1;

	m_total_uncompressed_bytes_read = 0;
	m_max_uncompressed_bytes_to_read = AS_HIGHEST_MAX;

	rval = cfseek(cfp, m_data_offset, CF_SEEK_SET);

	if (rval) {
		fRtn = false;
	}

	m_data_bytes_left = m_nDataSize;
	m_abort_next_read = false;

	return fRtn;
}

// Read
//
// Returns number of bytes actually read.
// 
//	Returns -1 if there is nothing more to be read.  This function can return 0, since
// sometimes the amount of bytes requested is too small for the ACM decompression to 
// locate a suitable block
int WaveFile::Read(ubyte *pbDest, uint cbSize, int service)
{
	void *dest_buf = NULL, *uncompressed_wave_data;
	int rc, uncompressed_bytes_written;
	uint src_bytes_used, convert_len, num_bytes_desired=0, num_bytes_read;

//	nprintf(("Alan","Reqeusted: %d\n", cbSize));

	if ( service ) {
		uncompressed_wave_data = Wavedata_service_buffer;
	} else {
		uncompressed_wave_data = Wavedata_load_buffer;
	}

	switch (m_wave_format) {
		case WAVE_FORMAT_PCM: {
			num_bytes_desired = cbSize;
			dest_buf = pbDest;

			break;
		}

		case WAVE_FORMAT_ADPCM: {
			if ( !m_hStream_open ) {
				if ( !ACM_stream_open(m_pwfmt_original, &m_wfxDest, (void**)&m_hStream, m_bits_per_sample_uncompressed) ) {
					m_hStream_open = 1;
				} else {
					Int3();
				}
			}

			num_bytes_desired = cbSize;
	
			if (service) {
				dest_buf = Compressed_service_buffer;
			} else {
				dest_buf = Compressed_buffer;
			}

			if (num_bytes_desired <= 0) {
				num_bytes_desired = 0;
//				nprintf(("Alan","No bytes required for ADPCM time interval\n"));
			} else {
				num_bytes_desired = ACM_query_source_size((void*)m_hStream, cbSize);
//				nprintf(("Alan","Num bytes desired: %d\n", num_bytes_desired));
			}

			break;
		}

		default:
			nprintf(("SOUND", "SOUND => Not supporting %d format for playing wave files\n"));
			Int3();
			break;

	} // end switch

	num_bytes_read = 0;
	convert_len = 0;
	src_bytes_used = 0;

	// read data from disk
	if (m_data_bytes_left <= 0) {
		num_bytes_read = 0;
		uncompressed_bytes_written = 0;

		return -1;
	}

	if ( (m_data_bytes_left > 0) && (num_bytes_desired > 0) ) {
		int actual_read = 0;

		if (num_bytes_desired <= (uint)m_data_bytes_left) {
			num_bytes_read = num_bytes_desired;
		} else {
			num_bytes_read = m_data_bytes_left;
		}

		actual_read = cfread(dest_buf, 1, num_bytes_read, cfp);

		if ( (actual_read <= 0) || (m_abort_next_read) ) {
			num_bytes_read = 0;
			uncompressed_bytes_written = 0;

			return -1;
		}

		if (num_bytes_desired >= (uint)m_data_bytes_left) {
			m_abort_next_read = 1;			
		}

		num_bytes_read = actual_read;
	}

	// convert data if necessary, to PCM
	if (m_wave_format == WAVE_FORMAT_ADPCM) {
		if ( num_bytes_read > 0 ) {
			rc = ACM_convert((void*)m_hStream, (ubyte*)dest_buf, num_bytes_read, (ubyte*)uncompressed_wave_data, BIGBUF_SIZE, &convert_len, &src_bytes_used);

			if (rc == -1) {
				goto READ_ERROR;
			}

			if (convert_len == 0) {
				Int3();
			}
		}

		SDL_assert( src_bytes_used <= num_bytes_read );

		if (src_bytes_used < num_bytes_read) {
			// seek back file pointer to reposition before unused source data
			cfseek(cfp, src_bytes_used - num_bytes_read, CF_SEEK_CUR);
		}

		// Adjust number of bytes left
		m_data_bytes_left -= src_bytes_used;
		m_nBytesPlayed += src_bytes_used;
		uncompressed_bytes_written = convert_len;

		// Successful read, keep running total of number of data bytes read
		goto READ_DONE;
	} else {
		// Successful read, keep running total of number of data bytes read
		// Adjust number of bytes left
		m_data_bytes_left -= num_bytes_read;
		m_nBytesPlayed += num_bytes_read;
		uncompressed_bytes_written = num_bytes_read;

#if BYTE_ORDER == BIG_ENDIAN
		if (m_wave_format == WAVE_FORMAT_PCM) {
			// swap 16-bit sound data
			if (m_wfmt.bits_per_sample == 16) {
				ushort *swap_tmp;
				
				for (int i = 0; i < uncompressed_bytes_written; i = (i+2)) {
					swap_tmp = (ushort*)((ubyte*)dest_buf + i);
					*swap_tmp = INTEL_SHORT(*swap_tmp);
				}
			}
		}
#endif

		goto READ_DONE;
	}
    
READ_ERROR:
	num_bytes_read = 0;
	uncompressed_bytes_written = 0;

READ_DONE:
	m_total_uncompressed_bytes_read += uncompressed_bytes_written;
//	nprintf(("Alan","Read: %d\n", uncompressed_bytes_written));

	return uncompressed_bytes_written;
}

// GetSilenceData
//
// Returns 8 bits of data representing silence for the Wave file format.
//
// Since we are dealing only with PCM format, we can fudge a bit and take
// advantage of the fact that for all PCM formats, silence can be represented
// by a single byte, repeated to make up the proper word size. The actual size
// of a word of wave data depends on the format:
//
// PCM Format       Word Size       Silence Data
// 8-bit mono       1 byte          0x80
// 8-bit stereo     2 bytes         0x8080
// 16-bit mono      2 bytes         0x0000
// 16-bit stereo    4 bytes         0x00000000
//
ubyte WaveFile::GetSilenceData()
{
	ubyte bSilenceData = 0;

	// Silence data depends on format of Wave file
	if (m_pwfmt_original) {
		if (m_wfmt.bits_per_sample == 8) {
			// For 8-bit formats (unsigned, 0 to 255)
			// Packed DWORD = 0x80808080;
			bSilenceData = 0x80;
		} else if (m_wfmt.bits_per_sample == 16) {
			// For 16-bit formats (signed, -32768 to 32767)
			// Packed DWORD = 0x00000000;
			bSilenceData = 0x00;
		} else {
			Int3();
		}
	} else {
		Int3();
	}

	return bSilenceData;
}

//
// AudioStream class implementation
//
////////////////////////////////////////////////////////////

// The following constants are the defaults for our streaming buffer operation.
static const ushort DefBufferLength          = 2000; // default buffer length in msec
static const ushort DefBufferServiceInterval = 250;  // default buffer service interval in msec

// Constructor
AudioStream::AudioStream()
{
	type = ASF_NONE;

	m_bLooping = false;
	m_bFade = false;
	m_fade_timer_id = 0;
	m_finished_id = 0;
	m_bPastLimit = false;

	m_bDestroy_when_faded = false;
	m_lDefaultVolume = 1.0f;
	m_lVolume = 1.0f;
	m_lCutoffVolume = 0.0f;
	m_bIsPaused = false;
	m_silence_written = 0;
	m_bReadingDone = false;

	m_pwavefile = NULL;
	m_bits_per_sample_uncompressed = 0;

	m_fPlaying = m_fCued = false;
	m_lInService = false;
	m_cbBufOffset = 0;
	m_nBufLength = DefBufferLength;
	m_cbBufSize = 0;
	m_nBufService = DefBufferServiceInterval;
	m_nTimeStarted = 0;

	memset(m_buffer_ids, 0, sizeof(m_buffer_ids));
	m_source_id = 0;
	m_play_buffer_id = 0;
}

// Destructor
AudioStream::~AudioStream()
{
}

// Create
bool AudioStream::Create(const char *pszFilename)
{
	SDL_assert( pszFilename != NULL );

	if (pszFilename == NULL) {
		return false;
	}

	// make 100% sure we got a good filename
	if ( !strlen(pszFilename) )
		return false;

	// Create a new WaveFile object
	m_pwavefile = (WaveFile *)malloc(sizeof(WaveFile));
	SDL_assert( m_pwavefile != NULL );

	if (m_pwavefile == NULL) {
		nprintf(("Sound", "SOUND => Failed to create WaveFile object %s\n\r", pszFilename));
		return false;
	}

	// Call constructor
	m_pwavefile->Init();

	m_pwavefile->m_bits_per_sample_uncompressed = m_bits_per_sample_uncompressed;

	// Open given file
	if ( m_pwavefile->Open(pszFilename) ) {
		// Calculate sound buffer size in bytes
		// Buffer size is average data rate times length of buffer
		// No need for buffer to be larger than wave data though
		m_cbBufSize = (m_nBufLength/1000) * (m_pwavefile->m_wfmt.bits_per_sample/8) * m_pwavefile->m_wfmt.num_channels * m_pwavefile->m_wfmt.sample_rate;
		m_cbBufSize /= MAX_STREAM_BUFFERS;
		// if the requested buffer size is too big then cap it
		m_cbBufSize = (m_cbBufSize > BIGBUF_SIZE) ? BIGBUF_SIZE : m_cbBufSize;

//		nprintf(("SOUND", "SOUND => Stream buffer created using %d bytes\n", m_cbBufSize));

		// Create sound buffer
		alGenBuffers(MAX_STREAM_BUFFERS, m_buffer_ids);

		Snd_sram += m_cbBufSize * MAX_STREAM_BUFFERS;
	} else {
		// Error opening file
		nprintf(("SOUND", "SOUND => Failed to open wave file: %s\n\r", pszFilename));

		m_pwavefile->Close();

		free(m_pwavefile);
		m_pwavefile = NULL;

		return false;
	}

	return true;
}

// Destroy
bool AudioStream::Destroy()
{
	// Stop playback
	Stop();

	// Release sound buffer
	alDeleteBuffers(MAX_STREAM_BUFFERS, m_buffer_ids);

	Snd_sram -= m_cbBufSize;

	// Delete WaveFile object
	if (m_pwavefile) {
		m_pwavefile->Close();

		free(m_pwavefile);
		m_pwavefile = NULL;
	}

	type = ASF_NONE;

	return true;
}

// WriteWaveData
//
// Writes wave data to sound buffer. This is a helper method used by Create and
// ServiceBuffer; it's not exposed to users of the AudioStream class.
bool AudioStream::WriteWaveData(uint size, uint *num_bytes_written, int service)
{
	ubyte *uncompressed_wave_data;

	*num_bytes_written = 0;

	if ( (size == 0) || m_bReadingDone ) {
		return false;
	}

	if ( (m_buffer_ids[0] == 0) || !m_pwavefile ) {
		return false;
	}

	if ( service ) {
		SDL_LockMutex(Global_service_lock);
	}
		    
	if ( service ) {
		uncompressed_wave_data = Wavedata_service_buffer;
	} else {
		uncompressed_wave_data = Wavedata_load_buffer;
	}

	int num_bytes_read = 0;

	if ( !service ) {
		for (int ib = 0; ib < MAX_STREAM_BUFFERS; ib++) {
			num_bytes_read = m_pwavefile->Read(uncompressed_wave_data, m_cbBufSize, service);

			if (num_bytes_read < 0) {
				m_bReadingDone = 1;
			} else if (num_bytes_read > 0) {
				alBufferData(m_buffer_ids[ib], m_pwavefile->GetOALFormat(), uncompressed_wave_data, num_bytes_read, m_pwavefile->m_wfmt.sample_rate);
				alSourceQueueBuffers(m_source_id, 1, &m_buffer_ids[ib]);

				*num_bytes_written += num_bytes_read;
			}
		}
	} else {
		ALint buffers_processed = 0;
		ALuint buffer_id = 0;

		alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &buffers_processed);

		while (buffers_processed) {
			alSourceUnqueueBuffers(m_source_id, 1, &buffer_id);

			num_bytes_read = m_pwavefile->Read(uncompressed_wave_data, m_cbBufSize, service);

			if (num_bytes_read < 0) {
				m_bReadingDone = 1;
			} else if (num_bytes_read > 0) {
				alBufferData(buffer_id, m_pwavefile->GetOALFormat(), uncompressed_wave_data, num_bytes_read, m_pwavefile->m_wfmt.sample_rate);
				alSourceQueueBuffers(m_source_id, 1, &buffer_id);

				*num_bytes_written += num_bytes_read;
			}

			buffers_processed--;
		}
	}

	if ( service ) {
		SDL_UnlockMutex(Global_service_lock);
	}
    
	return true;
}

// GetMaxWriteSize
//
// Helper function to calculate max size of sound buffer write operation, i.e. how much
// free space there is in buffer.
uint AudioStream::GetMaxWriteSize()
{
	uint dwMaxSize = m_cbBufSize;
	ALint n = 0, q = 0;

	alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &n);

	alGetSourcei(m_source_id, AL_BUFFERS_QUEUED, &q);

	if ( !n && (q >= MAX_STREAM_BUFFERS) ) {
		//all buffers queued
		dwMaxSize = 0;
	}

	//	nprintf(("Alan","Max write size: %d\n", dwMaxSize));
	return dwMaxSize;
}

#define VOLUME_ATTENUATION_BEFORE_CUTOFF	0.03f		//  12db
#define VOLUME_ATTENUATION					0.65f

bool AudioStream::ServiceBuffer()
{
	float vol;
	int	fRtn = true;

	if (type == ASF_NONE) {
		return false;
	}

	if (m_bFade) {
		if (m_lCutoffVolume == 0.0f) {
			vol = Get_Volume();
//			nprintf(("Alan","Volume is: %d\n",vol));
			m_lCutoffVolume = vol * VOLUME_ATTENUATION_BEFORE_CUTOFF;
		}

		vol = Get_Volume() * VOLUME_ATTENUATION;
//		nprintf(("Alan","Volume is now: %d\n",vol));
		Set_Volume(vol);

//		nprintf(("Sound","SOUND => Volume for stream sound is %d\n",vol));
//		nprintf(("Alan","Cuttoff Volume is: %d\n",m_lCutoffVolume));
		if (vol < m_lCutoffVolume) {
			m_bFade = false;
			m_lCutoffVolume = 0.0f;

			if (m_bDestroy_when_faded) {
				Destroy();	

				return false;
			}
			else {
				Stop_and_Rewind();

				return true;
			}
		}
	}

	// All of sound not played yet, send more data to buffer
	uint dwFreeSpace = GetMaxWriteSize();

	// Determine free space in sound buffer
	if (dwFreeSpace) {
		// Some wave data remains, but not enough to fill free space
		// Send wave data to buffer, fill remainder of free space with silence
		uint num_bytes_written;

		if ( WriteWaveData(dwFreeSpace, &num_bytes_written) ) {
//			nprintf(("Alan","Num bytes written: %d\n", num_bytes_written));

			if (m_pwavefile->m_total_uncompressed_bytes_read >= m_pwavefile->m_max_uncompressed_bytes_to_read) {
				m_fade_timer_id = timer_get_milliseconds() + 1700;		// start fading 1.7 seconds from now
				m_finished_id = timer_get_milliseconds() + 2000;		// 2 seconds left to play out buffer
				m_pwavefile->m_max_uncompressed_bytes_to_read = AS_HIGHEST_MAX;
			}

			if ( (m_fade_timer_id > 0) && ((uint)timer_get_milliseconds() > m_fade_timer_id) ) {
				m_fade_timer_id = 0;
				Fade_and_Stop();
			}

			if ( (m_finished_id > 0) && ((uint)timer_get_milliseconds() > m_finished_id) ) {
				m_finished_id = 0;
				m_bPastLimit = true;
			}

			// see if we're done
			ALint state = 0;

			alGetSourcei(m_source_id, AL_SOURCE_STATE, &state);

			if ( m_bReadingDone && (state != AL_PLAYING) ) {
				if ( m_bDestroy_when_faded == true ) {
					Destroy();
					// Reset reentrancy semaphore

					return false;
				}

				// All of sound has played, stop playback or loop again
				if ( m_bLooping && !m_bFade) {
					Play(m_lVolume, m_bLooping);
				} else {
					Stop_and_Rewind();
				}
			}
		} else {
			// Error writing wave data
			fRtn = false;
			Int3(); 
		}
	}

	return (fRtn);
}

// Cue
void AudioStream::Cue()
{
	uint num_bytes_written;

	if ( !m_fCued ) {
		m_bFade = false;
		m_fade_timer_id = 0;
		m_finished_id = 0;
		m_bPastLimit = false;
		m_lVolume = 1.0f;
		m_lCutoffVolume = 0.0f;

		m_bDestroy_when_faded = false;

		// Reset buffer ptr
		m_cbBufOffset = 0;

		// Reset file ptr, etc
		m_pwavefile->Cue();

		// Unqueue all buffers
		ALint buffers_processed = 0;
		ALuint buffer_id = 0;

		alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &buffers_processed);

		while (buffers_processed) {
			alSourceUnqueueBuffers(m_source_id, 1, &buffer_id);

			buffers_processed--;
		}

		// Fill buffer with wave data
		WriteWaveData(m_cbBufSize, &num_bytes_written, 0);

		m_fCued = true;
	}
}

// Play
void AudioStream::Play(float volume, int looping)
{
	if (m_buffer_ids[0] != 0) {
		// If playing, stop
		if (m_fPlaying) {
			if ( m_bIsPaused == false)
				Stop_and_Rewind();
		}

		// Cue for playback if necessary
		if (!m_fCued) {
			Cue ();
		}

		if (looping == 1) {
			m_bLooping = true;
		} else {
			m_bLooping = false;
		}

		alSourcePlay(m_source_id);

		m_nTimeStarted = timer_get_milliseconds();
		Set_Volume(volume);

		// Kick off timer to service buffer
		m_timer.constructor();

		m_timer.Create(m_nBufService, (ptr_u)this, TimerCallback);

		// Playback begun, no longer cued
		m_fPlaying = true;
		m_bIsPaused = false;
	}
}

// Timer callback for Timer object created by ::Play method.
bool AudioStream::TimerCallback(ptr_u dwUser)
{
    // dwUser contains ptr to AudioStream object
    AudioStream * pas = (AudioStream *) dwUser;

    return (pas->ServiceBuffer ());
}

void AudioStream::Set_Byte_Cutoff(unsigned int byte_cutoff)
{
	if ( m_pwavefile == NULL )
		return;

	m_pwavefile->m_max_uncompressed_bytes_to_read = byte_cutoff;
}

uint AudioStream::Get_Bytes_Committed(void)
{
	if (m_pwavefile == NULL) {
		return 0;
	}

	return m_pwavefile->m_total_uncompressed_bytes_read;
}


// Fade_and_Destroy
void AudioStream::Fade_and_Destroy()
{
	m_bFade = true;
	m_bDestroy_when_faded = true;
}

// Fade_and_Destroy
void AudioStream::Fade_and_Stop()
{
	m_bFade = true;
	m_bDestroy_when_faded = false;
}


// Stop
void AudioStream::Stop(int paused)
{
	if (m_fPlaying) {
		if (paused) {
			alSourcePause(m_source_id);
		} else {
			alSourceStop(m_source_id);
		}

		m_fPlaying = false;
		m_bIsPaused = paused;

		// Delete Timer object
		m_timer.destructor();
	}
}

// Stop_and_Rewind
void AudioStream::Stop_and_Rewind()
{
	if (m_fPlaying) {
		// Stop playback
		alSourceStop(m_source_id);

		// Delete Timer object
		m_timer.destructor();

		m_fPlaying = false;
	}

	m_fCued = false;	// this will cause wave file to start from beginning
	m_bReadingDone = false;
}

// Set_Volume
void AudioStream::Set_Volume(float vol)
{
	alSourcef(m_source_id, AL_GAIN, vol);

	m_lVolume = vol;
}


// Set_Volume
float AudioStream::Get_Volume()
{
	return m_lVolume;
}


static std::vector<AudioStream> Audio_streams;


void audiostream_init()
{
	if (Audiostream_inited) {
		return;
	}

	// Allocate memory for the buffer which holds the uncompressed wave data that is streamed from the
	// disk during a load/cue
	if ( Wavedata_load_buffer == NULL ) {
		Wavedata_load_buffer = (ubyte*)malloc(BIGBUF_SIZE);
		Assert(Wavedata_load_buffer != NULL);
	}

	// Allocate memory for the buffer which holds the uncompressed wave data that is streamed from the
	// disk during a service interval
	if ( Wavedata_service_buffer == NULL ) {
		Wavedata_service_buffer = (ubyte*)malloc(BIGBUF_SIZE);
		Assert(Wavedata_service_buffer != NULL);
	}

	// Allocate memory for the buffer which holds the compressed wave data that is read from the hard disk
	if ( Compressed_buffer == NULL ) {
		Compressed_buffer = (ubyte*)malloc(COMPRESSED_BUFFER_SIZE);
		Assert(Compressed_buffer != NULL);
	}

	if ( Compressed_service_buffer == NULL ) {
		Compressed_service_buffer = (ubyte*)malloc(COMPRESSED_BUFFER_SIZE);
		Assert(Compressed_service_buffer != NULL);
	}

	Audio_streams.clear();

	SDL_InitSubSystem(SDL_INIT_TIMER);

	Global_service_lock = SDL_CreateMutex();

	Audiostream_inited = 1;
}

// Close down the audiostream system.  Must call audiostream_init() before any audiostream functions can
// be used.
void audiostream_close()
{
	int i;

	if ( !Audiostream_inited ) {
		return;
	}

	int size = (int)Audio_streams.size();

	for (i = 0; i < size; i++) {
		if (Audio_streams[i].type == ASF_NONE) {
			continue;
		}

		Audio_streams[i].Destroy();
	}

	Audio_streams.clear();

	// free global buffers
	if ( Wavedata_load_buffer ) {
		free(Wavedata_load_buffer);
		Wavedata_load_buffer = NULL;
	}

	if ( Wavedata_service_buffer ) {
		free(Wavedata_service_buffer);
		Wavedata_service_buffer = NULL;
	}

	if ( Compressed_buffer ) {
		free(Compressed_buffer);
		Compressed_buffer = NULL;
	}

	if ( Compressed_service_buffer ) {
		free(Compressed_service_buffer);
		Compressed_service_buffer = NULL;
	}

	SDL_DestroyMutex( Global_service_lock );

	Audiostream_inited = 0;

}

// Open a digital sound file for streaming
//
// input:	filename	=>	disk filename of sound file
//				type		=> what type of audio stream do we want to open:
//									ASF_SOUNDFX
//									ASF_EVENTMUSIC
//									ASF_VOICE
//	
// returns:	success => handle to identify streaming sound
//				failure => -1
int audiostream_open( const char *filename, int type )
{
	int i;

	if ( !Audiostream_inited ) {
		return -1;
	}

	int size = (int)Audio_streams.size();

	for (i = 0; i < size; i++) {
		if (Audio_streams[i].type == ASF_NONE) {
			Audio_streams[i].type = type;
			break;
		}
	}

	if (i == size) {
		AudioStream n_stream;
		Audio_streams.push_back(n_stream);
	}

	switch (type) {
		case ASF_VOICE:
		case ASF_SOUNDFX:
			Audio_streams[i].m_bits_per_sample_uncompressed = 8;
			break;

		case ASF_EVENTMUSIC:
			Audio_streams[i].m_bits_per_sample_uncompressed = 16;
			break;

		default:
			Int3();
			return -1;
	}

	if ( !Audio_streams[i].Create(filename) ) {
		Audio_streams[i].type = ASF_NONE;
		return -1;
	}

	return i;
}

void audiostream_close_file(int i, int fade)
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	if (fade) {
		Audio_streams[i].Fade_and_Destroy();
	} else {
		Audio_streams[i].Destroy();
	}
}

void audiostream_close_all(int fade)
{
	int i;

	if ( !Audiostream_inited ) {
		return;
	}

	int size = (int)Audio_streams.size();

	for (i = 0; i < size; i++) {
		if (Audio_streams[i].type == ASF_NONE) {
			continue;
		}

		audiostream_close_file(i, fade);
	}
}

void audiostream_play(int i, float volume, int looping)
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	if (volume < 0.0f) {
		volume = Audio_streams[i].Get_Default_Volume();
	}

	Audio_streams[i].Set_Default_Volume(volume);
	Audio_streams[i].Play(volume, looping);
}

// use as buffer service function
bool audiostream_is_playing(int i)
{
	if ( !Audiostream_inited ) {
		return false;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return false;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return false;
	}

	return Audio_streams[i].Is_Playing();
}

void audiostream_stop(int i, int rewind, int paused)
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	if (rewind) {
		Audio_streams[i].Stop_and_Rewind();
	} else {
		Audio_streams[i].Stop(paused);
	}
}

void audiostream_set_volume_all(float volume, int type)
{
	int i;

	if ( !Audiostream_inited ) {
		return;
	}

	int size = (int)Audio_streams.size();

	for (i = 0; i < size; i++) {
		if (Audio_streams[i].type == ASF_NONE) {
			continue;
		}

		if (Audio_streams[i].type == type) {
			Audio_streams[i].Set_Volume(volume);
		}
	}
}

void audiostream_set_volume(int i, float volume)
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	Audio_streams[i].Set_Volume(volume);
}

bool audiostream_is_paused(int i)
{
	if ( !Audiostream_inited ) {
		return false;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return false;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return false;
	}

	return Audio_streams[i].Is_Paused();
}

void audiostream_set_byte_cutoff(int i, uint cutoff)
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	Audio_streams[i].Set_Byte_Cutoff(cutoff);
}

uint audiostream_get_bytes_committed(int i)
{
	if ( !Audiostream_inited ) {
		return 0;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return 0;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return 0;
	}

	return Audio_streams[i].Get_Bytes_Committed();
}

bool audiostream_done_reading(int i)
{
	if ( !Audiostream_inited ) {
		return true;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return true;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return true;
	}

	return Audio_streams[i].Is_Past_Limit();
}

int audiostream_is_inited()
{
	return Audiostream_inited;
}

void audiostream_pause(int i)
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	if ( audiostream_is_playing(i) ) {
		audiostream_stop(i, 0, 1);
	}
}

void audiostream_pause_all()
{
	int i;

	if ( !Audiostream_inited ) {
		return;
	}

	int size = (int)Audio_streams.size();

	for (i = 0; i < size; i++) {
		if (Audio_streams[i].type == ASF_NONE) {
			continue;
		}

		audiostream_pause(i);
	}
}

void audiostream_unpause(int i)
{
	int is_looping;

	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( i >= 0 );
	SDL_assert( i < (int)Audio_streams.size() );

	if (i < 0) {
		return;
	}

	if (Audio_streams[i].type == ASF_NONE) {
		return;
	}

	if ( audiostream_is_paused(i) ) {
		is_looping = Audio_streams[i].Is_looping();
		audiostream_play(i, -1.0f, is_looping);
	}
}

void audiostream_unpause_all()
{
	int i;

	if ( !Audiostream_inited ) {
		return;
	}

	int size = (int)Audio_streams.size();

	for (i = 0; i < size; i++) {
		if (Audio_streams[i].type == ASF_NONE) {
			continue;
		}

		audiostream_unpause(i);
	}
}
