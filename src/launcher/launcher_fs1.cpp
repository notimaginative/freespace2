/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "launcher.h"
#include "launcher_internal.h"
#include "cfile.h"
#include "osregistry.h"
#include "osapi.h"
#include "bmpman.h"

#include <string>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


void launcher_close_fs1()
{
	// nothing special to do here
}

void launcher_init_style_fs1()
{
	launcher_init_background("launch_fs1_bg.png");

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowPadding = ImVec2(0, 0);
	style.WindowBorderSize = 0;
	style.ItemSpacing = ImVec2(10, 10);
	style.ButtonTextAlign = ImVec2(.5f, .5f);
	style.FramePadding = ImVec2(10, 5);
	style.FrameBorderSize = 1;

	style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
	style.Colors[ImGuiCol_Button] = ImColor(49, 58, 65);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(90, 140, 100);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(45, 70, 140);
}

void launcher_draw_fs1(bool *done, bool *play_game, LauncherScale *WindowScale)
{
	if ( !WindowScale ) {
		return;
	}

	ImGui::SetNextWindowPos(WindowScale->get(295, 200));
	ImGui::SetNextWindowBgAlpha(0);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove
								  | ImGuiWindowFlags_NoDecoration
								  | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::Begin("Buttons", nullptr, window_flags);

	auto buttonSize = WindowScale->get(175.0f, 0.f);

	if (ImGui::Button("Play FreeSpace", buttonSize)) {
		*done = true;
		*play_game = true;
	}

	if (ImGui::Button("Setup", buttonSize)) {
		launcher_setup_open();
	}

	if (ImGui::Button("View README", buttonSize)) {
		launcher_open_readme();
	}

	if (ImGui::Button("PXO", buttonSize)) {
		SDL_OpenURL("https://pxo.nottheeye.com");
	}

	if (ImGui::Button("FreeSpace2 Project", buttonSize)) {
		SDL_OpenURL("https://github.com/notimaginative/freespace2");
	}

	if (ImGui::Button("Quit", buttonSize)) {
		*done = true;
		*play_game = false;
	}

	ImGui::End();
}
