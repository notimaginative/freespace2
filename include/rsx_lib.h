/*
 * $Logfile: /Freespace2/code/Sound/rsx_lib.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Header file for RSX sound lib
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
 * 2     10/14/97 11:33p Lawrance
 * get RSX implemented
 * 
 * 1     10/14/97 12:58p Lawrance
 * 
 * 1     10/14/97 10:35a Lawrance
 *
 * $NoKeywords: $
 */


#ifndef __FREESPACE_RSX_H__
#define __FREESPACE_RSX_H__

#include "pstypes.h"

extern int rsx_initialized;

int	rsx_init();
void	rsx_update_listener(vector *pos, matrix *orient);
void	rsx_close();
int	rsx_create_cached_emitter(char *filename, int is_3d, int use_doppler, int min, int max, float max_volume);
int	rsx_play( int sid, float priority, float volume);
int	rsx_play_3d( int sid, float priority, float volume, vector *pos, vector *sound_fvec);
void	rsx_unload_buffer(int sid);


#endif

