/*
 * $Logfile: /Freespace2/code/Graphics/GrGlide.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Include file for a 3DFX's Glide graphics lib
 *
 * $Log$
 * Revision 1.1  2002/05/03 03:28:12  root
 * Initial revision
 *
 * 
 * 3     6/29/99 10:35a Dave
 * Interface polygon bitmaps! Whee!
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 1     5/12/97 12:13p John
 *
 * $NoKeywords: $
 */

#ifndef _GRGLIDE_H
#define _GRGLIDE_H

void gr_glide_init();
void gr_glide_cleanup();

void gr_glide_bitmap(int x, int y);
void gr_glide_bitmap_ex(int x, int y, int w, int h, int sx, int sy);

#endif

