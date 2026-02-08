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
#include "cfilesystem.h"
#include "bmpman.h"
#include "osapi.h"
#include "osregistry.h"
#include "systemvars.h"

#include <string>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


static SDL_Window *Window = nullptr;
static SDL_Renderer *Renderer = nullptr;
static ImGuiContext *Context = nullptr;
static LauncherScale *WindowScale = nullptr;

static void launcher_setup_load_config();

static const ImVec2 SpecialPadding(3, 3);

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
	{ "DroidSans.ttf", 15.f, nullptr },
	{ "Cousine-Regular.ttf", 16.f, nullptr },
};

SDL_COMPILE_TIME_ASSERT(Fonts_size, SDL_arraysize(Fonts) == FONT_COUNT);

struct config {
	// video
	int renderer;
	int msaa;
	bool fullscreen;
	bool show_fps;
	// audio
	std::string playback_device;
	std::string capture_device;
	bool efx;
	bool launcher_sounds;
	// controls
	std::string controller;
	bool haptic;
	bool direct_force;
	bool swap_action_cancel;
	// speed
	int detail_level;
	// network
	int network_connection;
	int network_speed;
	int port;
	// pxo
	std::string pxo_login;
	std::string pxo_pass;
	bool pxo_skip_version_check;
	bool pxo_banners;
	// misc
	std::string extras_path;
	std::string cmdline;
	bool nosound;
	bool nomusic;
	bool nomovies;
	bool nodpiscaling;
	bool nobriefanim;

	config(): msaa(0), fullscreen(true), show_fps(false), efx(false), launcher_sounds(true),
			  haptic(false), direct_force(true), swap_action_cancel(false),
			  detail_level(2), network_connection(2), network_speed(5), port(0),
			  pxo_skip_version_check(true), pxo_banners(true), nosound(false),
			  nomusic(false), nomovies(false), nodpiscaling(false), nobriefanim(false)
			  {}
};

static config Config;

static const int MSAA[] = {
	0, 2, 4, 8, 16
};

static const char *VideoRenderers[] = {
	nullptr, "GLES2", "OpenGL",
};

static const char *NetworkConnections[] = {
	nullptr, "dialup", "lan"
};

static const char *NetworkSpeeds[] = {
	nullptr, "Slow", "56K", "ISDN", "Cable", "Fast"
};


static void close_window()
{
	SDL_Event event;
	SDL_zero(event);
	event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
	event.window.windowID = SDL_GetWindowID(Window);
	SDL_PushEvent(&event);
}

bool launcher_setup_is_active()
{
	return (Window != nullptr && Context != nullptr);
}

void launcher_setup_event(const SDL_Event &event)
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
				launcher_setup_close();
			}
			break;
		case SDL_EVENT_JOYSTICK_ADDED:
		case SDL_EVENT_JOYSTICK_REMOVED:
			// TODO: enumerate joysticks
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

void launcher_setup_open()
{
	Uint32 window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	WindowScale = new (std::nothrow) LauncherScale;

	if ( !WindowScale ) {
		launcher_setup_close();
		return;
	}

	Window = SDL_CreateWindow("FreeSpace Setup", 400, 460, window_flags);

	if ( !Window ) {
		launcher_setup_close();
		return;
	}

	WindowScale->init(Window);

	SDL_SetWindowParent(Window, launcher_get_window());
	SDL_SetWindowModal(Window, true);

	Renderer = SDL_CreateRenderer(Window, nullptr);

	if ( !Renderer ) {
		launcher_setup_close();
		return;
	}

	SDL_SetRenderVSync(Renderer, 1);

	SDL_SetWindowPosition(Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(Window);

	launcher_setup_load_config();

	auto savedContext = ImGui::GetCurrentContext();

	Context = ImGui::CreateContext();
	ImGui::SetCurrentContext(Context);
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// FIXME: gamepad events trigger on all windows, breaking setup/help
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

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

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowPadding = ImVec2(10, 10);
	style.WindowBorderSize = 0;
	style.ItemSpacing = ImVec2(10, 10);
	style.ButtonTextAlign = ImVec2(.5f, .5f);
	style.FramePadding = ImVec2(5, 6);
	style.FrameBorderSize = 1;
	style.TabRounding = 1;
	style.TabBarOverlineSize = 2;

	style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
	style.Colors[ImGuiCol_Button] = ImColor(49, 58, 65);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(90, 140, 100);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(45, 70, 140);
	style.Colors[ImGuiCol_WindowBg] = ImColor(25, 25, 25);
	style.Colors[ImGuiCol_CheckMark] = ImColor(255, 255, 255);
	style.Colors[ImGuiCol_Tab] = ImColor(49, 58, 65);
	style.Colors[ImGuiCol_TabSelected] = ImColor(45, 70, 140);
	style.Colors[ImGuiCol_TabHovered] = ImColor(90, 140, 100);

	WindowScale->setStyle(Context);

	ImGui::SetCurrentContext(savedContext);
}

void launcher_setup_close()
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

static std::string trim(const std::string& str)
{
	if (str.empty()) {
		return str;
	}

	auto start = str.begin();
	while (start != str.end() && std::isspace(*start)) ++start;

	auto end = str.end();
	do { --end; } while (end != start && std::isspace(*end));

	return std::string(start, end + 1);
}

static void launcher_setup_load_config()
{
	const char *ptr;
	int i_val;

	// Video
	ptr = os_config_read_string("Video", "Renderer", nullptr);
	Config.renderer = 0;

	if (ptr) {
		for (int i = 0; i < static_cast<int>(SDL_arraysize(VideoRenderers)); ++i) {
			if ( !VideoRenderers[i] ) {
				continue;
			}

			if ( !SDL_strcasecmp(VideoRenderers[i], ptr) ) {
				Config.renderer = i;
				break;
			}
		}
	}

	Config.fullscreen = (os_config_read_uint("Video", "Fullscreen", 1) == 1);
	Config.show_fps = (os_config_read_uint("Video", "ShowFPS", 0) == 1);
	i_val = os_config_read_uint("Video", "AntiAlias", 0);

	for (int i = 0; i < static_cast<int>(SDL_arraysize(MSAA)); ++i) {
		if (i_val == MSAA[i]) {
			Config.msaa = i;
			break;
		}
	}

	// Audio
//	ptr = os_config_read_string("Audio", "PlaybackDevice", nullptr);
//	if (ptr) Config.playback_device = ptr;

//	ptr = os_config_read_string("Audio", "CaptureDevice", nullptr);
//	if (ptr) Config.capture_device = ptr;

	Config.efx = (os_config_read_uint("Audio", "EFX", 0) == 1);
	Config.launcher_sounds = (os_config_read_uint("Audio", "LauncherSoundEnabled", 1) == 1);

	// Controls
	ptr = os_config_read_string("Controls", "CurrentJoystick", nullptr);
	if (ptr) Config.controller = ptr;

	Config.haptic = (os_config_read_uint("Controls", "EnableJoystickFF", 0) == 1);
	Config.direct_force = (os_config_read_uint("Controls", "EnableHitEffect", 0) == 1);
	Config.swap_action_cancel = (os_config_read_uint("Controls", "SwapActionCancel", 0) == 1);

	// Speed
	Config.detail_level = os_config_read_uint(nullptr, "ComputerSpeed", 2);
	Config.detail_level = SDL_clamp(Config.detail_level, 0, NUM_DEFAULT_DETAIL_LEVELS-1);

	// Network
	ptr = os_config_read_string("Network", "NetworkConnection", nullptr);
	Config.network_connection = 0;	// start with none

	if (ptr) {
		for (int i = 0; i < static_cast<int>(SDL_arraysize(NetworkConnections)); ++i) {
			if ( !NetworkConnections[i] ) {
				continue;
			}

			if ( !SDL_strcasecmp(ptr, NetworkConnections[i]) ) {
				Config.network_connection = i;
				break;
			}
		}
	}

	ptr = os_config_read_string("Network", "ConnectionSpeed", nullptr);
	Config.network_speed = 0;	// start with none

	if (ptr) {
		for (int i = 0; i < static_cast<int>(SDL_arraysize(NetworkSpeeds)); ++i) {
			if ( !NetworkSpeeds[i] ) {
				continue;
			}

			if ( !SDL_strcasecmp(ptr, NetworkSpeeds[i]) ) {
				Config.network_speed = i;
				break;
			}
		}
	}

	Config.port = os_config_read_uint("Network", "ForcePort", 0);
	Config.port = SDL_clamp(Config.port, 0, SDL_MAX_UINT16);

	// PXO
	ptr = os_config_read_string("PXO", "Login", nullptr);
	if (ptr) Config.pxo_login = ptr;

	ptr = os_config_read_string("PXO", "Password", nullptr);
	if (ptr) Config.pxo_pass = ptr;

	Config.pxo_skip_version_check = (os_config_read_uint("PXO", "SkipVerify", 0) == 1);
	Config.pxo_banners = (os_config_read_uint("PXO", "PXOBanners", 1) == 1);

	// Misc
	ptr = os_config_read_string(nullptr, "ExtrasPath", nullptr);
	if (ptr) Config.extras_path = ptr;

	if ( !Config.extras_path.empty() && Config.extras_path.back() == DIR_SEPARATOR_CHAR ) {
		Config.extras_path.pop_back();
	}

	char cmdline_cfg[MAX_PATH_LEN];
	cf_create_default_path_string(cmdline_cfg, CF_TYPE_DATA, "cmdline.cfg");

	char *cfg = reinterpret_cast<char *>(SDL_LoadFile(cmdline_cfg, nullptr));
	if (cfg) {
		Config.cmdline = cfg;
		SDL_free(cfg);
	}

	// search for quick options in cmdline and remove entries if found
	// we'll add them back later as needed when saving
	// (single character options need trailing space!!)
	const char *nosound_opts[] = { "--nosound", "-nosound", "-s " };
	const char *nomusic_opts[] = { "--nomusic", "-nomusic" };
	const char *nomovies_opts[] = { "--nomovies", "-nomovies", "-n " };
	const char *nodpiscale_opts[] = { "--no_dpi_scaling", "-no_dpi_scaling" };

	if ( !Config.cmdline.empty() ) {
		// add trailing space for easier option parsing
		Config.cmdline += " ";

		// no sound
		for (size_t i = 0; i < SDL_arraysize(nosound_opts); ++i) {
			auto pos = Config.cmdline.find(nosound_opts[i]);

			if (pos != std::string::npos) {
				Config.nosound = true;
				// strip option from cmdline, including extra space
				Config.cmdline.erase(pos, SDL_strlen(nosound_opts[i]) + 1);
			}
		}

		// no music
		for (size_t i = 0; i < SDL_arraysize(nomusic_opts); ++i) {
			auto pos = Config.cmdline.find(nomusic_opts[i]);

			if (pos != std::string::npos) {
				Config.nomusic = true;
				// strip option from cmdline, including extra space
				Config.cmdline.erase(pos, SDL_strlen(nomusic_opts[i]) + 1);
			}
		}

		// no movies
		for (size_t i = 0; i < SDL_arraysize(nomovies_opts); ++i) {
			auto pos = Config.cmdline.find(nomovies_opts[i]);

			if (pos != std::string::npos) {
				Config.nomovies = true;
				// strip option from cmdline, including extra space
				Config.cmdline.erase(pos, SDL_strlen(nomovies_opts[i]) + 1);
			}
		}

		// no dpi scaling
		for (size_t i = 0; i < SDL_arraysize(nodpiscale_opts); ++i) {
			auto pos = Config.cmdline.find(nodpiscale_opts[i]);

			if (pos != std::string::npos) {
				Config.nodpiscaling = true;
				// strip option from cmdline, including extra space
				Config.cmdline.erase(pos, SDL_strlen(nodpiscale_opts[i]) + 1);
			}
		}

		// clean whitespace
		auto temp = trim(Config.cmdline);
		Config.cmdline = temp;
	}

	Config.nobriefanim = (os_config_read_uint("Video", "BriefingAnimation", 1) == 0);
}

static void launcher_setup_save_config()
{
	// Video
	os_config_write_string("Video", "Renderer", VideoRenderers[Config.renderer]);
	os_config_write_uint("Video", "Fullscreen", Config.fullscreen ? 1 : 0);
	os_config_write_uint("Video", "ShowFPS", Config.show_fps ? 1 : 0);
	os_config_write_uint("Video", "AntiAlias", MSAA[Config.msaa]);

	// Audio
	os_config_write_uint("Audio", "EFX", Config.efx ? 1 : 0);
	os_config_write_uint("Audio", "LauncherSoundEnabled", Config.launcher_sounds ? 1 : 0);
	Launcher_sounds = Config.launcher_sounds;

	// Controls
	os_config_write_string("Controls", "CurrentJoystick", Config.controller.c_str());
	os_config_write_uint("Controls", "EnableJoystickFF", Config.haptic ? 1 : 0);
	os_config_write_uint("Controls", "EnableHitEffect", Config.direct_force ? 1 : 0);
	os_config_write_uint("Controls", "SwapActionCancel", Config.swap_action_cancel ? 1 : 0);

	// Speed
	os_config_write_uint(nullptr, "ComputerSpeed", Config.detail_level);

	// Network
	os_config_write_string("Network", "NetworkConnection", NetworkConnections[Config.network_connection]);
	os_config_write_string("Network", "ConnectionSpeed", NetworkSpeeds[Config.network_speed]);
	os_config_write_uint("Network", "ForcePort", Config.port);

	// PXO
	os_config_write_string("PXO", "Login", Config.pxo_login.c_str());
	os_config_write_string("PXO", "Password", Config.pxo_pass.c_str());
	os_config_write_uint("PXO", "SkipVerify", Config.pxo_skip_version_check ? 1 : 0);
	os_config_write_uint("PXO", "PXOBanners", Config.pxo_banners ? 1 : 0);

	// Misc
	if ( !Config.extras_path.empty() && Config.extras_path.back() != DIR_SEPARATOR_CHAR ) {
		Config.extras_path.push_back(DIR_SEPARATOR_CHAR);
	}

	os_config_write_string(nullptr, "ExtrasPath", Config.extras_path.c_str());

	if (Config.nosound) {
		Config.cmdline.append(" --nosound"); // with leading space
	}

	if (Config.nomusic) {
		Config.cmdline.append(" --nomusic"); // with leading space
	}

	if (Config.nomovies) {
		Config.cmdline.append(" --nomovies"); // with leading space
	}

	if (Config.nodpiscaling) {
		Config.cmdline.append(" --no_dpi_scaling");	// with leading space
	}

	char cmdline_cfg[MAX_PATH_LEN];
	cf_create_default_path_string(cmdline_cfg, CF_TYPE_DATA, "cmdline.cfg");

	auto cmdline = trim(Config.cmdline);

	if (cmdline.empty()) {
		SDL_RemovePath(cmdline_cfg);
	} else {
		SDL_SaveFile(cmdline_cfg, cmdline.c_str(), cmdline.size());
	}

	os_config_write_uint("Video", "BriefingAnimation", Config.nobriefanim ? 0 : 1);
}

static void tabVideo()
{
	if ( !ImGui::BeginTabItem("Video") ) {
		return;
	}

	ImGui::SeparatorText("Renderer");

	const char *renderers[] = {
		"Automatic", "OpenGL ES 2", "OpenGL (safe mode)"
	};

	ImGui::Combo("##renderer", &Config.renderer, renderers, SDL_arraysize(renderers));

	ImGui::SeparatorText("Options");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);
	ImGui::Checkbox("Fullscreen", &Config.fullscreen);
	ImGui::Checkbox("Show FPS", &Config.show_fps);
	ImGui::PopStyleVar();

	const char *antialias[] = {
		"Disabled", "2x", "4x", "8x", "16x"
	};

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Anti-Alias");
	ImGui::SameLine();
	ImGui::PushItemWidth(WindowScale->get(100.f));
	ImGui::Combo("##msaa", &Config.msaa, antialias, SDL_arraysize(antialias));
	ImGui::PopItemWidth();

	ImGui::EndTabItem();
}

static void tabAudio()
{
	if ( !ImGui::BeginTabItem("Audio") ) {
		return;
	}

	int unused_i = 0;

	ImGui::SeparatorText("Playback Device");

	ImGui::Combo("##playback", &unused_i, "Default\0");

	ImGui::SeparatorText("Capture Device");

	ImGui::Combo("##capture", &unused_i, "Default\0");

	ImGui::SeparatorText("Options");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);
	ImGui::Checkbox("EFX / EAX Reverb", &Config.efx);
#ifndef MAKE_FS1
	ImGui::Checkbox("Enable Launcher Sounds", &Config.launcher_sounds);
#endif
	ImGui::PopStyleVar();

	ImGui::EndTabItem();
}

static void tabControls()
{
	if ( !ImGui::BeginTabItem("Controls") ) {
		return;
	}

	ImGui::SeparatorText("Controller");

	int unused_i = 0;
	ImGui::Combo("##controller", &unused_i, "Default\0");

	ImGui::SeparatorText("Options");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);
	ImGui::Checkbox("Enable Force Feedback / Rumble", &Config.haptic);
	ImGui::Checkbox("Enable Directional Hit Effect", &Config.direct_force);
	ImGui::Checkbox("Swap Gamepad Action/Cancel Buttons", &Config.swap_action_cancel);
	ImGui::PopStyleVar();

	ImGui::EndTabItem();
}

static void tabSpeed()
{
	if ( !ImGui::BeginTabItem("Speed") ) {
		return;
	}

	ImGui::SeparatorText("Default Detail Level");

	const char *levels[] = {
		"Low", "Medium", "High", "Very High"
	};

	ImGui::Combo("##detail", &Config.detail_level, levels, SDL_arraysize(levels));

	ImGui::EndTabItem();
}

static void tabNetwork()
{
	if ( !ImGui::BeginTabItem("Network") ) {
		return;
	}

	ImGui::SeparatorText("Internet Connection");

	ImGui::BeginGroup();
	ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 3);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);

	const char *connection_names[SDL_arraysize(NetworkConnections)] = {
		"None", "Dialup Networking", "LAN/Direct Connection"
	};

	for (int i = 0; i < static_cast<int>(SDL_arraysize(NetworkConnections)); ++i) {
		ImGui::RadioButton(connection_names[i], &Config.network_connection, i);
	}

	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

	ImGui::SeparatorText("Connection Speed");

	ImGui::BeginGroup();
	ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 3);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);
	const char *speed_names[SDL_arraysize(NetworkSpeeds)] = {
		"None Specified",
		"Slower than 56K Modem",
		"56K Modem",
		"Single Channel ISDN",
		"Dual Channel ISDN, Cable Modems",
		"T1, ADSL, T3, etc."
	};

	for (int i = 0; i < static_cast<int>(SDL_arraysize(NetworkSpeeds)); ++i) {
		ImGui::RadioButton(speed_names[i], &Config.network_speed, i);
	}

	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

	ImGui::SeparatorText("Misc");

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Force Local Port");
	ImGui::SameLine();

	char port_buf[6] = "";

	if (Config.port > 0) {
		SDL_snprintf(port_buf, SDL_arraysize(port_buf), "%u", Config.port);
	}

	ImGui::PushItemWidth(WindowScale->get(75.f));
	ImGui::PushFont(Fonts[FONT_MONO].ptr, Fonts[FONT_MONO].size);
	if (ImGui::InputText("##port", port_buf, SDL_arraysize(port_buf), ImGuiInputTextFlags_CharsDecimal)) {
		if ( !SDL_strlen(port_buf) ) {
			Config.port = 0;
		} else {
			int port = SDL_atoi(port_buf);
			Config.port = SDL_clamp(port, 0, SDL_MAX_UINT16);
		}
	}
	ImGui::PopFont();
	ImGui::PopItemWidth();

	ImGui::EndTabItem();
}

static void tabPXO()
{
	if ( !ImGui::BeginTabItem("PXO") ) {
		return;
	}

	char login_buf[32] = "";
	char pass_buf[17] = "";

	ImGui::SeparatorText("PXO Account");

	// to line up login and pass input boxes, we need to offset the "Login" text
	float login_offset = ImGui::CalcTextSize("Password").x - ImGui::CalcTextSize("Login").x;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + login_offset);
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Login");
	ImGui::SameLine();
	ImGui::PushFont(Fonts[FONT_MONO].ptr, Fonts[FONT_MONO].size);
	SDL_strlcpy(login_buf, Config.pxo_login.c_str(), SDL_arraysize(login_buf));
	if (ImGui::InputText("##login", login_buf, SDL_arraysize(login_buf), ImGuiInputTextFlags_CharsDecimal)) {
		Config.pxo_login = login_buf;
	}
	ImGui::PopFont();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Password");
	ImGui::SameLine();
	ImGui::PushFont(Fonts[FONT_MONO].ptr, Fonts[FONT_MONO].size);
	SDL_strlcpy(pass_buf, Config.pxo_pass.c_str(), SDL_arraysize(pass_buf));
	if (ImGui::InputText("##password", pass_buf, SDL_arraysize(pass_buf), ImGuiInputTextFlags_CharsNoBlank)) {
		Config.pxo_pass = pass_buf;
	}
	ImGui::PopFont();

	ImGui::SeparatorText("Options");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);
	ImGui::Checkbox("Skip version check", &Config.pxo_skip_version_check);
	ImGui::Checkbox("Show lobby banners", &Config.pxo_banners);
	ImGui::PopStyleVar();

	ImGui::EndTabItem();
}

static void SDLCALL extras_folder_callback(void *userdata, const char * const *filelist, int filter)
{
	if ( !filelist || !(*filelist) ) {
		return;
	}

	Config.extras_path = *filelist;
}

static void tabMisc()
{
	if ( !ImGui::BeginTabItem("Misc") ) {
		return;
	}

	ImGui::SeparatorText("Extras Path");

	char extras_str[MAX_PATH_LEN] = "";

	if (ImGui::Button("Select")) {
		SDL_ShowOpenFolderDialog(extras_folder_callback, nullptr, Window,
								 nullptr, false);
	}

	if ( !Config.extras_path.empty() ) {
		SDL_strlcpy(extras_str, Config.extras_path.c_str(), SDL_arraysize(extras_str));
	}

	ImGui::PushFont(Fonts[FONT_MONO].ptr, Fonts[FONT_MONO].size);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::InputText("##extras", extras_str, SDL_arraysize(extras_str))) {
		Config.extras_path = extras_str;
	}
	ImGui::PopFont();

	ImGui::SeparatorText("Optional Command Line");

	char cmdline_str[1024] = "";

	ImGui::PushFont(Fonts[FONT_MONO].ptr, Fonts[FONT_MONO].size);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	SDL_strlcpy(cmdline_str, Config.cmdline.c_str(), SDL_arraysize(cmdline_str));
	if (ImGui::InputText("##cmdline", cmdline_str, SDL_arraysize(cmdline_str))) {
		Config.cmdline = cmdline_str;
	}
	ImGui::PopFont();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SpecialPadding);
	ImGui::Checkbox("Disable all audio", &Config.nosound);
	ImGui::Checkbox("Disable music", &Config.nomusic);
	ImGui::Checkbox("Disable movies", &Config.nomovies);
	ImGui::Checkbox("Disable DPI scaling", &Config.nodpiscaling);
#ifdef MAKE_FS1
	ImGui::Checkbox("Disable briefing animation", &Config.nobriefanim);
#endif
	ImGui::PopStyleVar();

	ImGui::EndTabItem();
}

static void AlignForWidth(float width, float alignment = 0.5f)
{
	float avail = ImGui::GetContentRegionAvail().x;
	float off = (avail - width) * alignment;
	if (off > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void launcher_setup_draw()
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

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove
								  | ImGuiWindowFlags_NoDecoration
								  | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::Begin("Setup", nullptr, window_flags);

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_DrawSelectedOverline;

	if (ImGui::BeginTabBar("SetupBar", tab_bar_flags)) {
		tabVideo();
		tabAudio();
		tabControls();
		tabSpeed();
		tabNetwork();
		tabPXO();
		tabMisc();

		ImGui::EndTabBar();
	}

	// center button group
	const float button_width = WindowScale->get(100.f);
	ImGuiStyle& style = ImGui::GetStyle();
	float width = button_width * 2.0f;	// two buttons
	width += style.ItemSpacing.x;
	AlignForWidth(width);
	// then place at bottom of window
	float y_offset = style.WindowPadding.y + style.ItemSpacing.y + ImGui::GetTextLineHeight() + 5.f;
	ImGui::SetCursorPosY(i2fl(h) - y_offset);

	ImGui::BeginGroup();

	if (ImGui::Button("Ok", ImVec2(button_width, 0))) {
		launcher_setup_save_config();
		close_window();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
		close_window();
	}

	ImGui::EndGroup();

	ImGui::End();

	// render
	ImGui::Render();
	SDL_SetRenderScale(Renderer, ImGui::GetIO().DisplayFramebufferScale.x,
					   ImGui::GetIO().DisplayFramebufferScale.y);
	SDL_SetRenderDrawColorFloat(Renderer, 0.2f, 0.4f, 0.6f, 1.0f);
	SDL_RenderClear(Renderer);

	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), Renderer);
	SDL_RenderPresent(Renderer);

	ImGui::SetCurrentContext(savedContext);
}
