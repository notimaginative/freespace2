/*
 * $Logfile: /Freespace2/code/Hud/HUDreticle.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Header file for functions to draw and manage the reticle
 *
 * $Log$
 * Revision 1.1  2002/05/03 03:28:12  root
 * Initial revision
 *
 * 
 * 3     12/21/98 5:02p Dave
 * Modified all hud elements to be multi-resolution friendly.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 5     4/08/98 10:34p Allender
 * make threat indicators work in multiplayer.  Fix socket problem (once
 * and for all???)
 * 
 * 4     11/04/97 7:49p Lawrance
 * integrating new HUD reticle and shield icons
 * 
 * 3     12/08/96 1:54a Lawrance
 * integrating hud configuration
 * 
 * 2     10/28/96 4:54p Lawrance
 * moving #defines to header file since other files need to reference them
 * 
 * 1     10/24/96 11:50a Lawrance
 *
 * $NoKeywords: $
 *
*/

#ifndef _HUDRETICLE_H
#define _HUDRETICLE_H

#include "player.h"

extern int Outer_circle_radius[GR_NUM_RESOLUTIONS];
extern int Hud_reticle_center[GR_NUM_RESOLUTIONS][2];

void hud_init_reticle();
void hud_update_reticle( player *pp );
void hud_show_reticle();

void hud_draw_outer_reticle();
void hud_draw_center_reticle();
void hud_draw_throttle_gauge();
void hud_draw_target_throttle_gauge();



#endif

