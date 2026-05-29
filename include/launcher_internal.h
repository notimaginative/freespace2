/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef _LAUNCHER_INTERNAL_H
#define _LAUNCHER_INTERNAL_H

#include <imgui.h>

class LauncherScale {
private:
	SDL_Window *window;
	int unscaled_w;
	int unscaled_h;
	float content_scale;
	float coord_scale;
	float scale_factor;

	ImGuiContext *context;
	ImGuiStyle styleOrig;

public:
	void init(SDL_Window *win);
	void update();

	void setStyle(ImGuiContext *ctx);

	float getCoordScale() { return coord_scale; }

	ImVec2 get(float x, float y) { return ImVec2(x * scale_factor, y * scale_factor); }
	float get(float x) { return (x * scale_factor); }
	int get(int x) { return static_cast<int>(x * scale_factor); }
};

enum class LauncherSetupTab {
	unset,
	Video,
	Audio,
	Controls,
	Speed,
	Network,
	PXO,
	Misc
};

SDL_Renderer *launcher_get_renderer();
SDL_Window *launcher_get_window();

void launcher_open_readme();
void launcher_init_background(const char *filename);
bool launcher_ready_to_play();
void launcher_data_missing_error();

bool launcher_help_is_active();
void launcher_help_open();
void launcher_help_close();
void launcher_help_draw();
void launcher_help_event(const SDL_Event &event);

bool launcher_setup_is_active();
void launcher_setup_open(const LauncherSetupTab initial_tab = LauncherSetupTab::unset, bool tab_locked = false);
void launcher_setup_close();
void launcher_setup_draw();
void launcher_setup_event(const SDL_Event &event);

void launcher_init_style_fs1();
void launcher_draw_fs1(bool *done, bool *play_game, LauncherScale *WindowScale);
void launcher_close_fs1();

void launcher_init_style_fs2();
void launcher_draw_fs2(bool *done, bool *play_game, LauncherScale *WindowScale);
void launcher_close_fs2();

extern bool Launcher_sounds;

#endif	// !_LAUNCHER_INTERNAL_H
