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
#include "cfilesystem.h"
#include "osregistry.h"
#include "osapi.h"
#include "bmpman.h"
#include "version.h"

#include <string>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


static SDL_Window *Window = nullptr;
static SDL_Renderer *Renderer = nullptr;
static SDL_Texture *Background = nullptr;
static ImGuiContext *Context = nullptr;
static LauncherScale *WindowScale = nullptr;

enum {
	FONT_SANS = 0,
	FONT_MONO,

	FONT_COUNT,
};

struct fonts {
	const char *filename;
	const float size;
	ImFont *ptr;
};

static fonts Fonts[] = {
	{ "DroidSans.ttf", 16.f, nullptr },
	{ "Cousine-Regular.ttf", 12.f, nullptr },
};


void LauncherScale::init(SDL_Window *win)
{
	window = win;

	if ( !window ) {
		return;
	}

	coord_scale = 1.0f;
	content_scale = 1.0f;
	scale_factor = 1.0f;

	context = nullptr;

	SDL_GetWindowSize(window, &unscaled_w, &unscaled_h);

	update();
}

void LauncherScale::update()
{
	if ( !window ) {
		return;
	}

	const float old_scale_factor = scale_factor;

	int window_w, window_h;
	int framebuffer_w, framebuffer_h;

	SDL_GetWindowSize(window, &window_w, &window_h);
	SDL_GetWindowSizeInPixels(window, &framebuffer_w, &framebuffer_h);

	float sx = framebuffer_w / static_cast<float>(window_w);
	float sy = framebuffer_h / static_cast<float>(window_h);

	coord_scale = std::max(sx, sy);
	content_scale = SDL_GetWindowDisplayScale(window);
	scale_factor = content_scale / coord_scale;

	bool resize = ((scale_factor != old_scale_factor) &&
				   ((scale_factor > 1.0f) || (old_scale_factor > 1.0f)));

	if (resize) {
		SDL_SetWindowSize(window, get(unscaled_w), get(unscaled_h));

		if (context) {
			auto oldCtx = ImGui::GetCurrentContext();

			ImGui::SetCurrentContext(context);
			ImGui::GetStyle() = styleOrig;
			ImGui::GetStyle().ScaleAllSizes(scale_factor);
			ImGui::GetStyle().FontScaleDpi = scale_factor;


			ImGui::SetCurrentContext(oldCtx);
		}
	}
}

void LauncherScale::setStyle(ImGuiContext *ctx) {
	context = ctx;

	if (context) {
		auto oldCtx = ImGui::GetCurrentContext();

		styleOrig = ImGui::GetStyle();
		ImGui::GetStyle().ScaleAllSizes(scale_factor);
		ImGui::GetStyle().FontScaleDpi = scale_factor;

		ImGui::SetCurrentContext(oldCtx);
	}
}


SDL_Renderer *launcher_get_renderer()
{
	return Renderer;
}

SDL_Window *launcher_get_window()
{
	return Window;
}

static void launcher_close()
{
	launcher_help_close();
	launcher_setup_close();

#ifdef MAKE_FS1
	launcher_close_fs1();
#else
	launcher_close_fs2();
#endif

	if (Context) {
		ImGui::SetCurrentContext(Context);
		ImGui_ImplSDLRenderer3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext(Context);
		Context = nullptr;
	}

	if (Background) {
		SDL_DestroyTexture(Background);
		Background = nullptr;
	}

	if (Renderer) {
		SDL_DestroyRenderer(Renderer);
		Renderer = nullptr;
	}

	if (Window) {
		SDL_DestroyWindow(Window);
		Window = nullptr;
	}

	if (WindowScale) {
		delete WindowScale;
		WindowScale = nullptr;
	}

	extern void cfile_close();
	cfile_close();

	SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD);
}

static bool launcher_init()
{
	if ( !SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD) ) {
		return false;
	}

	SDL_InitSubSystem(SDL_INIT_AUDIO);

	const std::string title = Osreg_title + std::string(" Launcher");

#ifdef MAKE_FS1
	const int window_width = 495;
	const int window_height = 463;
#else
	const int window_width = 375;
	const int window_height = 440;
#endif

	WindowScale = new (std::nothrow) LauncherScale;

	if ( !WindowScale ) {
		return false;
	}

	Uint32 window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	Window = SDL_CreateWindow(title.c_str(), window_width, window_height, window_flags);

	if ( !Window ) {
		return false;
	}

	WindowScale->init(Window);

	Renderer = SDL_CreateRenderer(Window, nullptr);

	if ( !Renderer ) {
		return false;
	}

	SDL_SetRenderVSync(Renderer, 1);

	os_set_icon(Window);

	SDL_SetWindowPosition(Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(Window);

	return true;
}

#ifdef SDL_PLATFORM_WINDOWS
#pragma warning( push )
#pragma warning( disable : 4702)	// unreachable code (for-loops)
#endif

void launcher_open_readme()
{
	std::string readme_path;
	bool readme_found = false;

	auto base = SDL_GetBasePath();

	if (base) {
		auto results = SDL_GlobDirectory(base, "readme.txt", SDL_GLOB_CASEINSENSITIVE, nullptr);

		if (results) {
			for (int i = 0; results[i]; ++i) {
				readme_found = true;
				readme_path = base;
				readme_path += results[i];
				break;
			}

			SDL_free(results);
		}
	}

	if ( !readme_found ) {
		auto extras = os_config_read_string(nullptr, "ExtrasPath", nullptr);

		if (extras) {
			auto results = SDL_GlobDirectory(extras, "readme.txt", SDL_GLOB_CASEINSENSITIVE, nullptr);

			if (results) {
				for (int i = 0; results[i]; ++i) {
					readme_found = true;
					readme_path = extras;
					if (readme_path.back() != DIR_SEPARATOR_CHAR) {
						readme_path.push_back(DIR_SEPARATOR_CHAR);
					}
					readme_path += results[i];
					break;
				}

				SDL_free(results);
			}
		}
	}

	if (readme_found) {
		std::string readme_uri = "file:///";
		readme_uri += readme_path;

		SDL_OpenURL(readme_uri.c_str());
	}
}

#ifdef SDL_PLATFORM_WINDOWS
#pragma warning( pop )
#endif

void launcher_init_background(const char *filename)
{
	if ( !filename ) {
		return;
	}

	auto surface = bm_image_to_surface(filename, CF_TYPE_INTERFACE);

	if (surface) {
		Background = SDL_CreateTextureFromSurface(launcher_get_renderer(), surface);

		SDL_DestroySurface(surface);
	}
}

static void launcher_enable_gamepad_nav_safe()
{
	// already enabled
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) {
		return;
	}

	// window doesn't have keyboard focus
	if (SDL_GetKeyboardFocus() != Window) {
		return;
	}

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
}

static void launcher_show_version()
{
	const int padding = 2;
	auto draw_list = ImGui::GetBackgroundDrawList();

	int window_w, window_h;
	SDL_GetWindowSize(Window, &window_w, &window_h);

	// NOTE: the font size *must* be specified here or else it will be wrong!!
	ImGui::PushFont(Fonts[FONT_MONO].ptr, Fonts[FONT_MONO].size);
	auto text_size = ImGui::CalcTextSize(version_get_string_full());

	// align version string to top right corner of window
	draw_list->AddText(ImVec2(window_w - text_size.x - padding, padding),
					   IM_COL32_WHITE, version_get_string_full());

	ImGui::PopFont();
}

// explicitly use VM_* versions here, for safety
static void *MallocWrapper(size_t size, void* user_data)
{
	IM_UNUSED(user_data);
	return VM_MALLOC(size);
}

static void FreeWrapper(void* ptr, void* user_data)
{
	IM_UNUSED(user_data);
	VM_FREE(ptr);
}

static bool launcher_do()
{
	bool rval = true;
	bool done = false;
	SDL_Event event;

	// setup imgui
	IMGUI_CHECKVERSION();
	Context = ImGui::CreateContext();
	ImGui::SetCurrentContext(Context);
	ImGui::SetAllocatorFunctions(MallocWrapper, FreeWrapper, nullptr);
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	io.Fonts->Clear();
	ImFontConfig fontConfig = {};

	fontConfig.RasterizerDensity = WindowScale->getCoordScale();

	for (int i = 0; i < FONT_COUNT; ++i) {
		auto file = cfopen(Fonts[i].filename, "rb", CF_TYPE_FONT);

		if (file) {
			auto size = cfilelength(file);
			auto font = cfread_file(file);

			cfclose(file);
			file = nullptr;

			if (font) {
				Fonts[i].ptr = io.Fonts->AddFontFromMemoryTTF(font, size,
															  Fonts[i].size,
															  &fontConfig);
			}
		}
	}

	ImGui_ImplSDL3_InitForSDLRenderer(Window, Renderer);
	ImGui_ImplSDLRenderer3_Init(Renderer);

#ifdef MAKE_FS1
	launcher_init_style_fs1();
#else
	launcher_init_style_fs2();
#endif

	WindowScale->setStyle(Context);

	while ( !done ) {
		ImGui::SetCurrentContext(Context);

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			launcher_setup_event(event);
			launcher_help_event(event);

			switch (event.type) {
				case SDL_EVENT_QUIT:
					done = true;
					rval = false;
					break;
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					if (event.window.windowID == SDL_GetWindowID(Window)) {
						done = true;
						rval = false;
					}
					break;
				case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
					if (event.window.windowID == SDL_GetWindowID(Window)) {
						WindowScale->update();
					}
					break;
				case SDL_EVENT_WINDOW_FOCUS_GAINED:
					// not safe to enable gamepad nav here!!
					break;
				case SDL_EVENT_WINDOW_FOCUS_LOST:
					ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
					break;
				default:
					break;
			}
		}

		if (SDL_GetWindowFlags(Window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		// gamepad nav hack (to avoid events from recently closed windows)
		launcher_enable_gamepad_nav_safe();

		// draw ui
#ifdef MAKE_FS1
		launcher_draw_fs1(&done, &rval, WindowScale);
#else
		launcher_draw_fs2(&done, &rval, WindowScale);
#endif

		launcher_show_version();

		// render
		ImGui::Render();
		SDL_SetRenderScale(Renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColorFloat(Renderer, 0.0f, 0.0f, 0.0f, 1.0f);
		SDL_RenderClear(Renderer);

		if (Background) {
			SDL_RenderTexture(Renderer, Background, nullptr, nullptr);
		}

		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), Renderer);
		SDL_RenderPresent(Renderer);

		if (launcher_setup_is_active()) {
			launcher_setup_draw();
		} else if (launcher_help_is_active()) {
			launcher_help_draw();
		}
	}

#ifndef MAKE_FS1
	// give FS2 a little extra time to play launcher sounds
	if (Launcher_sounds) {
		SDL_Delay(170);	// click sound is 160ms long
	}
#endif

	return rval;
}

// cmdline options that should skip the launcher ui
static const char *skip_options[] = {
	"-skip_launcher",
	"-standalone",
	"-version",
	"-v ",	// extra space!
	"-help",
	"-h ",	// extra space!
};

bool launcher_run(const char *szCmdline)
{
	bool rval = false;

	SDL_SetAppMetadata(Osreg_title, version_get_string_full(), Osreg_app_id);

	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
							   "Copyright (C) Volition, Inc. 1999.  All rights reserved.");

	if (cfile_init()) {
		return false;
	}

	// do some first-run stuff if needed
	if (os_config_read_uint(nullptr, "StraightToSetup", 1) == 1) {
		// set some sane config defaults
		os_init_registry_stuff();

		// unset first-run flag
		os_config_write_uint(nullptr, "StraightToSetup", 0);
	}

	// bypass launcher if the user doesn't want to see it
	// NOTE: cmdline options haven't been parsed yet, so we can't check that way
	if (szCmdline) {
		for (size_t i = 0; i < SDL_arraysize(skip_options); ++i) {
			if (SDL_strstr(szCmdline, skip_options[i])) {
				return true;
			}
		}
	}

	if ( !launcher_init() ) {
		launcher_close();
		return false;
	}

	rval = launcher_do();

	// shutdown and return to game (to play or exit)
	launcher_close();

	return rval;
}


