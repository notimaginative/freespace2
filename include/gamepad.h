/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__

void gamepad_setup(SDL_JoystickID id);
bool gamepad_action();
bool gamepad_cancel();
bool gamepad_action_or_cancel();
void gamepad_update_mouse_pos();

#endif	//__GAMEPAD_H__
