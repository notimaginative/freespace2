/*
 * $Logfile: /Freespace2/code/GameHelp/GameplayHelp.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Header for displaying in-game help
 *
 * $Log$
 * Revision 1.1  2002/05/03 03:28:12  root
 * Initial revision
 *
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:48a Dave
 * 
 * 2     3/09/98 9:54p Lawrance
 * integrate new art for gameplay help
 * 
 * 1     3/09/98 5:05p Lawrance
 *
 * $NoKeywords: $
 */

#ifndef __GAMEPLAY_HELP_H__
#define __GAMEPLAY_HELP_H__

void gameplay_help_init();
void gameplay_help_close();
void gameplay_help_do_frame(float frametime);

#endif

