/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

/*
 * $Logfile: /Freespace2/code/Fonttool/FontKern.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Tool for interactively kerning fonts
 *
 * $Log$
 * Revision 1.4  2006/04/26 19:38:36  taylor
 * fix some minor fonttool compile errors
 *
 * Revision 1.3  2003/01/30 20:03:48  relnev
 * various files ported needed for fonttool.  There is a bug where on exit it segfaults in SDL_GL_SwapBuffers, I'm probably missing something (don't know what) but it works fine otherwise (Taylor Richards)
 *
 * Revision 1.2  2002/06/09 04:41:16  relnev
 * added copyright header
 *
 * Revision 1.1.1.1  2002/05/03 03:28:08  root
 * Initial import.
 *
 * 
 * 6     5/19/99 4:07p Dave
 * Moved versioning code into a nice isolated common place. Fixed up
 * updating code on the pxo screen. Fixed several stub problems.
 * 
 * 5     12/18/98 1:14a Dave
 * Rough 1024x768 support for Direct3D. Proper detection and usage through
 * the launcher.
 * 
 * 4     12/02/98 9:58a Dave
 * Got fonttool working under glide/direct3d.
 * 
 * 3     11/30/98 1:09p Dave
 * 
 * 2     10/24/98 5:15p Dave
 * 
 * 1     10/24/98 4:58p Dave
 * 
 * 14    5/06/98 5:30p John
 * Removed unused cfilearchiver.  Removed/replaced some unused/little used
 * graphics functions, namely gradient_h and _v and pixel_sp.   Put in new
 * DirectX header files and libs that fixed the Direct3D alpha blending
 * problems.
 * 
 * 13    4/13/98 10:11a John
 * Made timer functions thread safe.  Made timer_init be called in all
 * projects.
 * 
 * 12    3/10/98 4:18p John
 * Cleaned up graphics lib.  Took out most unused gr functions.   Made D3D
 * & Glide have popups and print screen.  Took out all >8bpp software
 * support.  Made Fred zbuffer.  Made zbuffer allocate dynamically to
 * support Fred.  Made zbuffering key off of functions rather than one
 * global variable.
 * 
 * 11    3/05/98 11:15p Hoffoss
 * Changed non-game key checking to use game_check_key() instead of
 * game_poll().
 * 
 * 10    10/30/97 4:56p John
 * Fixed up font stuff to build.  Fixed bug where it didn't show the last
 * 3 characters in kerning table.
 * 
 * 9     9/03/97 4:32p John
 * changed bmpman to only accept ani and pcx's.  made passing .pcx or .ani
 * to bm_load functions not needed.   Made bmpman keep track of palettes
 * for bitmaps not mapped into game palettes.
 * 
 * 8     6/06/97 6:47p John
 * Fixed bug
 * 
 * 7     6/06/97 4:41p John
 * Fixed alpha colors to be smoothly integrated into gr_set_color_fast
 * code.
 * 
 * 6     6/06/97 11:10a John
 * made scrolling kern pair box.
 * 
 * 5     6/06/97 9:21a John
 * added some kerning pairs
 * 
 * 4     6/06/97 9:18a John
 * Added capital hamburger.    
 * 
 * 3     6/05/97 5:00p John
 * used fonttool.pcx
 * 
 * 2     6/05/97 4:53p John
 * First rev of new antialiased font stuff.
 * 
 * 1     6/02/97 4:04p John
 *
 * $NoKeywords: $
 */

#ifndef PLAT_UNIX
#include <io.h>
#include <conio.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "pstypes.h"
#include "osapi.h"
#include "cfile.h"
#include "2d.h"
#include "key.h"
#include "mouse.h"
#include "palman.h"
#include "timer.h"
#include "bmpman.h"
#include "osregistry.h"
#include "cmdline.h"

#include "fonttool.h"

const char *SampleText = "This is some sample text that is here to\n" \
"Show you how the antialiasing will\n"	\
"look over different color backgrounds\n" \
"KERN PAIRS: VaWaVeWeVAV-LyT.T,TyTvTcYe\n";

static void myexit(int value)
{
#ifdef WIN32
	getch();
#endif
	exit(value);
}

int game_check_key()
{
	return key_inkey();
}

int game_poll()
{
	return key_inkey();
}

int os_done = 0;

void os_close()
{
	os_done = 1;
}

int fonttool_get_kerning( font *fnt, int c1, int c2, int *pairnum )
{
	int i;

	int l1 = c1 - fnt->first_ascii;
	int l2 = c2 - fnt->first_ascii;

	for (i=0; i<fnt->num_kern_pairs; i++ )	{
		if ( (fnt->kern_data[i].c1 == l1) && (fnt->kern_data[i].c2 == l2) )	{
			if (pairnum) *pairnum = i;
			return fnt->kern_data[i].offset;
		}
	}
	return 0;
}

void fonttool_resync_kerning( font *fnt )
{
	int i;

	// update all the font into
	for (i=0; i<fnt->num_chars; i++ )	{
		fnt->char_data[i].kerning_entry = -1;
	}

	for (i=0; i<fnt->num_kern_pairs; i++ )	{
		int c = fnt->kern_data[i].c1;
		if ( fnt->char_data[c].kerning_entry == -1 )
			fnt->char_data[c].kerning_entry = (short)i;
	}
}

void fonttool_remove_kern_pair( font *fnt, int index )
{
	// not found, add it
	int i, n, new_num_pairs;

	new_num_pairs = fnt->num_kern_pairs - 1;

	if ( new_num_pairs < 1 )	{
		fonttool_remove_kerning(fnt);
		return;
	}

	font_kernpair *new_kern_data = (font_kernpair *)malloc( new_num_pairs*sizeof(font_kernpair) );
	if (!new_kern_data)	{
		printf( "Out of memory!\n" );
		myexit(1);
	}


	n=0;
	for (i=0; i<fnt->num_kern_pairs; i++ )	{
		if ( i != index )	{
			new_kern_data[n] = fnt->kern_data[i];
			n++;
		}
	}

	if ( fnt->kern_data ) free( fnt->kern_data );
	fnt->kern_data = new_kern_data;
	fnt->kern_data_size = sizeof(font_kernpair)*new_num_pairs;
	fnt->num_kern_pairs = new_num_pairs;

	fonttool_resync_kerning(fnt);

	mprintf(( "Font has %d kern pairs\n", fnt->num_kern_pairs ));
}

void fonttool_set_kerning( font *fnt, int c1, int c2, int dist )
{
	int i;

	int l1 = c1 - fnt->first_ascii;
	int l2 = c2 - fnt->first_ascii;

	for (i=0; i<fnt->num_kern_pairs; i++ )	{
		if ( (fnt->kern_data[i].c1 == l1) && (fnt->kern_data[i].c2 == l2) )	{
			fnt->kern_data[i].offset = (signed char)dist;
			if ( dist == 0 )	{
				fonttool_remove_kern_pair( fnt, i );				
			}
			return;
		}
	}
	if ( dist == 0 )	return;

	// not found, add it
	int new_num_pairs;

	new_num_pairs = fnt->num_kern_pairs+1;

	font_kernpair *new_kern_data = (font_kernpair *)malloc( new_num_pairs*sizeof(font_kernpair) );
	if (!new_kern_data)	{
		printf( "Out of memory!\n" );
		myexit(1);
	}


	int n=0;
	uint newcode = l1*256+l2;

	for (i=0; i<fnt->num_kern_pairs; i++ )	{
		uint code = fnt->kern_data[i].c1*256 + fnt->kern_data[i].c2;
		if ( code < newcode )	{
			new_kern_data[n] = fnt->kern_data[i];
			n++;
		} else {
			break;
		}
	}


	new_kern_data[n].c1 = (char)l1;
	new_kern_data[n].c2 = (char)l2;
	new_kern_data[n].offset = (signed char)dist;
 	n++;

	
	for (; i<fnt->num_kern_pairs; i++ )	{
		new_kern_data[n] = fnt->kern_data[i];
		n++;
	}


	if ( fnt->kern_data ) free( fnt->kern_data );
	fnt->kern_data = new_kern_data;
	fnt->kern_data_size += 	sizeof(font_kernpair);
	fnt->num_kern_pairs++;

	fonttool_resync_kerning(fnt);

	mprintf(( "Font has %d kern pairs\n", fnt->num_kern_pairs ));

}


void fonttool_remove_kerning( font *fnt )
{
	int i;

	for (i=0; i<fnt->num_chars; i++ )	{
		fnt->char_data[i].kerning_entry = -1;
	}

	fnt->kern_data_size = 0;
	if (fnt->kern_data)
		free( fnt->kern_data );
	fnt->kern_data = NULL;
	fnt->num_kern_pairs = 0;
}


void fonttool_edit_kerning(char *fname1)
{
	int i, k,x;
	int done;
	int bkg;
	font KernFont, tmpfont;
	int alpha = 16;
	int cr=16, cb=16, cg=16;
	int c1 = 'b', c2 = 'u';
	char kerntext[128];
	int current_pair = -1;
	int first_item = 0;
	int current_item = 0;
	int num_items_displayed = 1;
	int last_good_pair = -1;
	color ac;
	
	printf( "Editing kerning data for %s\n", fname1 );
	fonttool_read( fname1, &KernFont );

	timer_init();

	// setup the fred exe directory so CFILE can init properly
	//char *c = GetCommandLine();
	//SDL_assert(c != NULL);
	//char *tok = strtok(c, " ");
	//SDL_assert(tok != NULL);	

	cfile_init();

	os_init( "FontTool", "FontTool - Kerning Table Editor" );

	// always run this thing in a window
	Cmdline_fullscreen = 0;
	Cmdline_window = 1;

	gr_init();

	gr_set_palette("none",NULL);

	bkg = bm_load("fonttool");

	// fallback, for if it's run in build tree
	if (bkg < 0) {
		char fonttool_pcx[128];
		snprintf(fonttool_pcx, sizeof(fonttool_pcx), "src%sfonttool%sfonttool", DIR_SEPARATOR_STR, DIR_SEPARATOR_STR);

		bkg = bm_load( fonttool_pcx );
	}

	if ( bkg < 0 )	{
		printf("Error loading FontTool\n" );
		myexit(1);
	}
	palette_use_bm_palette(bkg);

	key_init();
	mouse_init();

	gr_init_alphacolor( &ac, cr*16,cg*16,cb*16,alpha*16,AC_TYPE_HUD );
	

	{
		extern font *Current_font;
		Current_font = &KernFont;
	}

	done = 0;
	while (!done)	{
		
		os_poll();
		k = key_inkey();
		switch(k)	{		
		case SDLK_F3:
			gr_toggle_fullscreen();
			break;

		case SDLK_F5:
			fonttool_read( fname1, &tmpfont );
			fonttool_copy_kern( &tmpfont, &KernFont );
			break;

		case SDLK_F6:
			fonttool_remove_kerning( &KernFont );
			break;

		case SDLK_F10:
			fonttool_dump( fname1, &KernFont );
			done=1;
			break;

		case SDLK_COMMA:
			if ( alpha > 1 )	{
				alpha--;
				gr_init_alphacolor(&ac,cr*16,cg*16,cb*16,alpha*16,AC_TYPE_HUD);
			}
			break;

		case SDLK_PERIOD:
			if ( alpha < 17 )	{
				alpha++;
				gr_init_alphacolor(&ac,cr*16,cg*16,cb*16,alpha*16,AC_TYPE_HUD);
			}
			break;

		case SDLK_r:
			if ( cr == 16 ) cr = 1; else cr = 16;
			gr_init_alphacolor(&ac,cr*16,cg*16,cb*16,alpha*16,AC_TYPE_HUD);
			break;

		case SDLK_g:
			if ( cg == 16 ) cg = 1; else cg = 16;
			gr_init_alphacolor(&ac,cr*16,cg*16,cb*16,alpha*16,AC_TYPE_HUD);
			break;

		case SDLK_b:
			if ( cb == 16 ) cb = 1; else cb = 16;
			gr_init_alphacolor(&ac,cr*16,cg*16,cb*16,alpha*16,AC_TYPE_HUD);
			break;

		case SDLK_KP_6:
			x = fonttool_get_kerning( &KernFont, c1, c2, NULL );
			fonttool_set_kerning( &KernFont, c1, c2, x+1 );
			break;

		case SDLK_KP_4:
			x = fonttool_get_kerning( &KernFont, c1, c2, NULL );
			fonttool_set_kerning( &KernFont, c1, c2, x-1 );
			break;

		case SDLK_KP_5:
			fonttool_set_kerning( &KernFont, c1, c2, 0 );
			break;

		case SDLK_KP_7:
			if ( c1 < KernFont.first_ascii + KernFont.num_chars-1 ) c1++;
			break;

		case SDLK_KP_1:
			if ( c1 > KernFont.first_ascii ) c1--;
			break;

		case SDLK_KP_9:
			if ( c2 < KernFont.first_ascii + KernFont.num_chars-1 ) c2++;
			mprintf(( "C2 = %d\n", c2 ));
			break;

		case SDLK_KP_3:
			if ( c2 > KernFont.first_ascii ) c2--;
			mprintf(( "C2 = %d\n", c2 ));
			break;

		case SDLK_KP_2:
			if ( current_pair < 0 ) 
				current_pair = last_good_pair;
			else 
				current_pair++;
			if ( current_pair >= KernFont.num_kern_pairs )	{
				current_pair = KernFont.num_kern_pairs-1;
			}
			if ( (current_pair < KernFont.num_kern_pairs) && (current_pair > -1) )	{
				c1 = KernFont.kern_data[current_pair].c1 + KernFont.first_ascii;
				c2 = KernFont.kern_data[current_pair].c2 + KernFont.first_ascii;
			}
			break;

		case SDLK_KP_8:
			if ( current_pair < 0 ) 
				current_pair = last_good_pair;
			else
				current_pair--;
			if ( current_pair < 0 )	{
				current_pair = 0;
			}
			if ( (current_pair < KernFont.num_kern_pairs) && (current_pair > -1) )	{
				c1 = KernFont.kern_data[current_pair].c1 + KernFont.first_ascii;
				c2 = KernFont.kern_data[current_pair].c2 + KernFont.first_ascii;
			}
			break;

		case SDLK_ESCAPE:
			done=1;
			break;
		}

		if ( current_pair >= KernFont.num_kern_pairs )	{
			current_pair = -1;
		}

		int tmpp=-1;
		fonttool_get_kerning( &KernFont, c1, c2, &tmpp );

		if ( tmpp != -1 )	{
			current_pair = tmpp;
		} else	{
			if ( current_pair > -1 )
				last_good_pair = current_pair;
			current_pair = -1;
		}
	
		gr_reset_clip();
		gr_set_bitmap(bkg);
		gr_bitmap(0,0);
		gr_set_color_fast(&ac);

		snprintf( kerntext, sizeof(kerntext), "%c (%d)", c1, c1 );
		gr_string( 240, 210, kerntext );
		snprintf( kerntext, sizeof(kerntext), "%c (%d)", c2, c2 );
		gr_string( 340, 210, kerntext );

		snprintf( kerntext, sizeof(kerntext), "Ham%c%crger", c1, c2 );
		gr_string( 0x8000, 240, kerntext );

		snprintf( kerntext, sizeof(kerntext), "HAM%c%cRGER", c1, c2 );
		gr_string( 0x8000, 270, kerntext );

		snprintf( kerntext, sizeof(kerntext), "Offset: %d pixels", fonttool_get_kerning( &KernFont, c1, c2, NULL ) );
		gr_string( 0x8000, 300, kerntext );

		{
			int tw, th;
			gr_get_string_size( &tw, &th, kerntext );

			gr_string( 0x8000, 360, SampleText );
			gr_string( 20, 360+th+20, SampleText );
		}

		x = 5;
		int y = 200;
		int widest = 0;

		 //= ( 330 - 200 ) / KernFont.h;
		int num_items = KernFont.num_kern_pairs;

		if ( current_pair > -1 )
			current_item = current_pair;

		if (current_item <0 )
			current_item = 0;

		if (current_item >= num_items )
			current_item = num_items-1;

		if (current_item<first_item)
			first_item = current_item;

		if (current_item>=(first_item+num_items_displayed))
			first_item = current_item-num_items_displayed+1;

		if (num_items <= num_items_displayed )	
			first_item = 0;

		int stop = first_item+num_items_displayed;
		if (stop>num_items) stop = num_items;

		int n = 0;
		for (i=first_item; i<stop; i++ )	{
			int tw, th;

			snprintf( kerntext, sizeof(kerntext), "%c%c", KernFont.kern_data[i].c1 + KernFont.first_ascii, KernFont.kern_data[i].c2 + KernFont.first_ascii );
			if ( i==current_pair )	{
				gr_set_color( 255, 0, 0 );
				//hud_tri( i2fl(x),i2fl(y), i2fl(x+6), i2fl(y+5), i2fl(x), i2fl(y+8) );
				
				int x1 = x;
				int y1 = y;
				int x2 = x+6;
				int y2 = y+5;
				int x3 = x;
				int y3 = y+8;
	
				gr_line(x1,y1,x2,y2);
				gr_line(x2,y2,x3,y3);
				gr_line(x3,y3,x1,y1);

				gr_set_color_fast(&ac);
			}
			gr_string( x+8, y, kerntext );
			n++;
			gr_get_string_size( &tw, &th, kerntext );
			tw += 8;
			if ( tw > widest )
				widest = tw;
			y += KernFont.h;
			if ( y > 330 ) {
				y = 200;
				x += widest + 5;
				if ( x > 150 ) {
					num_items_displayed=n;
					break;
				}
				widest = 0;
			}
		}
		if (i==stop)
			num_items_displayed++;

		if (num_items_displayed < 1 )
			num_items_displayed = 1;

		if (num_items_displayed > num_items )
			num_items_displayed = num_items;

		//mprintf(( "Num items = %d\n", num_items_displayed ));


		gr_flip();

		// sleep a little bit, don't need high framerate here
		SDL_Delay(10);
	}

	// cleanup
	if (bkg > -1) {
		bm_unload(bkg);
	}


	exit(0);
}

