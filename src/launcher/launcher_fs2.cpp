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
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


struct fs2_buttons {
	const char *name;
	SDL_Texture *texture;
	float x;
	float y;
	float w;
	float h;
};

enum {
	BTN_PLAY = 0,
	BTN_PLAY_HL,
	BTN_PLAY_CK,
	BTN_SETUP,
	BTN_SETUP_HL,
	BTN_SETUP_CK,
	BTN_README,
	BTN_README_HL,
	BTN_README_CK,
	BTN_HELP,
	BTN_HELP_HL,
	BTN_HELP_CK,
	BTN_QUIT,
	BTN_QUIT_HL,
	BTN_QUIT_CK,
	BTN_VOLITION,
	BTN_VOLITION_HL,
	BTN_VOLITION_CK,
	BTN_PXO,
	BTN_PXO_HL,
	BTN_PXO_CK,
	//BTN_UPDATE,
	//BTN_UPDATE_HL,
	//BTN_UPDATE_CK,
	//BTN_UNINSTALL,
	//BTN_UNINSTALL_HL,
	//BTN_UNINSTALL_CK,

	BTN_COUNT,
};

static fs2_buttons Buttons[] = {
	{ "launch_fs2_play.png", nullptr, 45, 102, 131, 58, },
	{ "launch_fs2_play1.png", nullptr, 45, 102, 131, 58, },
	{ "launch_fs2_play2.png", nullptr, 45, 102, 131, 58, },
	{ "launch_fs2_setup.png", nullptr, 199, 102, 131, 58, },
	{ "launch_fs2_setup1.png", nullptr, 199, 102, 131, 58, },
	{ "launch_fs2_setup2.png", nullptr, 199, 102, 131, 58, },
	{ "launch_fs2_readme.png", nullptr, 45, 175, 131, 58, },
	{ "launch_fs2_readme1.png", nullptr, 45, 175, 131, 58, },
	{ "launch_fs2_readme2.png", nullptr, 45, 175, 131, 58, },
	{ "launch_fs2_help.png", nullptr, 45, 247, 131, 58, },
	{ "launch_fs2_help1.png", nullptr, 45, 247, 131, 58, },
	{ "launch_fs2_help2.png", nullptr, 45, 247, 131, 58, },
	{ "launch_fs2_quit.png", nullptr, 116, 339, 131, 58, },
	{ "launch_fs2_quit1.png", nullptr, 116, 339, 131, 58, },
	{ "launch_fs2_quit2.png", nullptr, 116, 339, 131, 58, },
	{ "launch_fs2_volition.png", nullptr, 15, 304, 90, 108, },
	{ "launch_fs2_volition1.png", nullptr, 15, 304, 90, 108, },
	{ "launch_fs2_volition2.png", nullptr, 15, 304, 90, 108, },
	{ "launch_fs2_pxo.png", nullptr, 249, 305, 114, 113, },
	{ "launch_fs2_pxo1.png", nullptr, 249, 305, 114, 113, },
	{ "launch_fs2_pxo2.png", nullptr, 249, 305, 114, 113, },
//	{ "launch_fs2_update.png", nullptr, 199, 175, 131, 58, },
//	{ "launch_fs2_update1.png", nullptr, 199, 175, 131, 58, },
//	{ "launch_fs2_update2.png", nullptr, 199, 175, 131, 58, },
//	{ "launch_fs2_uninstall.png", nullptr, 199, 247, 131, 58, },
//	{ "launch_fs2_uninstall1.png", nullptr, 199, 247, 131, 58, },
//	{ "launch_fs2_uninstall2.png", nullptr, 199, 247, 131, 58, },
};

SDL_COMPILE_TIME_ASSERT(Button_size, SDL_arraysize(Buttons) == BTN_COUNT);

static SDL_AudioStream *AudioStream = nullptr;

static uint8_t *SndHover = nullptr;
static uint32_t SndHover_size = 0;

static uint8_t *SndClick = nullptr;
static uint32_t SndClick_size = 0;

bool Launcher_sounds = true;


void launcher_close_fs2()
{
	for (int i = 0; i < static_cast<int>(SDL_arraysize(Buttons)); ++i) {
		if (Buttons[i].texture) {
			SDL_DestroyTexture(Buttons[i].texture);
			Buttons[i].texture = nullptr;
		}
	}

	if (AudioStream) {
		SDL_DestroyAudioStream(AudioStream);
		AudioStream = nullptr;
	}

	if (SndHover) {
		SDL_free(SndHover);
		SndHover = nullptr;
		SndHover_size = 0;
	}

	if (SndClick) {
		SDL_free(SndClick);
		SndClick = nullptr;
		SndClick_size = 0;
	}
}

void launcher_init_style_fs2()
{
	launcher_init_background("launch_fs2_bg.png");

	for (int i = 0; i < static_cast<int>(SDL_arraysize(Buttons)); ++i) {
		auto surface = bm_image_to_surface(Buttons[i].name, CF_TYPE_INTERFACE);

		if (surface) {
			Buttons[i].texture = SDL_CreateTextureFromSurface(launcher_get_renderer(), surface);
			SDL_DestroySurface(surface);
		}
	}

	SDL_AudioSpec spec = { SDL_AUDIO_U8, 1, 22050 };

	Launcher_sounds = (os_config_read_uint("Audio", "LauncherSoundEnabled", 1) == 1);

	AudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
											&spec, nullptr, nullptr);

	if (AudioStream) {
		SDL_ResumeAudioStreamDevice(AudioStream);

		SDL_LoadWAV_IO(cfopen_io("launch_fs2_hover.wav", "rb", CF_TYPE_SOUNDS),
					   true, &spec, &SndHover, &SndHover_size);

		SDL_LoadWAV_IO(cfopen_io("launch_fs2_click.wav", "rb", CF_TYPE_SOUNDS),
					   true, &spec, &SndClick, &SndClick_size);
	}

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowPadding = ImVec2(0, 0);
	style.WindowBorderSize = 0;
	style.FramePadding = ImVec2(0, 0);
	style.FrameBorderSize = 0;
	style.ImageBorderSize = 0;

	style.Colors[ImGuiCol_ButtonHovered] = ImColor(0, 0, 0, 0);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(0, 0, 0, 0);
	style.Colors[ImGuiCol_NavCursor] = ImColor(0, 0, 0, 0);
}

void launcher_draw_fs2(bool *done, bool *play_game, LauncherScale *WindowScale)
{
	int btn_id = 0;
	const fs2_buttons *btn;
	bool do_click = false;

	if ( !WindowScale ) {
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowBgAlpha(0);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove
								  | ImGuiWindowFlags_NoDecoration
								  | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::Begin("Buttons", nullptr, window_flags);

	btn_id = BTN_PLAY;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("play", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		if (launcher_ready_to_play(true)) {
			*done = true;
			*play_game = true;
		} else {
			launcher_data_missing_error();
		}
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

	btn_id = BTN_SETUP;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("setup", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		launcher_setup_open();
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

	btn_id = BTN_README;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("readme", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		launcher_open_readme();
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

#if 0
	btn_id = BTN_UPDATE;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("update", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		printf("update!\n");
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}
#endif

	btn_id = BTN_HELP;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("help", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		launcher_help_open();
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

#if 0
	btn_id = BTN_UNINSTALL;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("uninstall", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		printf("uninstall!\n");
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}
#endif

	btn_id = BTN_QUIT;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("quit", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		*done = true;
		*play_game = false;
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

	btn_id = BTN_VOLITION;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("volition", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		SDL_OpenURL("https://web.archive.org/web/20040331091342/http://volition-inc.com/");
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

	btn_id = BTN_PXO;
	btn = &Buttons[btn_id];

	ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
	if (ImGui::ImageButton("pxo", (ImTextureRef)(intptr_t)Buttons[btn_id].texture, WindowScale->get(btn->w, btn->h))) {
		SDL_OpenURL("https://pxo.nottheeye.com");
	}

	if (ImGui::IsItemActive()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+2].texture, WindowScale->get(btn->w, btn->h));
		do_click = true;
	} else if (ImGui::IsItemHovered()) {
		ImGui::SetCursorScreenPos(WindowScale->get(btn->x, btn->y));
		ImGui::Image((ImTextureRef)(intptr_t)Buttons[btn_id+1].texture, WindowScale->get(btn->w, btn->h));
	}

	// ------------------------------------------------

	if (Launcher_sounds) {
		static ImGuiID hover_id = 0;
		static ImGuiID active_id = 0;

		if (ImGui::IsAnyItemHovered() || ImGui::IsAnyItemFocused()) {
			static uint64_t timeout = 0;

			ImGuiID test_id = ImGui::IsAnyItemFocused() ? ImGui::GetFocusID() : ImGui::GetHoveredID();

			if (SndHover && (hover_id != test_id) && (timeout < SDL_GetTicks())) {
				hover_id = test_id;
				timeout = SDL_GetTicks() + 240;	// hover sound is 240ms long
				SDL_PutAudioStreamData(AudioStream, SndHover, SndHover_size);
			}
		} else {
			hover_id = 0;
		}

		if (do_click) {
			if (SndClick && (active_id != ImGui::GetActiveID())) {
				active_id = ImGui::GetActiveID();
				SDL_PutAudioStreamData(AudioStream, SndClick, SndClick_size);
			}
		} else {
			active_id = 0;
		}
	}

	ImGui::End();
}

