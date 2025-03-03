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
#include "gamepad.h"


static SDL_Gamepad *Gamepad = nullptr;
static bool Swap_action_cancel = false;


void gamepad_setup(SDL_JoystickID id)
{
	if ( !id ) {
		Gamepad = nullptr;
		return;
	}

	// TODO: figure out how to set this properly (ini?, flag?, detect somehow?)
	// Swap_action_cancel = true;

	Gamepad = SDL_GetGamepadFromID(id);
}

bool gamepad_action(bool reset)
{
	if ( !Gamepad ) {
		return false;
	}

	bool down = joy_down(Swap_action_cancel ? 1 : 0) == 1;

	// Because of how this ties in with the mouse, we need to be able to keep
	// the button down for multiple frames. So don't reset when called from
	// mouse_down().
	if (down && reset) {
		// mark button as down so we aren't just spamming it for a bunch of frames
		joy_mark_button(Swap_action_cancel ? 1 : 0, 0);
	}

	return down;
}

bool gamepad_cancel()
{
	if ( !Gamepad ) {
		return false;
	}

	bool down = joy_down(Swap_action_cancel ? 0 : 1) == 1;

	if (down) {
		// mark button as down so we aren't just spamming it for a bunch of frames
		joy_mark_button(Swap_action_cancel ? 0 : 1, 0);
	}

	return down;
}

bool gamepad_action_or_cancel()
{
	if ( !Gamepad ) {
		return false;
	}

	return (gamepad_action() || gamepad_cancel());
}

int gamepad_get_dpad_key()
{
	int k = 0;

	if (joy_down(JOY_HATBACK)) {
		// mark button as down so we aren't just spamming it for a bunch of frames
		joy_mark_button(JOY_HATBACK, 0);
		k = SDLK_DOWN;
	} else if (joy_down(JOY_HATFORWARD)) {
		joy_mark_button(JOY_HATFORWARD, 0);
		k = SDLK_UP;
	} else if (joy_down(JOY_HATLEFT)) {
		joy_mark_button(JOY_HATLEFT, 0);
		k = SDLK_LEFT;
	} else if (joy_down(JOY_HATRIGHT)) {
		joy_mark_button(JOY_HATRIGHT, 0);
		k = SDLK_RIGHT;
	}

	return k;
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

	// update deltas (x/y should be the same as current)
	mouse_update_pos_scaled(x, y, dx, dy);
	// now change position
	mouse_set_pos(x+dx, y+dy);
}
