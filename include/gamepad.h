/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__

typedef enum GamepadTriggerButton {
	GAMEPAD_BUTTON_LEFT_TRIGGER = SDL_GAMEPAD_BUTTON_COUNT,
	GAMEPAD_BUTTON_RIGHT_TRIGGER
} GamepadTriggerButton;

void gamepad_setup(SDL_JoystickID id);
bool gamepad_action();
bool gamepad_cancel();
bool gamepad_action_or_cancel();
int gamepad_get_key();
int gamepad_get_button_axis(int btn);
void gamepad_update_mouse_pos();
void gamepad_mark_mouse_button(int button, bool down);

#endif	//__GAMEPAD_H__
