/*
 * $Logfile: /Freespace2/code/MenuUI/Barracks.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * C source file for implementing barracks
 *
 * $Log$
 * Revision 1.1  2002/05/03 03:28:12  root
 * Initial revision
 *
 * 
 * 3     2/02/99 11:58a Neilk
 * added vss revision/log comments
 *
 * $NoKeywords: $
 */

#ifndef _BARRACKS_H
#define _BARRACKS_H

// initialize the barracks 
void barracks_init();

// do a frame for the barrracks
void barracks_do_frame(float frametime);

// close the barracks
void barracks_close();

#endif // _BARRACKS_H

