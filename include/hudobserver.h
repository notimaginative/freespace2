/*
 * $Logfile: /Freespace2/code/Hud/HUDObserver.h $
 * $Revision$
 * $Date$
 * $Author$
 *
 * $NoKeywords: $
 *
 */

#ifndef _HUD_OBSERVER_FILE
#define _HUD_OBSERVER_FILE

#include "hud.h"

// prototypes
struct ship;
struct ai_info;

// use these to redirect Player_ship and Player_ai when switching into ai mode
extern ship Hud_obs_ship;
extern ai_info Hud_obs_ai;

// initialize observer hud stuff
void hud_observer_init(ship *shipp,ai_info *aip);

// render any specific observer stuff
void hud_render_observer();


#endif

