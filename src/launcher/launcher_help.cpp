/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "launcher_internal.h"
#include "cfile.h"
#include "bmpman.h"
#include "osapi.h"

#include <string>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


static SDL_Window *Window = nullptr;
static SDL_Renderer *Renderer = nullptr;
static ImGuiContext *Context = nullptr;
static LauncherScale *WindowScale = nullptr;

static char *HelpText = nullptr;


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
	{ "Cousine-Regular.ttf", 14.f, nullptr },
};

SDL_COMPILE_TIME_ASSERT(Fonts_size, SDL_arraysize(Fonts) == FONT_COUNT);


static void close_window()
{
	SDL_Event event;
	SDL_zero(event);
	event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
	event.window.windowID = SDL_GetWindowID(Window);
	SDL_PushEvent(&event);
}

bool launcher_help_is_active()
{
	return (Window != nullptr && Context != nullptr);
}

void launcher_help_event(const SDL_Event &event)
{
	if ( !Window ) {
		return;
	}

	auto savedContext = ImGui::GetCurrentContext();

	ImGui::SetCurrentContext(Context);

	ImGui_ImplSDL3_ProcessEvent(&event);

	switch (event.type) {
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			if (event.window.windowID == SDL_GetWindowID(Window)) {
				launcher_help_close();
			}
			break;
		case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
			if (event.window.windowID == SDL_GetWindowID(Window)) {
				WindowScale->update();
			}
			break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
			break;
		default:
			break;
	}

	ImGui::SetCurrentContext(savedContext);
}

void launcher_help_open()
{
	Uint32 window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;

	WindowScale = new (std::nothrow) LauncherScale;

	if ( !WindowScale ) {
		launcher_help_close();
		return;
	}

	Window = SDL_CreateWindow("Launcher Help", 620, 420, window_flags);

	if ( !Window ) {
		launcher_help_close();
		return;
	}

	WindowScale->init(Window);

	SDL_SetWindowParent(Window, launcher_get_window());
	SDL_SetWindowModal(Window, true);

	Renderer = SDL_CreateRenderer(Window, nullptr);

	if ( !Renderer ) {
		launcher_help_close();
		return;
	}

	SDL_SetRenderVSync(Renderer, 1);

	SDL_SetWindowPosition(Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(Window);

	HelpText = reinterpret_cast<char*>(cf_load_file("launch_help.txt", "rt", CF_TYPE_TEXT));

	auto savedContext = ImGui::GetCurrentContext();

	Context = ImGui::CreateContext();
	ImGui::SetCurrentContext(Context);
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
															  WindowScale->get(Fonts[i].size),
															  &fontConfig);
			}
		}
	}

	ImGui_ImplSDL3_InitForSDLRenderer(Window, Renderer);
	ImGui_ImplSDLRenderer3_Init(Renderer);

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowPadding = ImVec2(15, 15);
	style.WindowBorderSize = 0;
	style.ItemSpacing = ImVec2(10, 20);
	style.ButtonTextAlign = ImVec2(.5f, .5f);
	style.FramePadding = ImVec2(10, 5);
	style.FrameBorderSize = 1;
//	style.sp = ImVec2(20, 20);
	style.TabRounding = 1;

	style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
	style.Colors[ImGuiCol_Button] = ImColor(49, 58, 65);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(90, 140, 100);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(45, 70, 140);
	style.Colors[ImGuiCol_WindowBg] = ImColor(25, 25, 25);

	WindowScale->setStyle(Context);

	ImGui::SetCurrentContext(savedContext);
}

void launcher_help_close()
{
	if (Context) {
		auto savedContext = ImGui::GetCurrentContext();

		ImGui::SetCurrentContext(Context);
		ImGui_ImplSDLRenderer3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext(Context);
		Context = nullptr;

		ImGui::SetCurrentContext(savedContext);
	}

	if (HelpText) {
		free(HelpText);
		HelpText = nullptr;
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
}

static void AlignForWidth(float width, float alignment = 0.5f)
{
	float avail = ImGui::GetContentRegionAvail().x;
	float off = (avail - width) * alignment;
	if (off > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void launcher_help_draw()
{
	int w, h;
	auto savedContext = ImGui::GetCurrentContext();

	ImGui::SetCurrentContext(Context);

	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	// draw
	SDL_GetWindowSize(Window, &w, &h);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(i2fl(w), i2fl(h)));
//	ImGui::SetNextWindowBgAlpha(0);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove
								  | ImGuiWindowFlags_NoDecoration;

	ImGui::Begin("Help", nullptr, window_flags);

	ImGui::PushFont(Fonts[FONT_MONO].ptr, 0.0f);
	ImGui::SetNextWindowSize(ImVec2(0.f, i2fl(h) - WindowScale->get(75.f)));
	ImGui::BeginChild("text");
	ImGui::TextUnformatted(HelpText ? HelpText : "No help available.");
	ImGui::EndChild();
	ImGui::PopFont();

	// center button group
	const float button_width = WindowScale->get(100.f);
	AlignForWidth(button_width);

	if (ImGui::Button("Close", ImVec2(button_width, 0))) {
		close_window();
	}

	ImGui::End();

	// render
	ImGui::Render();
	SDL_SetRenderScale(Renderer, ImGui::GetIO().DisplayFramebufferScale.x,
					   ImGui::GetIO().DisplayFramebufferScale.y);
	SDL_SetRenderDrawColorFloat(Renderer, 0.0f, 0.0f, 0.0f, 1.0f);
	SDL_RenderClear(Renderer);

	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), Renderer);
	SDL_RenderPresent(Renderer);

	ImGui::SetCurrentContext(savedContext);
}
