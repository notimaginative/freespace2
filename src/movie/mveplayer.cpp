#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "mvelib.h"                                             /* next buffer */
#include "bmpman.h"
#include "2d.h"
#include "mve_audio.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


static int g_spdFactorNum=0;
static int g_spdFactorDenom=10;

void initializeMovie(MVESTREAM *mve);
void playMovie(MVESTREAM *mve);
void shutdownMovie(MVESTREAM *mve);

#if 0
static int doPlay(const char *filename)
{
    MVESTREAM *mve = mve_open(filename);
    if (mve == NULL)
    {
        fprintf(stderr, "can't open MVE file '%s'\n", filename);
        return 1;
    }

    initializeMovie(mve);
    playMovie(mve);
    shutdownMovie(mve);

    mve_close(mve);

    return 0;
}
#endif

static short get_short(unsigned char *data)
{
    short value;
    value = data[0] | (data[1] << 8);
    return value;
}

static unsigned short get_ushort(unsigned char *data)
{
    unsigned short value;
    value = data[0] | (data[1] << 8);
    return value;
}

static int get_int(unsigned char *data)
{
    int value;
    value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

static int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
/*    fprintf(stderr, "unknown chunk type %02x/%02x\n", major, minor); */
    return 1;
}

/*************************
 * general handlers
 *************************/
static int end_movie_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    return 0;
}

/*************************
 * timer handlers
 *************************/

/*
 * timer variables
 */
static int micro_frame_delay=0;
static int timer_started=0;
static struct timeval timer_expire = {0, 0};

static int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    __extension__ long long temp;
    micro_frame_delay = get_int(data) * (int)get_short(data+4);
    if (g_spdFactorNum != 0)
    {
        temp = micro_frame_delay;
        temp *= g_spdFactorNum;
        temp /= g_spdFactorDenom;
        micro_frame_delay = (int)temp;
    }

    return 1;
}

static void timer_start(void)
{
    int nsec=0;
    gettimeofday(&timer_expire, NULL);
    timer_expire.tv_usec += micro_frame_delay;
    if (timer_expire.tv_usec > 1000000)
    {
        nsec = timer_expire.tv_usec / 1000000;
        timer_expire.tv_sec += nsec;
        timer_expire.tv_usec -= nsec*1000000;
    }
    timer_started=1;
}

static void do_timer_wait(void)
{
    int nsec=0;
    struct timespec ts, tsRem;
    struct timeval tv;
    if (! timer_started)
        return;

    gettimeofday(&tv, NULL);
    if (tv.tv_sec > timer_expire.tv_sec)
        goto end;
    else if (tv.tv_sec == timer_expire.tv_sec  &&  tv.tv_usec >= timer_expire.tv_usec)
        goto end;

    ts.tv_sec = timer_expire.tv_sec - tv.tv_sec;
    ts.tv_nsec = 1000 * (timer_expire.tv_usec - tv.tv_usec);
    if (ts.tv_nsec < 0)
    {
        ts.tv_nsec += 1000000000UL;
        --ts.tv_sec;
    }
    if (nanosleep(&ts, &tsRem) == -1  &&  errno == EINTR)
       exit(1);

end:
    timer_expire.tv_usec += micro_frame_delay;
    if (timer_expire.tv_usec > 1000000)
    {
        nsec = timer_expire.tv_usec / 1000000;
        timer_expire.tv_sec += nsec;
        timer_expire.tv_usec -= nsec*1000000;
    }
}

/*************************
 * audio handlers
 *************************/
static void mve_audio_callback(void *userdata, unsigned char *stream, int len); 
static short *mve_audio_buffers[64];
static int    mve_audio_buflens[64];
static int    mve_audio_curbuf_curpos=0;
static int mve_audio_bufhead=0;
static int mve_audio_buftail=0;
static int mve_audio_playing=0;
static int mve_audio_canplay=0;
#if 0
static SDL_AudioSpec *mve_audio_spec=NULL;
#endif
static int mve_audio_compressed = 0;

static void mve_audio_callback(void *userdata, unsigned char *stream, int len)
{
#if 0
    int total=0;
    int length;
    if (mve_audio_bufhead == mve_audio_buftail)
        return /* 0 */;

//fprintf(stderr, "+ <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);

    while (mve_audio_bufhead != mve_audio_buftail                                       /* while we have more buffers  */
            &&  len > (mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos))   /* and while we need more data */
    {
        length = mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos;
        memcpy(stream,                                                        /* cur output position */
                ((unsigned char *)mve_audio_buffers[mve_audio_bufhead])+mve_audio_curbuf_curpos,           /* cur input position  */
                length);                                                                /* cur input length    */

        total += length;
        stream += length;                                                               /* advance output */
        len -= length;                                                                  /* decrement avail ospace */
        free(mve_audio_buffers[mve_audio_bufhead]);                                     /* free the buffer */
        mve_audio_buffers[mve_audio_bufhead]=NULL;                                      /* free the buffer */
        mve_audio_buflens[mve_audio_bufhead]=0;                                         /* free the buffer */

        if (++mve_audio_bufhead == 64)                                                  /* next buffer */
            mve_audio_bufhead = 0;
        mve_audio_curbuf_curpos = 0;
    }

//fprintf(stderr, "= <%d (%d), %d, %d>: %d\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len, total);
/*    return total; */

    if (len != 0                                                                        /* ospace remaining  */
            &&  mve_audio_bufhead != mve_audio_buftail)                                 /* buffers remaining */
    {
        memcpy(stream,                                                        /* dest */
                ((unsigned char *)mve_audio_buffers[mve_audio_bufhead]) + mve_audio_curbuf_curpos,         /* src */
                len);                                                                   /* length */

        mve_audio_curbuf_curpos += len;                                                 /* advance input */
        stream += len;                                                                  /* advance output (unnecessary) */
        len -= len;                                                                     /* advance output (unnecessary) */

        if (mve_audio_curbuf_curpos >= mve_audio_buflens[mve_audio_bufhead])            /* if this ends the current chunk */
        {
            free(mve_audio_buffers[mve_audio_bufhead]);                                 /* free buffer */
            mve_audio_buffers[mve_audio_bufhead]=NULL;
            mve_audio_buflens[mve_audio_bufhead]=0;

            if (++mve_audio_bufhead == 64)                                              /* next buffer */
                mve_audio_bufhead = 0;
            mve_audio_curbuf_curpos = 0;
        }
    }

//fprintf(stderr, "- <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);
#endif
}

static int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#if 0
    int flags;
    int sample_rate;
    int desired_buffer;
	
    int stereo;
    int bitsize;
    int compressed;
    
    int format;
    
    flags = get_ushort(data + 2);
    sample_rate = get_ushort(data + 4);
    desired_buffer = get_int(data + 6);
    
    stereo = (flags & 0x0001) ? 1 : 0;
    bitsize = (flags & 0x0002) ? 1 : 0;
    
    if (minor > 0) {
    	compressed = flags & 0x0004 ? 1 : 0;
    } else {
	compressed = 0;
    }

    mve_audio_compressed = compressed;
        
    if (bitsize == 1) {
    	format = AUDIO_S16LSB;
    } else {
    	format = AUDIO_U8;
    }
    
    fprintf(stderr, "creating audio buffers:\n");
    fprintf(stderr, "sample rate = %d, stereo = %d, bitsize = %d, compressed = %d\n", 
    	sample_rate, stereo, bitsize ? 16 : 8, compressed);
    
    mve_audio_spec = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
    mve_audio_spec->freq = sample_rate;
    mve_audio_spec->format = format;
    mve_audio_spec->channels = (stereo) ? 2 : 1;
    mve_audio_spec->samples = 4096;
    mve_audio_spec->callback = mve_audio_callback;
    mve_audio_spec->userdata = NULL;
    if (SDL_OpenAudio(mve_audio_spec, NULL) >= 0)
    {
fprintf(stderr, "   success\n");
        mve_audio_canplay = 1;
    }
    else
    {
fprintf(stderr, "   failure : %s\n", SDL_GetError());
        mve_audio_canplay = 0;
    }

    memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
    memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));

#endif
    return 1;
}

static int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#if 0
    if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail)
    {
        SDL_PauseAudio(0);
        mve_audio_playing = 1;
    }
#endif
    return 1;
}

static int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#if 0
    static const int selected_chan=1;
    int chan;
    int nsamp;
    if (mve_audio_canplay)
    {
        if (mve_audio_playing)
            SDL_LockAudio();

        chan = get_ushort(data + 2);
        nsamp = get_ushort(data + 4);
        if (chan & selected_chan)
        {
            /* HACK: +4 mveaudio_uncompress adds 4 more bytes */
            if (major == 8) {
                if (mve_audio_compressed) {
                	nsamp += 4;
                	
                        mve_audio_buflens[mve_audio_buftail] = nsamp;
                        mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);
	                mveaudio_uncompress(mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
		} else {
			nsamp -= 8;
			data += 8;
			
              		mve_audio_buflens[mve_audio_buftail] = nsamp;
                        mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);
			memcpy(mve_audio_buffers[mve_audio_buftail], data, nsamp);
		}	              
            } else {
                mve_audio_buflens[mve_audio_buftail] = nsamp;
                mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);
                
                memset(mve_audio_buffers[mve_audio_buftail], 0, nsamp); /* XXX */
	    }
	    
            if (++mve_audio_buftail == 64)
                mve_audio_buftail = 0;

            if (mve_audio_buftail == mve_audio_bufhead)
                fprintf(stderr, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
        }

        if (mve_audio_playing)
            SDL_UnlockAudio();
    }
#endif

    return 1;
}

/*************************
 * video handlers
 *************************/
int g_width, g_height;
void *g_vBackBuf1, *g_vBackBuf2;

#if 0
static SDL_Surface *g_screen;
#endif
static int g_screenWidth, g_screenHeight;
static unsigned char g_palette[768];
static unsigned char *g_pCurMap=NULL;
static int g_nMapLength=0;
static int g_truecolor;

static int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short w, h;
    short count, truecolor;
    w = get_short(data);
    h = get_short(data+2);
    
    if (minor > 0) {
	    count = get_short(data+4);
    } else {
    	count = 1;
    }
    
    if (minor > 1) {
    	truecolor = get_short(data+6);
    } else {
        truecolor = 0;
    }
    
    g_width = w << 3;
    g_height = h << 3;
    
    /* TODO: * 4 causes crashes on some files */
    g_vBackBuf1 = malloc(g_width * g_height * 8);
    if (truecolor) {
    	g_vBackBuf2 = (unsigned short *)g_vBackBuf1 + (g_width * g_height);
    } else {
    	g_vBackBuf2 = (unsigned char *)g_vBackBuf1 + (g_width * g_height);
    }
        
    memset(g_vBackBuf1, 0, g_width * g_height * 4);
    
    fprintf(stderr, "DEBUG: w,h=%d,%d count=%d, tc=%d\n", w, h, count, truecolor);
    
    g_truecolor = truecolor;
    
    return 1;
}

#if 0
static int do_sdl_events()
{
	SDL_Event event;
	int retr = 0;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				exit(0);
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					exit(0);
				break;
			case SDL_KEYUP:
				retr = 1;
				break;
			case SDL_MOUSEBUTTONDOWN:
/*
				if (event.button.button == SDL_BUTTON_LEFT) {
					printf("GRID: %d,%d (pix:%d,%d)\n", 
					event.button.x / 16, event.button.y / 8,
					 event.button.x, event.button.y);
				}
*/
				break;
			default:
				break;
		}
	}
	
	return retr;
}
#endif

static unsigned short stab[65536];
static unsigned int itab[65536];
static int table_inited;

unsigned short *pixelbuf;
static void ConvertAndDraw()
{
    int i;
    
    unsigned short *pDests;
    unsigned int *pDesti;
    unsigned short *pSrcs;
    unsigned char *pixels = (unsigned char *)g_vBackBuf1;
    int x, y;

if (g_truecolor) {
	pSrcs = (unsigned short *)pixels;
	
	/*
	if (table_inited == 0) {
		int r, g, b;
		
		table_inited = 1;
		
		for (i = 0; i < 65536; i++) {
			r = (i & 0x7c00) >> 10;
			g = (i & 0x03e0) >> 5;
			b = (i & 0x001f) >> 0;
			
			stab[i]  = 1 << 15;
			stab[i] |= (r << 10)&0x7c00;
			stab[i] |= (g <<  5)&0x03e0;
			stab[i] |= (b <<  0)&0x001f;
		}
	}
	*/
	
	pDests = pixelbuf;

	if (g_screenWidth > g_width) {
		pDests += ((g_screenWidth - g_width) / 2) / 2;
	}
	if (g_screenHeight > g_height) {
		pDests += ((g_screenHeight - g_height) / 2) * g_screenWidth;
	}

	for (y=0; y<g_height; y++) {
		for (x = 0; x < g_width; x++) {
			//pDests[x] = stab[*pSrcs];
			pDests[x] = (1<<15)|*pSrcs;
		
			pSrcs++;
		}
		pDests += g_screenWidth;
	}
} else {
#if 0
/* original slow 8 bit code */
    int i;
    unsigned char *pal = g_palette;
    unsigned char *pDest;
    unsigned char *pixels = g_vBackBuf1;
    SDL_Surface *screenSprite, *initSprite;
    SDL_Rect renderArea;
    int x, y;

       initSprite = SDL_CreateRGBSurface(SDL_SWSURFACE, g_width, g_height, 8, 0, 0, 0, 0);

    if (!g_truecolor) {
    	for(i = 0; i < 256; i++)
	    {
	        initSprite->format->palette->colors[i].r = (*pal++) << 2;
	        initSprite->format->palette->colors[i].g = (*pal++) << 2;
	        initSprite->format->palette->colors[i].b = (*pal++) << 2;
	        initSprite->format->palette->colors[i].unused = 0;
	    }    
    }
    
    pDest = initSprite->pixels;
    for (i=0; i<g_height; i++)
    {
        memcpy(pDest, pixels, g_width * (g_truecolor?2:1));
        pixels += g_width* (g_truecolor?2:1);
        pDest += initSprite->pitch;
    }

    screenSprite = SDL_DisplayFormat(initSprite);
    SDL_FreeSurface(initSprite);

    if (g_screenWidth > screenSprite->w) x = (g_screenWidth - screenSprite->w) >> 1;
    else x=0;
    if (g_screenHeight > screenSprite->h) y = (g_screenHeight - screenSprite->h) >> 1;
    else y=0;
    renderArea.x = x;
    renderArea.y = y;
    renderArea.w = MIN(g_screenWidth  - x, screenSprite->w);
    renderArea.h = MIN(g_screenHeight - y, screenSprite->h);
    SDL_BlitSurface(screenSprite, NULL, g_screen, &renderArea);

	SDL_FreeSurface(screenSprite);
#endif
}

}

static int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	ConvertAndDraw();
#if 0
		
	SDL_Flip(g_screen);
	
	do_sdl_events();
#endif
	/* DDOI - This is probably really fricking slow */
	int bitmap = bm_create (16, g_screenWidth, g_screenHeight, pixelbuf, 0);
	gr_set_bitmap (bitmap);
	gr_bitmap (0, 0);
	bm_release (bitmap);
	gr_flip ();
	
	return 1;
}

static int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short width, height;
    width = get_short(data);
    height = get_short(data+2);
#if 0
    g_screen = SDL_SetVideoMode(width, height, 16, SDL_ANYFORMAT|SDL_DOUBLEBUF);
#endif
    // DDOI - Allocate RGB565 pixel buffer
    pixelbuf = (unsigned short *)malloc (width * height * 2);
    g_screenWidth = width;
    g_screenHeight = height;
    memset(g_palette, 0, 768);
    return 1;
}

static int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short start, count;
    start = get_short(data);
    count = get_short(data+2);
    memcpy(g_palette + 3*start, data+4, 3*count);
    return 1;
}

static int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    g_pCurMap = data;
    g_nMapLength = len;
    return 1;
}

void decodeFrame16(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain);

static int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short nFrameHot, nFrameCold;
    short nXoffset, nYoffset;
    short nXsize, nYsize;
    unsigned short nFlags;
    unsigned char *temp;

    nFrameHot  = get_short(data);
    nFrameCold = get_short(data+2);
    nXoffset   = get_short(data+4);
    nYoffset   = get_short(data+6);
    nXsize     = get_short(data+8);
    nYsize     = get_short(data+10);
    nFlags     = get_ushort(data+12);

    if (nFlags & 1)
    {
        temp = (unsigned char *)g_vBackBuf1;
        g_vBackBuf1 = g_vBackBuf2;
        g_vBackBuf2 = temp;
    }

    /* convert the frame */
    if (g_truecolor) {
	decodeFrame16((unsigned char *)g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
#if 0
    } else {
    	decodeFrame8(g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
#endif
    }

    return 1;
}

static int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    g_pCurMap=NULL;
    return 1;
}

void initializeMovie(MVESTREAM *mve)
{
    int i;
    
    for (i = 0; i < 32; i++) {
    	mve_set_handler(mve, i, default_seg_handler);
    }
    
    mve_set_handler(mve, 0x00, end_movie_handler);
    mve_set_handler(mve, 0x01, end_chunk_handler);
    mve_set_handler(mve, 0x02, create_timer_handler);
    mve_set_handler(mve, 0x03, create_audiobuf_handler);
    mve_set_handler(mve, 0x04, play_audio_handler);
    mve_set_handler(mve, 0x05, create_videobuf_handler);
    mve_set_handler(mve, 0x07, display_video_handler);
    mve_set_handler(mve, 0x08, audio_data_handler);
    mve_set_handler(mve, 0x09, audio_data_handler);
    mve_set_handler(mve, 0x0a, init_video_handler);
    mve_set_handler(mve, 0x0c, video_palette_handler);
    mve_set_handler(mve, 0x0f, video_codemap_handler);
    mve_set_handler(mve, 0x11, video_data_handler);
}

void playMovie(MVESTREAM *mve)
{
    int init_timer=0;
    int cont=1;
    while (cont)
    {
        cont = mve_play_next_chunk(mve);
        if (micro_frame_delay  &&  !init_timer)
        {
            timer_start();
            init_timer = 1;
        }

        do_timer_wait();
    }
}

void shutdownMovie(MVESTREAM *mve)
{
	free (pixelbuf);
}
