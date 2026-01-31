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
#include <climits>

#include "pstypes.h"
#include "audiostr.h"
#include "acm.h"
#include "cfile.h"
#include "sound.h"
#include "timer.h"


// status
#define ASF_FREE	0
#define ASF_USED	1

static int Audiostream_inited = 0;

static SDL_AudioDeviceID Audiostream_device = 0;
static SDL_AudioSpec Audiostream_spec;


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

	WAVE_chunk m_wfmt;					// format of wave file
	WAVE_chunk *m_pwfmt_original;	// foramt of wave file from actual wave source
	uint m_total_uncompressed_bytes_read;
	uint m_max_uncompressed_bytes_to_read;
	uint m_bits_per_sample_uncompressed;

protected:
	uint m_data_offset;						// number of bytes to actual wave data
	int  m_data_bytes_left;
	CFILE *cfp;

	uint m_wave_format;						// format of wave source (ie WAVE_FORMAT_PCM, WAVE_FORMAT_ADPCM)
	uint m_nBlockAlign;						// wave data block alignment spec
	uint m_nUncompressedAvgDataRate;		// average wave data rate
	uint m_nDataSize;							// size of data chunk
	uint m_nBytesPlayed;						// offset into data chunk
	bool m_abort_next_read;

	uint8_t *m_comp_buffer;
	uint m_comp_buffer_size;

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
	void Stop(bool paused = false);
	void Stop_and_Rewind();
	void Fade_and_Destroy();
	void Fade_and_Stop();
	void Set_Volume(float vol);
	float Get_Volume();
	void Init_Data();
	void Set_Byte_Cutoff(uint num_bytes_cutoff);
	uint Get_Bytes_Committed();

	bool Is_Playing()
	{
		return m_fPlaying;
	}

	bool Is_Paused()
	{
		return m_bIsPaused;
	}

	bool Is_Past_Limit()
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

	bool Is_looping()
	{
		return m_bLooping;
	}

	void Do_Frame();

	int status;
	int	type;
	ushort m_bits_per_sample_uncompressed;

protected:
	void Cue();
	bool WriteWaveData(uint* num_bytes_written = nullptr);
	uint GetMaxWriteSize();

	static void SDLCALL ServiceBuffer(void *userdata, SDL_AudioStream *stream,
									  int additional_amount, int total_amount);

	SDL_AudioStream *m_audio_stream;

	WaveFile *m_pwavefile;        // ptr to WaveFile object
	bool m_fCued;                  // semaphore (stream cued)
	bool m_fPlaying;               // semaphore (stream playing)
	uint m_nTimeStarted;           // time (in system time) playback started

	uint8_t *m_cbBufData;				// uncompressed sound buffer
	uint m_cbBufSize;				// size of sound buffer in bytes

	bool	m_bLooping;						// whether or not to loop playback
	bool	m_bFade;							// fade out music
	bool	m_bDestroy_when_faded;
	float	m_lVolume;						// volume of stream ( 0.0f -> 1.0f )
	float	m_lCutoffVolume;
	bool	m_bIsPaused;					// stream is stopped, but not rewinded
	bool	m_bReadingDone;				// no more bytes to be read from disk, still have remaining buffer to play
	int		m_fade_timer_id;				// timestamp so we know when to start fade
	int		m_finished_id;					// timestamp so we know when we've played #bytes required
	bool	m_bPastLimit;					// flag to show we've played past the number of bytes requred
	float	m_lDefaultVolume;
};


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
	m_max_uncompressed_bytes_to_read = UINT_MAX;
	SDL_zero(m_wfmt);
	SDL_zero(m_wfxDest);

	m_comp_buffer = nullptr;
	m_comp_buffer_size = 0;

	m_hStream_open = 0;
	m_abort_next_read = false;
}

void WaveFile::Close()
{
	// Free memory
	if (m_pwfmt_original) {
		if (m_pwfmt_original->extra_data) {
			free(m_pwfmt_original->extra_data);
		}

		free(m_pwfmt_original);
		m_pwfmt_original = NULL;
	}

	if (m_comp_buffer) {
		free(m_comp_buffer);
		m_comp_buffer = nullptr;
		m_comp_buffer_size = 0;
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
	m_max_uncompressed_bytes_to_read = UINT_MAX;

	m_pwfmt_original = (WAVE_chunk*) malloc (sizeof(WAVE_chunk));

	if (m_pwfmt_original == NULL) {
		goto OPEN_ERROR;
	}

	SDL_zerop(m_pwfmt_original);

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
		if (m_pwfmt_original->extra_data) {
			free(m_pwfmt_original->extra_data);
		}

		free(m_pwfmt_original);
		m_pwfmt_original = NULL;
	}

	if (m_comp_buffer) {
		free(m_comp_buffer);
		m_comp_buffer = nullptr;
		m_comp_buffer_size = 0;
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

	m_total_uncompressed_bytes_read = 0;
	m_max_uncompressed_bytes_to_read = UINT_MAX;

	if ( !cfseek(cfp, m_data_offset, CF_SEEK_SET) ) {
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
int WaveFile::Read(uint8_t *pbDest, uint cbSize, int service)
{
	void *dest_buf = NULL, *uncompressed_wave_data;
	int rc, uncompressed_bytes_written;
	uint src_bytes_used, convert_len, num_bytes_desired=0, num_bytes_read;

//	nprintf(("Alan","Reqeusted: %d\n", cbSize));

	uncompressed_wave_data = pbDest;

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

			if ( !m_comp_buffer || (m_comp_buffer_size < cbSize) ) {
				SDL_assert(cbSize > 0);

				if (m_comp_buffer) {
					free(m_comp_buffer);
				}

				m_comp_buffer_size = cbSize;
				m_comp_buffer = reinterpret_cast<uint8_t *>(malloc(m_comp_buffer_size));
			}

			dest_buf = m_comp_buffer;

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
			rc = ACM_convert((void*)m_hStream, (ubyte*)dest_buf, num_bytes_read,
							 (ubyte*)uncompressed_wave_data, cbSize, &convert_len,
							 &src_bytes_used);

			if (rc == -1) {
				goto READ_ERROR;
			}

//			if (convert_len == 0) {
//				Int3();
//			}
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

// Constructor
AudioStream::AudioStream()
{
}

// Destructor
AudioStream::~AudioStream()
{
}

void AudioStream::Init_Data()
{
	m_audio_stream = nullptr;

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
	m_bReadingDone = false;

	m_pwavefile = NULL;

	m_fPlaying = m_fCued = false;
	m_nTimeStarted = 0;

	m_cbBufSize = 0;
	m_cbBufData = nullptr;
}

// Create
bool AudioStream::Create(const char *pszFilename)
{
	SDL_assert( pszFilename != NULL );

	Init_Data();

	if (pszFilename == NULL) {
		return false;
	}

	// make 100% sure we got a good filename
	if ( !SDL_strlen(pszFilename) ) {
		return false;
	}

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
		// Calculate sound buffer size in bytes for 1 second of audio
		m_cbBufSize = (m_pwavefile->m_wfmt.bits_per_sample/8) * m_pwavefile->m_wfmt.num_channels * m_pwavefile->m_wfmt.sample_rate;
		// shouldn't need more than 250ms worth at a time
		m_cbBufSize /= 4;

		m_cbBufData = reinterpret_cast<uint8_t *>(malloc(m_cbBufSize));

		if ( !m_cbBufData ) {
			nprintf(("SOUND", "SOUND => Failed to create wave audio buffer for %s\n", pszFilename));

			m_pwavefile->Close();

			free(m_pwavefile);
			m_pwavefile = nullptr;

			return false;
		}

//		nprintf(("SOUND", "SOUND => Stream buffer created using %d bytes\n", m_cbBufSize));

		// Create sound buffer
		if ( !m_audio_stream ) {
			SDL_AudioSpec spec{};

			spec.format = (m_pwavefile->m_wfmt.bits_per_sample == 16) ? SDL_AUDIO_S16LE : SDL_AUDIO_U8;
			spec.channels = m_pwavefile->m_wfmt.num_channels;
			spec.freq = m_pwavefile->m_wfmt.sample_rate;

			m_audio_stream = SDL_CreateAudioStream(&spec, &Audiostream_spec);
		}

		Snd_sram += m_cbBufSize;
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
	SDL_DestroyAudioStream(m_audio_stream);
	m_audio_stream = nullptr;

	Snd_sram -= m_cbBufSize;

	// Delete WaveFile object
	if (m_pwavefile) {
		m_pwavefile->Close();

		free(m_pwavefile);
		m_pwavefile = NULL;
	}

	status = ASF_FREE;

	return true;
}

// WriteWaveData
//
// Writes wave data to sound buffer. This is a helper method used by Create and
// ServiceBuffer; it's not exposed to users of the AudioStream class.
bool AudioStream::WriteWaveData(uint *num_bytes_written)
{
	if (num_bytes_written) {
		*num_bytes_written = 0;
	}

	if (m_bReadingDone) {
		return true;
	}

	if ( !m_audio_stream || !m_pwavefile ) {
		return true;
	}

	int num_bytes_read = 0;

	num_bytes_read = m_pwavefile->Read(m_cbBufData, m_cbBufSize);

	// if looping then maybe reset wavefile and keep going
	if ((num_bytes_read < 0) && m_bLooping) {
		m_pwavefile->Cue();
		num_bytes_read = m_pwavefile->Read(m_cbBufData, m_cbBufSize);
	}

	if (num_bytes_read < 0) {
		m_bReadingDone = true;
		// we're done adding more data so let SDL know to use all that's left
		SDL_FlushAudioStream(m_audio_stream);
	} else if (num_bytes_read > 0) {
		SDL_PutAudioStreamData(m_audio_stream, m_cbBufData, num_bytes_read);

		if (num_bytes_written) {
			*num_bytes_written += num_bytes_read;
		}
	}

	return true;
}

#define VOLUME_ATTENUATION_BEFORE_CUTOFF	0.03f		//  12db
#define VOLUME_ATTENUATION					0.65f

void SDLCALL AudioStream::ServiceBuffer(void *userdata, SDL_AudioStream *stream,
										int additional_amount, int total_amount)
{
	// stream doesn't actually need more data, so bail
	if (additional_amount <= 0) {
		return;
	}

	auto info = reinterpret_cast<AudioStream *>(userdata);

	// adjust buffer size if necessary
	if (static_cast<uint>(additional_amount) > info->m_cbBufSize) {
		if (info->m_cbBufData) {
			free(info->m_cbBufData);
		}

		info->m_cbBufData = reinterpret_cast<uint8_t *>(malloc(additional_amount));
		info->m_cbBufSize = static_cast<uint>(additional_amount);
	}

	// read in additional wave data
	info->WriteWaveData();
}

// Cue
void AudioStream::Cue()
{
	if ( !m_fCued ) {
		m_bFade = false;
		m_fade_timer_id = 0;
		m_finished_id = 0;
		m_bPastLimit = false;
		m_lVolume = 1.0f;
		m_lCutoffVolume = 0.0f;

		m_bDestroy_when_faded = false;

		// Reset file ptr, etc
		m_pwavefile->Cue();

		// Unqueue all buffers
		SDL_ClearAudioStream(m_audio_stream);

		// Fill buffer with wave data
		WriteWaveData();

		m_fCued = true;
	}
}

// Play
void AudioStream::Play(float volume, int looping)
{
	// If playing, stop
	if (m_fPlaying) {
		if ( m_bIsPaused == false)
			Stop_and_Rewind();
	}

	// create stream if we don't have one
	if ( !m_audio_stream ) {
		SDL_AudioSpec spec{};

		spec.format = (m_pwavefile->m_wfmt.bits_per_sample == 16) ? SDL_AUDIO_S16LE : SDL_AUDIO_U8;
		spec.channels = m_pwavefile->m_wfmt.num_channels;
		spec.freq = m_pwavefile->m_wfmt.sample_rate;

		m_audio_stream = SDL_CreateAudioStream(&spec, &Audiostream_spec);

		if ( !m_audio_stream ) {
			return;
		}
	}

	if (looping == 1) {
		m_bLooping = true;
	} else {
		m_bLooping = false;
	}

	// Cue for playback if necessary
	if (!m_fCued) {
		Cue ();
	}

	m_nTimeStarted = timer_get_milliseconds();
	Set_Volume(volume);

	SDL_SetAudioStreamGain(m_audio_stream, m_lVolume);
	SDL_SetAudioStreamGetCallback(m_audio_stream, ServiceBuffer, this);

	// once bound it should start playing immediately
	SDL_BindAudioStream(Audiostream_device, m_audio_stream);

	// Playback begun, no longer cued
	m_fPlaying = true;
	m_bIsPaused = false;
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
void AudioStream::Stop(bool paused)
{
	if (m_fPlaying) {
		SDL_SetAudioStreamGetCallback(m_audio_stream, nullptr, nullptr);
		SDL_UnbindAudioStream(m_audio_stream);

		if ( !paused ) {
			SDL_ClearAudioStream(m_audio_stream);
		}

		m_fPlaying = false;
		m_bIsPaused = paused;
	}
}

// Stop_and_Rewind
void AudioStream::Stop_and_Rewind()
{
	if (m_fPlaying) {
		// Stop playback
		SDL_SetAudioStreamGetCallback(m_audio_stream, nullptr, nullptr);
		SDL_UnbindAudioStream(m_audio_stream);
		SDL_ClearAudioStream(m_audio_stream);

		m_fPlaying = false;
	}

	m_fCued = false;	// this will cause wave file to start from beginning
	m_bReadingDone = false;
}

// Set_Volume
void AudioStream::Set_Volume(float vol)
{
	SDL_SetAudioStreamGain(m_audio_stream, vol);

	m_lVolume = vol;
}


// Set_Volume
float AudioStream::Get_Volume()
{
	return m_lVolume;
}

// Things that should be looked after at regular intervals (such as every frame)
void AudioStream::Do_Frame()
{
	float vol;

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
			} else {
				Stop_and_Rewind();
			}

			return;
		}
	}

	if (m_pwavefile->m_total_uncompressed_bytes_read >= m_pwavefile->m_max_uncompressed_bytes_to_read) {
		m_fade_timer_id = timer_get_milliseconds() + 1700;		// start fading 1.7 seconds from now
		m_finished_id = timer_get_milliseconds() + 2000;		// 2 seconds left to play out buffer
		m_pwavefile->m_max_uncompressed_bytes_to_read = UINT_MAX;
	}

	if ( (m_fade_timer_id > 0) && (timer_get_milliseconds() > m_fade_timer_id) ) {
		m_fade_timer_id = 0;
		Fade_and_Stop();
	}

	if ( (m_finished_id > 0) && (timer_get_milliseconds() > m_finished_id) ) {
		m_finished_id = 0;
		m_bPastLimit = true;
	}

	// see if we're done
	if ( m_bReadingDone && (SDL_GetAudioStreamQueued(m_audio_stream) < 1) ) {
		if (m_bDestroy_when_faded) {
			// All of sound has played, and we're done with it
			Destroy();
		} else if (m_bLooping && !m_bFade) {
			// All of sound has played, loop again
			Play(m_lVolume, m_bLooping);
		} else {
			// All of sound has played, stop playback
			Stop_and_Rewind();
		}
	}
}

#define MAX_AUDIO_STREAMS	30
static AudioStream *Audio_streams = NULL;


void audiostream_init()
{
	if (Audiostream_inited) {
		return;
	}

	// request a format that matches the best possible quality we'll play
	Audiostream_spec.freq = 22050;
	Audiostream_spec.channels = 2;
	Audiostream_spec.format = SDL_AUDIO_S16LE;

	Audiostream_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
											 &Audiostream_spec);

	if ( !Audiostream_device ) {
		return;
	}

	// now get actual format
	SDL_GetAudioDeviceFormat(Audiostream_device, &Audiostream_spec, nullptr);

	if (Audio_streams == NULL) {
		Audio_streams = (AudioStream*)malloc(sizeof(AudioStream) * MAX_AUDIO_STREAMS);

		if (Audio_streams == NULL) {
			goto INIT_ERROR;
		}
	}

	for (int i = 0; i < MAX_AUDIO_STREAMS; i++ ) {
		Audio_streams[i].Init_Data();
		Audio_streams[i].status = ASF_FREE;
		Audio_streams[i].type = ASF_NONE;
	}

	Audiostream_inited = 1;

	return;

INIT_ERROR:
	if (Audio_streams) {
		free(Audio_streams);
		Audio_streams = NULL;
	}

	if (Audiostream_device) {
		SDL_CloseAudioDevice(Audiostream_device);
		Audiostream_device = 0;
	}

	Audiostream_inited = 0;
}

// Close down the audiostream system.  Must call audiostream_init() before any audiostream functions can
// be used.
void audiostream_close()
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_assert( Audio_streams != NULL );

	for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
		if ( Audio_streams[i].status == ASF_USED ) {
			Audio_streams[i].status = ASF_FREE;
			Audio_streams[i].Destroy();
		}
	}

	free(Audio_streams);
	Audio_streams = NULL;

	if (Audiostream_device) {
		SDL_CloseAudioDevice(Audiostream_device);
		Audiostream_device = 0;
	}

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

	for (i = 0; i < MAX_AUDIO_STREAMS; i++) {
		if (Audio_streams[i].status == ASF_FREE) {
			Audio_streams[i].status = ASF_USED;
			Audio_streams[i].type = type;
			break;
		}
	}

	if (i == MAX_AUDIO_STREAMS) {
		nprintf(("Sound", "SOUND => No more audio streams available!\n"));
		return -1;
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
		Audio_streams[i].status = ASF_FREE;
		return -1;
	}

	return i;
}

void audiostream_close_file(int i, int fade)
{
	if ( !Audiostream_inited ) {
		return;
	}

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
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

	for (i = 0; i < MAX_AUDIO_STREAMS; i++) {
		if (Audio_streams[i].status == ASF_FREE) {
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

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
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

	if (i < 0) {
		return false;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return false;
	}

	return Audio_streams[i].Is_Playing();
}

void audiostream_stop(int i, int rewind, int paused)
{
	if ( !Audiostream_inited ) {
		return;
	}

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return;
	}

	if (rewind) {
		Audio_streams[i].Stop_and_Rewind();
	} else {
		Audio_streams[i].Stop( (paused != 0) );
	}
}

void audiostream_set_volume_all(float volume, int type)
{
	int i;

	if ( !Audiostream_inited ) {
		return;
	}

	for (i = 0; i < MAX_AUDIO_STREAMS; i++) {
		if (Audio_streams[i].status == ASF_FREE) {
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

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return;
	}

	Audio_streams[i].Set_Volume(volume);
}

bool audiostream_is_paused(int i)
{
	if ( !Audiostream_inited ) {
		return false;
	}

	if (i < 0) {
		return false;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return false;
	}

	return Audio_streams[i].Is_Paused();
}

void audiostream_set_byte_cutoff(int i, uint cutoff)
{
	if ( !Audiostream_inited ) {
		return;
	}

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return;
	}

	Audio_streams[i].Set_Byte_Cutoff(cutoff);
}

uint audiostream_get_bytes_committed(int i)
{
	if ( !Audiostream_inited ) {
		return 0;
	}

	if (i < 0) {
		return 0;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return 0;
	}

	return Audio_streams[i].Get_Bytes_Committed();
}

bool audiostream_done_reading(int i)
{
	if ( !Audiostream_inited ) {
		return true;
	}

	if (i < 0) {
		return true;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
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

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return;
	}

	if ( audiostream_is_playing(i) ) {
		audiostream_stop(i, 0, 1);
	}
}

void audiostream_pause_all()
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_PauseAudioDevice(Audiostream_device);
}

void audiostream_unpause(int i)
{
	int is_looping;

	if ( !Audiostream_inited ) {
		return;
	}

	if (i < 0) {
		return;
	}

	SDL_assert( i < MAX_AUDIO_STREAMS );

	if (Audio_streams[i].status == ASF_FREE) {
		return;
	}

	if ( audiostream_is_paused(i) ) {
		is_looping = Audio_streams[i].Is_looping();
		audiostream_play(i, -1.0f, is_looping);
	}
}

void audiostream_unpause_all()
{
	if ( !Audiostream_inited ) {
		return;
	}

	SDL_ResumeAudioDevice(Audiostream_device);
}

void audiostream_do_frame()
{
	if ( !Audiostream_inited ) {
		return;
	}

	for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
		if (Audio_streams[i].status == ASF_USED) {
			Audio_streams[i].Do_Frame();
		}
	}
}
