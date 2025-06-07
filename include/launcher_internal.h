/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef _LAUNCHER_INTERNAL_H
#define _LAUNCHER_INTERNAL_H

SDL_Renderer *launcher_get_renderder();
SDL_Window *launcher_get_window();

void launcher_open_readme();
void launcher_init_background(const char *filename);

bool launcher_help_is_active();
void launcher_help_open();
void launcher_help_close();
void launcher_help_draw();
void launcher_help_event(const SDL_Event &event);

bool launcher_setup_is_active();
void launcher_setup_open();
void launcher_setup_close();
void launcher_setup_draw();
void launcher_setup_event(const SDL_Event &event);

void launcher_init_style_fs1();
void launcher_draw_fs1(bool *done, bool *play_game);
void launcher_close_fs1();

void launcher_init_style_fs2();
void launcher_draw_fs2(bool *done, bool *play_game);
void launcher_close_fs2();

extern bool Launcher_sounds;

#endif	// !_LAUNCHER_INTERNAL_H
