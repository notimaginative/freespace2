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
#include "key.h"
#include "timer.h"


static SDL_Gamepad *Gamepad = nullptr;
static bool Swap_action_cancel = false;


void gamepad_setup(SDL_JoystickID id)
{
	if ( !id || !SDL_IsGamepad(id) ) {
		Gamepad = nullptr;
		return;
	}

	// TODO: figure out how to set this properly (ini?, flag?, detect somehow?)
	// Swap_action_cancel = true;

	Gamepad = SDL_GetGamepadFromID(id);
}

bool gamepad_action()
{
	if ( !Gamepad ) {
		return false;
	}

	int button = int(Swap_action_cancel ? SDL_GAMEPAD_BUTTON_EAST : SDL_GAMEPAD_BUTTON_SOUTH);

	return joy_down(button) == 1;
}

bool gamepad_cancel()
{
	if ( !Gamepad ) {
		return false;
	}

	int button = int(Swap_action_cancel ? SDL_GAMEPAD_BUTTON_SOUTH : SDL_GAMEPAD_BUTTON_EAST);

	return joy_down(button) == 1;
}

bool gamepad_action_or_cancel()
{
	if ( !Gamepad ) {
		return false;
	}

	return (gamepad_action() || gamepad_cancel());
}

#define IS_AXIS_DOWN(x)	(((x >= 0) && (x < JOY_NUM_AXES) && (axes[int(x)] > 5000)) ? true : false)

int gamepad_get_key()
{
	static int key_check_time = 1;
	int axes[JOY_NUM_AXES] = { 0 };
	int k = 0;

	if ( !Gamepad || !mouse_is_visible() ) {
		return 0;
	}

	if ( !timestamp_elapsed(key_check_time) ) {
		return 0;
	}

	joystick_read_raw_axis(JOY_NUM_AXES, axes);

	if (gamepad_cancel()) {
		k = SDLK_ESCAPE;
	} else if (joy_down(JOY_HATBACK)) {
		k = SDLK_DOWN;
	} else if (joy_down(JOY_HATFORWARD)) {
		k = SDLK_UP;
	} else if (joy_down(JOY_HATLEFT)) {
		k = SDLK_LEFT;
	} else if (joy_down(JOY_HATRIGHT)) {
		k = SDLK_RIGHT;
	} else if (joy_down(int(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))) {
		k = KEY_SHIFTED | SDLK_TAB;
	} else if (joy_down(int(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))) {
		k = SDLK_TAB;
	} else if (IS_AXIS_DOWN(SDL_GAMEPAD_AXIS_LEFT_TRIGGER)) {
		k = SDLK_PAGEDOWN;
	} else if (IS_AXIS_DOWN(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) {
		k = SDLK_PAGEUP;
	}

	key_check_time = timestamp(150);

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
	int dx = SDL_GetGamepadAxis(Gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 8000;
	int dy = SDL_GetGamepadAxis(Gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 8000;

	if ( !dx && !dy ) {
		return;
	}

	// negative numbers move a little faster so we need to even things out for positives
	if (dx > 0) dx += 1;
	if (dy > 0) dy += 1;

	CAP(dx, -4, 4);
	CAP(dy, -4, 4);

	int x = 0;
	int y = 0;

	mouse_get_real_pos(&x, &y);

	// update deltas (x/y should be the same as current)
	mouse_update_pos_scaled(x, y, dx, dy);
	// now change position
	mouse_set_pos(x+dx, y+dy);
}

void gamepad_mark_mouse_button(int button, bool down)
{
	if ( !Gamepad ) {
		return;
	}

	if ( !mouse_is_visible() ) {
		return;
	}

	const int left_button = Swap_action_cancel ? SDL_GAMEPAD_BUTTON_EAST : SDL_GAMEPAD_BUTTON_SOUTH;
	uint m_button = 0;

	if (button == left_button) {
		// "A" or "B" (if swapped)
		m_button = SDL_BUTTON_LEFT;
	} else if (button == SDL_GAMEPAD_BUTTON_WEST) {
		// "X"
		m_button = SDL_BUTTON_RIGHT;
	}

	if ( !m_button ) {
		return;
	}

	mouse_mark_button(m_button, down);
}
