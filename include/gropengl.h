/*
 * $Logfile: /Freespace2/code/Graphics/GrOpenGL.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Include file for OpenGL renderer
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
 * 1     5/12/97 12:14p John
 *
 * $NoKeywords: $
 */

#ifndef _GROPENGL_H
#define _GROPENGL_H

void gr_opengl_init();
void gr_opengl_cleanup();

void gr_opengl_bitmap(int x, int y);
void gr_opengl_bitmap_ex(int x, int y, int w, int h, int sx, int sy);

#endif

