/*
 * $Logfile: /Freespace2/code/Starfield/Nebula.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Include file for nebula stuff
 *
 * $Log$
 * Revision 1.1  2002/05/03 03:28:12  root
 * Initial revision
 *
 * 
 * 2     10/07/98 10:54a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 4     11/25/97 11:40a Hoffoss
 * Added support for nebula placement editing.
 * 
 * 3     11/24/97 12:04p John
 * 
 * 2     11/16/97 2:29p John
 * added versioning to nebulas; put nebula code into freespace.
 * 
 * 1     11/16/97 1:14p John
 *
 * $NoKeywords: $
 */

#ifndef _NEBULA_H
#define _NEBULA_H

// mainly only needed by Fred
extern int Nebula_pitch;
extern int Nebula_bank;
extern int Nebula_heading;

// You shouldn't pass the extension for filename.
// PBH = Pitch, Bank, Heading.   Pass NULL for default orientation.
void nebula_init( char *filename, int pitch, int bank, int heading );
void nebula_init( char *filename, angles *pbh = NULL );
void nebula_close();
void nebula_render();

#endif	//_NEBULA_H

