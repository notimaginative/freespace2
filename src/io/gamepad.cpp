/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "joy.h"
#include "mouse.h"


static SDL_Gamepad *Gamepad = nullptr;


void gamepad_setup(SDL_JoystickID id)
{
	if ( !id ) {
		Gamepad = nullptr;
		return;
	}

	Gamepad = SDL_GetGamepadFromID(id);
}

bool gamepad_action()
{
	if ( !Gamepad ) {
		return false;
	}

	return (joy_down(0) == 1);
}

bool gamepad_cancel()
{
	if ( !Gamepad ) {
		return false;
	}

	return (joy_down(1) == 1);
}

bool gamepad_action_or_cancel()
{
	if ( !Gamepad ) {
		return false;
	}

	return (gamepad_action() || gamepad_cancel());
}

void gamepad_update_mouse_pos()
{
	if ( !Gamepad ) {
		return;
	}

	if ( !mouse_is_visible() ) {
		return;
	}

	// we poll directly here in order to get smooth movement
	int dx = SDL_GetGamepadAxis(Gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 10000;
	int dy = SDL_GetGamepadAxis(Gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 10000;

	if ( !dx && !dy ) {
		return;
	}

	// negative numbers move a little faster so we need to even things out for positives
	if (dx > 0) dx += 1;
	if (dy > 0) dy += 1;

	int x = 0;
	int y = 0;

	mouse_get_real_pos(&x, &y);

	x += dx;
	y += dy;

	mouse_update_pos_scaled(x, y, dx, dy);
}
