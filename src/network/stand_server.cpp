/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/


#include "pstypes.h"
#include "multi.h"
#include "stand_server.h"

#ifndef __EMSCRIPTEN__

#include "osregistry.h"
#include "multi_options.h"
#include "gamesequence.h"
#include "version.h"
#include "multi_pmsg.h"
#include "multi_endgame.h"
#include "multimsgs.h"
#include "multiui.h"
#include "multiutil.h"
#include "freespace.h"
#include "missiongoals.h"
#include "cmdline.h"
#include "multi_kick.h"
#include "multi_fstracker.h"
#include "osregistry.h"
#include "multi_log.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_SHADOW
#include "ext/json.hpp"
DISABLE_WARNING_POP

#include <libwebsockets.h>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <atomic>


// Define this to use standalone ui in a separate thread
//#define STD_THREADED

#ifdef STD_THREADED
#include <thread>
#endif


namespace {

using json = nlohmann::json;

#ifdef STD_THREADED
std::thread Standalone_thread;
#endif

#define STANDALONE_MAX_BAN		50
std::vector<std::string> Standalone_ban_list;

enum UpdateTimes {
	info			= 3000,
	netgame			= 5000,
	mission_time	= 10000,
};

enum class PopupTypes {
	Status,
	Notice,
	Alert
};

struct chatlog_item {
	std::string id;
	std::string message;
};

struct Standalone_client {
	uint32_t m_id;
	struct lws *m_wsi;

	std::list<std::string> m_send_buffer;

	short m_active_player;
	bool m_multilog_enabled;

	uint64_t m_info_timestamp;
	uint64_t m_netgame_timestamp;
	uint64_t m_mission_time_timestamp;

	Standalone_client(uint32_t id, struct lws *wsi) :
		m_id(id), m_wsi(wsi), m_active_player(-1), m_multilog_enabled(true),
		m_info_timestamp(0), m_netgame_timestamp(0), m_mission_time_timestamp(0)
		{};

	~Standalone_client() {};
};

class StandaloneUI {
private:
	lws_context *m_lws_context;
	std::string m_interface;
	time_t m_start_time;

	const struct lws_protocols m_lws_protocols[3] = {
		{ "http", lws_callback_http_dummy, 0, 0 },
		{ "standalone", ext_callback_standalone, sizeof(uint32_t), 0, 1, this },
		{ nullptr, nullptr, 0, 0 }		// terminator
	};

	std::unique_ptr<struct lws_http_mount> m_lws_mount_root;
#ifndef NDEBUG
	std::unique_ptr<struct lws_http_mount> m_lws_mount_dev;
	std::string m_dev_mount_origin;
#endif

	std::string m_title;
	std::string m_state_text;
	json m_status_popup;

	bool m_pxo_refresh_state;
	bool m_pxo_enabled;

	void pxo_refresh();

	uint32_t m_next_client_id;

	int m_mission_time;
	int m_realized_fps;
	bool m_host_connected;
	int m_num_players;

	static constexpr size_t MAX_MULTILOG_LINES = 100;
	std::deque<std::string> m_multilog;

	static constexpr size_t MAX_CHATLOG_LINES = 50;
	std::deque<chatlog_item> m_chatlog;

	static constexpr size_t MAX_STD_CLIENTS = 5;
	std::list<std::unique_ptr<Standalone_client>> m_clients;

	Standalone_client *m_active_client;

	uint32_t getNewClientId() {
		uint32_t next = m_next_client_id++;

		if ( !m_next_client_id ) {
			m_next_client_id = 1;
		}

		return next;
	}

	bool add_message(const json &msg);
	void update_connections();

	void do_frame();

	lws_callback_function callback_standalone;

	void msg_handler_server(const json &msg);
	void msg_handler_player(const json &msg);

public:
	StandaloneUI();
	~StandaloneUI();

	static int ext_callback_standalone(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
		return reinterpret_cast<StandaloneUI*>(lws_get_protocol(wsi)->user)->callback_standalone(wsi, reason, user, in, len);
	}

	void process();
	void shutdown();

	void reset_all();
	void reset();
	void reset_timestamps();

	void server_set_state(const char *str);
	void server_set_realized_fps(int fps) { m_realized_fps = fps; }
	void server_set_host_status(bool connected) { m_host_connected = connected; }
	void server_set_num_players(int count) { m_num_players = count; }
	void server_update_vals();

	void netgame_set_name();
	void netgame_update();

	void player_add(const net_player *p);
	void player_update(const net_player *p);
	void player_info(const net_player *p);
	void player_remove(const net_player *p);

	void chat_add_text(const char *text, int player_index, int add_id);
	void chat_send_text(const std::string &id, const std::string &text);
	void chat_refresh();

	void multilog_add_line(const char *line);
	void multilog_refresh();

	void popup(PopupTypes type, const char *title = nullptr, const char *msg = nullptr);
	void popup_set_text(const char *str, int field_num);	// for status popup only
	void popup_close();	// for status popup only

	void mission_set_time(float mission_time);
	void mission_update_time();
	void mission_set_goals();
};


std::unique_ptr<StandaloneUI> Standalone(nullptr);
std::atomic<bool> Standalone_running(false);

void std_lws_logger(int level, const char *line)
{
	if (level & LLL_ERR) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "STD: %s", line);
	} else if (level & (LLL_WARN|LLL_NOTICE)) {
		nprintf(("lws", "STD: %s", line));
	}
}

lws_fop_fd_t std_lws_cfopen(const struct lws_plat_file_ops *fops_own,
							const struct lws_plat_file_ops *fops,
							const char *filename, const char *vpath,
							lws_fop_flags_t *flags)
{
	if ( !filename || !SDL_strlen(filename) ) {
		return nullptr;
	}

	auto filep = cfopen(filename, "rb", CF_TYPE_ROOT);

	if ( !filep ) {
		return nullptr;
	}

	auto fop_fd = new lws_fop_fd;

	SDL_zerop(fop_fd);

	fop_fd->fd = reinterpret_cast<lws_filefd_type>(1);	// unused, but shouldn't be 0/null
	fop_fd->fops = fops;
	fop_fd->flags = *flags;
	fop_fd->len = static_cast<lws_filepos_t>(cfilelength(filep));
	fop_fd->pos = 0;

	fop_fd->filesystem_priv = new CFILE;

	SDL_memcpy(fop_fd->filesystem_priv, filep, sizeof(CFILE));

	// prevent lws from trying to stat file since it will almost always result
	// in a failure or incorrect/invalid info
	// (shouldn't be on *our* flags though, so make sure we set that already)
	*flags |= LWS_FOP_FLAG_VIRTUAL;

	return fop_fd;
}

int std_lws_cfclose(lws_fop_fd_t *fop_fd)
{
	if (fop_fd && *fop_fd) {
		auto filep = reinterpret_cast<CFILE *>((*fop_fd)->filesystem_priv);

		if (filep) {
			cfclose(filep);
			delete filep;
		}

		delete *fop_fd;
		*fop_fd = nullptr;
	}

	return 0;
}

lws_fileofs_t std_lws_cfseek_cur(lws_fop_fd_t fop_fd, lws_fileofs_t offset)
{
	auto filep = reinterpret_cast<CFILE *>(fop_fd->filesystem_priv);

	if ( !filep ) {
		return -1;
	}

	if (cfseek(filep, offset, CF_SEEK_CUR)) {
		auto rval = static_cast<lws_fileofs_t>(cftell(filep));
		fop_fd->pos = rval;

		return rval;
	}

	return -1;
}

int std_lws_cfread(lws_fop_fd_t fop_fd, lws_filepos_t *amount,
				   uint8_t *buf, lws_filepos_t len)
{
	auto filep = reinterpret_cast<CFILE *>(fop_fd->filesystem_priv);

	if ( !filep ) {
		return -1;
	}

	auto bytes_read = cfread(buf, 1, len, filep);

	if ( len && !bytes_read ) {
		*amount = 0;
		return -1;
	}

	fop_fd->pos += static_cast<lws_filepos_t>(bytes_read);

	*amount = static_cast<lws_filepos_t>(bytes_read);

	return 0;
}

int std_lws_cfwrite(lws_fop_fd_t fop_fd, lws_filepos_t *amount,
					uint8_t *buf, lws_filepos_t len)
{
	*amount = 0;

	return -1;
}


StandaloneUI::StandaloneUI()
{
	struct lws_context_creation_info info{};

	m_lws_mount_root = std::make_unique<struct lws_http_mount>();

	m_lws_mount_root->mountpoint = "/";
	m_lws_mount_root->mountpoint_len = 1;
	m_lws_mount_root->origin = "standalone-web.zip";
	m_lws_mount_root->origin_protocol = LWSMPRO_FILE;
	m_lws_mount_root->def = "index.html";

#ifndef NDEBUG
	// add special developer mount for easier frontend work
	m_lws_mount_dev = std::make_unique<struct lws_http_mount>();

	auto cwd = SDL_GetCurrentDirectory();

	if (cwd) {
		m_dev_mount_origin = cwd;

		SDL_free(cwd);
		cwd = nullptr;
	}

	m_dev_mount_origin += "standalone-web";

	m_lws_mount_dev->mountpoint = "/dev";
	m_lws_mount_dev->mountpoint_len = 4;
	m_lws_mount_dev->origin = m_dev_mount_origin.c_str();
	m_lws_mount_dev->origin_protocol = LWSMPRO_FILE;
	m_lws_mount_dev->def = "index.html";

	// add dev mount to root
	m_lws_mount_root->mount_next = m_lws_mount_dev.get();
#endif

	if (os_config_read_uint("Network", "RestrictStandAdmin", 1) == 0) {
		info.iface = nullptr;
		m_interface = "<all>";
	} else if ( !SDL_strlen(Multi_options_g.std_listen_addr) ) {
		// this option is to get around a libwebsockets bug that prevented
		// binding to a IPv4 iface address properly
		info.options |= LWS_SERVER_OPTION_DISABLE_IPV6;
		info.iface = "127.0.0.1";
		m_interface = "127.0.0.1";
	} else {
		info.iface = Multi_options_g.std_listen_addr;
		m_interface = Multi_options_g.std_listen_addr;
	}

	m_interface += std::string(":") + std::to_string(Multi_options_g.port);

	info.port = Multi_options_g.port;
	info.protocols = m_lws_protocols;
	info.mounts = m_lws_mount_root.get();

	info.gid = static_cast<gid_t>(-1);
	info.uid = static_cast<uid_t>(-1);

	lws_set_log_level(LLL_ERR|LLL_WARN|LLL_NOTICE, std_lws_logger);

	m_lws_context = lws_create_context(&info);

	if ( !m_lws_context ) {
		throw std::runtime_error("Failed to create LWS context!");
	}

	// use our file ops
	lws_get_fops(m_lws_context)->open = std_lws_cfopen;
	lws_get_fops(m_lws_context)->close = std_lws_cfclose;
	lws_get_fops(m_lws_context)->seek_cur = std_lws_cfseek_cur;
	lws_get_fops(m_lws_context)->read = std_lws_cfread;
	lws_get_fops(m_lws_context)->write = std_lws_cfwrite;

	m_start_time = time(nullptr);

	char title[64];
	SDL_snprintf(title, SDL_arraysize(title), "%s %d.%02d.%02d",
				 XSTR("FreeSpace Standalone", 935),
				 FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_VERSION_BUILD);
	m_title = title;

	m_pxo_refresh_state = false;
	m_pxo_enabled = (Multi_options_g.pxo == 1);

	m_next_client_id = 1;

	m_active_client = nullptr;

	m_mission_time = -1;
	m_realized_fps = 0;
	m_host_connected = false;
	m_num_players = 0;

	Standalone_running.store(true, std::memory_order_relaxed);
}

StandaloneUI::~StandaloneUI()
{
	shutdown();
}

int StandaloneUI::callback_standalone(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	constexpr size_t MAX_BUF_SIZE = 2048;

	unsigned char buf[LWS_PRE + MAX_BUF_SIZE];
	unsigned char *p = &buf[LWS_PRE];
	int exit_val = 0;

	uint32_t *client_id = reinterpret_cast<uint32_t *>(user);

	if (client_id && *client_id) {
		for (auto &client : m_clients) {
			if (client->m_id == *client_id) {
				m_active_client = client.get();
				break;
			}
		}
	}

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED: {
			SDL_assert(m_active_client == nullptr);

			uint32_t cid = getNewClientId();

			m_clients.emplace_back(std::make_unique<Standalone_client>(cid, wsi));

			*client_id = cid;
			m_active_client = m_clients.back().get();

			reset();

			break;
		}

		case LWS_CALLBACK_CLOSED: {
			if ( !m_active_client ) {
				break;
			}

			for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
				if ((*it)->m_id == m_active_client->m_id) {
					m_clients.erase(it);
					break;
				}
			}

			break;
		}

		case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: {
			if (m_clients.size() >= MAX_STD_CLIENTS) {
				exit_val = -1;
			}

			break;
		}

		case LWS_CALLBACK_SERVER_WRITEABLE: {
			if ( !m_active_client ) {
				break;
			}

			while ( !m_active_client->m_send_buffer.empty() ) {
				if (m_active_client->m_send_buffer.front().size() >= MAX_BUF_SIZE) {
					lwsl_warn("Message size (%zu) exceeds buffer size (%zu)!  Discarding...\n", m_active_client->m_send_buffer.size(), MAX_BUF_SIZE);
					m_active_client->m_send_buffer.pop_front();

					continue;
				}

				auto size = SDL_strlcpy((char *)p, m_active_client->m_send_buffer.front().c_str(), MAX_BUF_SIZE);

				auto rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < static_cast<int>(size)) {
					lwsl_err("ERROR sending buffer!\n");
					lws_close_reason(wsi, LWS_CLOSE_STATUS_UNEXPECTED_CONDITION, (unsigned char *)"write error", 11);

					exit_val = -1;
					break;
				}

				m_active_client->m_send_buffer.pop_front();

				if ( lws_send_pipe_choked(wsi) ) {
					lws_callback_on_writable(wsi);

					break;
				}
			}

			break;
		}

		case LWS_CALLBACK_RECEIVE: {
			if ( !len || !in ) {
				break;
			}

			try {
				std::string str(reinterpret_cast<const char *>(in), len);
				json msg = json::parse(str);

				// "final" messages - if it exists, do and bail
				if (msg.contains("shutdown")) {
					lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"shutdown", 8);
					gameseq_post_event(GS_EVENT_QUIT_GAME);
					Standalone_running.store(false, std::memory_order_relaxed);

					exit_val = -1;
					break;
				}

				if (msg.contains("reset_all")) {
					multi_quit_game(PROMPT_NONE);
					reset_all();

					break;
				}

				// all other messages
				for (auto it = msg.begin(); it != msg.end(); ++it) {
					if (it.key() == "server") {
						json server_msg = it.value();
						msg_handler_server(server_msg);
					} else if (it.key() == "player") {
						json player_msg = it.value();
						msg_handler_player(player_msg);
					}
					// in-game chat box messages from server
					else if (it.key() == "chat") {
						auto txt = it.value().get<std::string>();

						if ( !txt.empty() ) {
							send_game_chat_packet(Net_player, txt.c_str(), MULTI_MSG_ALL, nullptr);
							std_add_chat_text(txt.c_str(), MY_NET_PLAYER_NUM, 1);
						}
					}
					// enable/disable sending of multi log (per client)
					else if (it.key() == "multilog") {
						auto enabled = it.value().get<bool>();

						if (m_active_client) {
							m_active_client->m_multilog_enabled = enabled;

							// if we are enabling the multilog then send all that we have to the client
							multilog_refresh();
						}
					}
				}
			} catch (json::exception &e) {
				ml_printf("STD => Exception caught handling client message: %s", e.what());
			}

			break;
		}

		default:
			break;
	}

	m_active_client = nullptr;

	return exit_val;
}

void StandaloneUI::msg_handler_server(const json &msg)
{
	for (auto it = msg.begin(); it != msg.end(); ++it) {
		// name
		if (it.key() == "name") {
			auto name = it.value().get<std::string>();

			if ( name.empty() ) {
				name = XSTR("Standalone Server", 916);
			}

			SDL_strlcpy(Multi_options_g.std_pname, name.c_str(), SDL_arraysize(Multi_options_g.std_pname));

			// if no host is connected then set as netgame name too
			if ( !Netgame.host ) {
				SDL_strlcpy(Netgame.name, name.c_str(), SDL_arraysize(Netgame.name));
			}

		}

		// password
		else if (it.key() == "password") {
			auto pass = it.value().get<std::string>();
			SDL_strlcpy(Multi_options_g.std_passwd, pass.c_str(), SDL_arraysize(Multi_options_g.std_passwd));
		}

		// allow voice
		else if (it.key() == "voice") {
			auto voice = it.value().get<bool>();
			Multi_options_g.std_voice = voice ? 1 : 0;
		}

		// server update rate
		else if (it.key() == "update_rate") {
			auto obj_update = it.value().get<int>();

			if ( (obj_update >= 0) && (obj_update < MAX_OBJ_UPDATE_LEVELS) ) {
				Multi_options_g.std_datarate = obj_update;
				Net_player->p_info.options.obj_update_level = obj_update;
			}
		}

		// max players
		else if (it.key() == "max_players") {
			auto max_players = it.value().get<int>();

			if ( (max_players == -1) || !((max_players < 1) || (max_players > MAX_PLAYERS)) ) {
				Multi_options_g.std_max_players = max_players;
			}
		}

		// PXO
		else if (it.key() == "pxo") {
			auto pxo = it.value().get<bool>();

			if (Multi_options_g.pxo && !pxo) {
				m_pxo_refresh_state = true;
				m_pxo_enabled = false;
			} else if ( !Multi_options_g.pxo && pxo ) {
				m_pxo_refresh_state = true;
				m_pxo_enabled = true;
			} else {
				// if we reverted to the original state then abort the refresh
				m_pxo_refresh_state = false;
			}

			// if we can't do this immediately then notify the client of that fact
			if (m_pxo_refresh_state && m_num_players) {
				popup(PopupTypes::Notice, nullptr,
					  "PXO state change will take affect at the conclusion of the current game.");
			}

			pxo_refresh();
		}

		// PXO channel
		else if (it.key() == "pxo_channel") {
			auto channel = it.value().get<std::string>();

			if ( !channel.empty() && ((channel == "global") || (channel.front() == '#') || (channel.front() == '$')) ) {
				SDL_strlcpy(Multi_fs_tracker_channel, channel.c_str(), SDL_arraysize(Multi_fs_tracker_channel));
				m_pxo_refresh_state = true;
				pxo_refresh();
			}
		}

		// frame cap
		else if (it.key() == "framecap") {
			auto cap = it.value().get<int>();

			CAP(cap, 15, 120);
			Multi_options_g.std_framecap = cap;
		}

		// re-validate missions on PXO
		else if (it.key() == "validate") {
			if (Multi_options_g.pxo) {
				cf_delete(MULTI_VALID_MISSION_FILE, CF_TYPE_DATA);

				multi_update_valid_missions();
			}
		}
	}
}

void StandaloneUI::msg_handler_player(const json &msg)
{
	for (auto it = msg.begin(); it != msg.end(); ++it) {
		// kick player
		if (it.key() == "kick") {
			auto player_id = it.value().get<short>();
			int idx = find_player_id(player_id);

			multi_kick_player(idx, 0);
		}

		// player info/stats
		else if (it.key() == "info") {
			auto player_id = it.value().get<short>();

			if ((player_id < 0) || (m_active_client->m_active_player == player_id)) {
				m_active_client->m_active_player = -1;
			} else {
				int idx = find_player_id(player_id);

				if (idx >= 0) {
					player_info(&Net_players[idx]);
					m_active_client->m_active_player = player_id;
				} else {
					m_active_client->m_active_player = -1;
				}
			}
		}
	}
}

bool StandaloneUI::add_message(const json &msg)
{
	// if no client then don't add messages
	if (m_clients.empty()) {
		return false;
	}

	const std::string msg_str = msg.dump();

	if (m_active_client) {
		m_active_client->m_send_buffer.push_back(msg_str);
	} else {
		for (auto &client : m_clients) {
			client->m_send_buffer.push_back(msg_str);
		}
	}

	return true;
}

void StandaloneUI::update_connections()
{
	for (const auto &player : Net_players) {
		if (MULTI_CONNECTED(player) && (Net_player != &player)) {
			player_add(&player);
		}
	}
}

void StandaloneUI::shutdown()
{
	if (m_lws_context) {
		lws_cancel_service(m_lws_context);
		SDL_Delay(1000);
		lws_context_destroy(m_lws_context);
		m_lws_context = nullptr;
	}
}

void StandaloneUI::process()
{
#ifdef STD_THREADED
	while (Standalone_running.load(std::memory_order_relaxed)) {
		do_frame();

		lws_service(m_lws_context, -1);

		SDL_Delay(1000/30);
	}

	shutdown();
#else
	do_frame();

	lws_service(m_lws_context, -1);
#endif
}

void StandaloneUI::do_frame()
{
	if (m_clients.empty()) {
		return;
	}

	const uint64_t cur_time_ms = SDL_GetTicks();

	auto prev_client = m_active_client;

	// update client specific stuff
	for (auto &client : m_clients) {
		m_active_client = client.get();

		// maybe update netgame info
		if (m_num_players) {
			if ( !client->m_netgame_timestamp || (cur_time_ms > client->m_netgame_timestamp) ) {
				client->m_netgame_timestamp = cur_time_ms + UpdateTimes::netgame;

				netgame_update();
			}
		}

		// maybe update mission time
		if (m_mission_time != -1) {
			if ( !client->m_mission_time_timestamp || (cur_time_ms > client->m_mission_time_timestamp) ) {
				client->m_mission_time_timestamp = cur_time_ms + UpdateTimes::mission_time;

				mission_update_time();
			}
		}

		// maybe update server/player info
		if (m_num_players) {
			if ( !client->m_info_timestamp || (cur_time_ms > client->m_info_timestamp) ) {
				client->m_info_timestamp = cur_time_ms + UpdateTimes::info;

				// server info
				json msg;

				msg["server_info"]["realized_fps"] = m_realized_fps;
				msg["server_info"]["host_connected"] = m_host_connected;
				msg["server_info"]["num_players"] = m_num_players;

				add_message(msg);

				// player info
				if (client->m_active_player != -1) {
					int player_idx = find_player_id(client->m_active_player);

					if (player_idx >= 0) {
						player_info(&Net_players[player_idx]);
					}
				}
			}
		}

		if ( !client->m_send_buffer.empty() ) {
			lws_callback_on_writable(client->m_wsi);
		}
	}

	m_active_client = prev_client;
}

void StandaloneUI::server_set_state(const char *str)
{
	m_state_text = str;

	if (m_clients.empty()) {
		return;
	}

	json msg;

	msg["server_info"]["state"] = str;

	add_message(msg);
}

void StandaloneUI::server_update_vals()
{
	if (m_clients.empty()) {
		return;
	}

	std::string build;
	json msg;

	build = Osreg_title;
	build += " ";
	build += version_get_string_full();

	// info / status
	msg["server_info"]["build"] = build;
	msg["server_info"]["address"] = m_interface;
	msg["server_info"]["start_time"] = m_start_time;
	msg["server_info"]["title"] = m_title;
	msg["server_info"]["state"] = m_state_text;
	msg["server_info"]["multi_version"] = MULTI_FS_SERVER_VERSION;
	msg["server_info"]["host_connected"] = m_host_connected;
	msg["server_info"]["realized_fps"] = m_realized_fps;
	msg["server_info"]["num_players"] = m_num_players;

	// settings / options
	if ( SDL_strlen(Multi_options_g.std_pname) ) {
		msg["server"]["name"] = Multi_options_g.std_pname;
	} else {
		msg["server"]["name"] = XSTR("Standalone Server", 916);
	}

	msg["server"]["password"] = Multi_options_g.std_passwd;
	msg["server"]["max_players"] = Multi_options_g.std_max_players;
	msg["server"]["voice"] = Multi_options_g.std_voice;
	msg["server"]["update_rate"] = Multi_options_g.std_datarate;
	msg["server"]["framecap"] = Multi_options_g.std_framecap;
	msg["server"]["pxo"] = m_pxo_enabled;	// use intended value, not actual
	msg["server"]["pxo_channel"] = Multi_fs_tracker_channel;

	add_message(msg);
}

void StandaloneUI::netgame_set_name()
{
	if (m_clients.empty()) {
		return;
	}

	json msg;

	// use netgame name if it's from host, otherwise trigger field reset
	msg["netgame"]["name"] = Netgame.host ? Netgame.name : "";

	add_message(msg);
}

void StandaloneUI::netgame_update()
{
	if (m_clients.empty()) {
		return;
	}

	json msg;
	std::string mode;
	std::string type;
	std::string state;

	switch (Netgame.mode) {
		case NG_MODE_OPEN:
			mode = XSTR("Open", 1322);
			break;
		case NG_MODE_CLOSED:
			mode = XSTR("Closed", 1323);
			break;
		case NG_MODE_PASSWORD:
			mode = XSTR("Password Protected", 1325);
			break;
		case NG_MODE_RESTRICTED:
			mode = XSTR("Restricted", 1324);
			break;
		case NG_MODE_RANK_ABOVE:
		case NG_MODE_RANK_BELOW:
			mode = "Rank Limited";
			break;
	}

	if (Netgame.type_flags & NG_TYPE_COOP) {
		type = XSTR("Coop", 1257);
	} else if (Netgame.type_flags & NG_TYPE_TEAM) {
		type = XSTR("Team", 1258);

		if (Netgame.type_flags & NG_TYPE_SW) {
			type.append(" (SquadWar)");
		}
	} else if (Netgame.type_flags & NG_TYPE_DOGFIGHT) {
		type = XSTR("Dogfight", 1259);
	}

	switch (Netgame.game_state) {
		case NETGAME_STATE_FORMING:
			state = XSTR("Forming", 764);
			break;
		case NETGAME_STATE_BRIEFING:
			state = XSTR("Briefing", 765);
			break;
		case NETGAME_STATE_DEBRIEF:
		case NETGAME_STATE_ENDGAME:
			state = XSTR("Debrief", 766);
			break;
		case NETGAME_STATE_PAUSED:
			state = XSTR("Paused", 767);
			break;
		case NETGAME_STATE_IN_MISSION:
		case NETGAME_STATE_MISSION_SYNC:
			state = XSTR("Playing", 768);
			break;
		default:
			state = XSTR("Unknown", 769);
	}

	msg["netgame"]["mission_name"] = Netgame.mission_name;
	msg["netgame"]["mission_title"] = Netgame.title;

	if (Netgame.campaign_mode) {
		msg["netgame"]["campaign_name"] = Netgame.campaign_name;
	} else {
		msg["netgame"]["campaign_name"] = "";
	}

	msg["netgame"]["mode"] = mode;
	msg["netgame"]["type"] = type;
	msg["netgame"]["state"] = state;

	msg["netgame"]["max_players"] = Netgame.max_players;
	msg["netgame"]["max_observers"] = Netgame.options.max_observers;
	msg["netgame"]["max_respawns"] = Netgame.respawn;

	add_message(msg);
}

void StandaloneUI::player_add(const net_player *p)
{
	SDL_assert(p);

	if (m_clients.empty()) {
		return;
	}

	char ip_address[INET_ADDRSTRLEN];
	json msg;

	msg["player"]["add"]["id"] = p->player_id;
	msg["player"]["add"]["name"] = p->player->callsign;
	msg["player"]["add"]["ping"] = p->s_info.ping.ping_avg;

	msg["player"]["add"]["host"] = MULTI_HOST((*p)) != 0;
	msg["player"]["add"]["observer"] = MULTI_OBSERVER((*p)) != 0;

	psnet_addr_to_string(ip_address, SDL_arraysize(ip_address), &p->p_info.addr);

	std::string address = ip_address;
	address += std::string(":") + std::to_string(p->p_info.addr.port);

	msg["player"]["add"]["address"] = address;

	add_message(msg);
}

void StandaloneUI::player_update(const net_player *p)
{
	SDL_assert(p);

	if (m_clients.empty()) {
		return;
	}

	json msg;

	msg["player"]["update"]["id"] = p->player_id;
	msg["player"]["update"]["ping"] = p->s_info.ping.ping_avg;

	msg["player"]["update"]["host"] = MULTI_HOST((*p)) != 0;
	msg["player"]["update"]["observer"] = MULTI_OBSERVER((*p)) != 0;

	add_message(msg);
}

void StandaloneUI::player_info(const net_player *p)
{
	if (m_clients.empty()) {
		return;
	}

	json msg;
	json info;
	char temp_str[50];

	info["id"] = p->player_id;
	info["name"] = p->player->callsign;
	info["ping"] = p->s_info.ping.ping_avg;

	// ip address
	psnet_addr_to_string(temp_str, SDL_arraysize(temp_str), &p->p_info.addr);

	std::string address = temp_str;
	address += std::string(":") + std::to_string(p->p_info.addr.port);

	info["address"] = address;

	// ship type
	info["ship"] = Ship_info[p->p_info.ship_class].name;

	// rank
	multi_sg_rank_build_name(Ranks[p->player->stats.rank].name, temp_str, SDL_arraysize(temp_str));
	info["rank"] = temp_str;

	// flight time
	game_format_time(p->player->stats.missions_flown, temp_str, SDL_arraysize(temp_str));
	info["flight_time"] = temp_str;

	// missions
	info["missions_flown"] = p->player->stats.missions_flown;

	// stats
	scoring_struct *ptr = &p->player->stats;
	std::vector<unsigned int> stats;

	stats.reserve(7);

	// all-time
	stats.push_back(ptr->kill_count);
	stats.push_back(ptr->kill_count - ptr->kill_count_ok);
	stats.push_back(ptr->assists);
	stats.push_back(ptr->p_shots_fired);
	stats.push_back(ptr->p_shots_fired ? (unsigned int)(100.0f * ((float)ptr->p_shots_hit / (float)ptr->p_shots_fired)) : 0);
	stats.push_back(ptr->s_shots_fired);
	stats.push_back(ptr->s_shots_fired ? (unsigned int)(100.0f * ((float)ptr->s_shots_hit / (float)ptr->s_shots_fired)) : 0);

	info["stats"]["all-time"] = stats;

	stats.clear();

	// mission
	stats.push_back(ptr->m_kill_count);
	stats.push_back(ptr->m_kill_count - ptr->m_kill_count_ok);
	stats.push_back(ptr->m_assists);
	stats.push_back(ptr->mp_shots_fired);
	stats.push_back(ptr->mp_shots_fired ? (unsigned int)(100.0f * ((float)ptr->mp_shots_hit / (float)ptr->mp_shots_fired)) : 0);
	stats.push_back(ptr->ms_shots_fired);
	stats.push_back(ptr->ms_shots_fired ? (unsigned int)(100.0f * ((float)ptr->ms_shots_hit / (float)ptr->ms_shots_fired)) : 0);

	info["stats"]["mission"] = stats;

	// final msg layout

	msg["player"]["info"] = info;

	add_message(msg);
}

void StandaloneUI::player_remove(const net_player *p)
{
	if (m_clients.empty()) {
		return;
	}

	json msg;

	msg["player"]["remove"]["id"] = p->player_id;

	add_message(msg);

	// clear active player, client should reset if needed
	for (auto &client : m_clients) {
		if (client->m_active_player == p->player_id) {
			client->m_active_player = -1;
			client->m_info_timestamp = 0;
		}
	}
}

void StandaloneUI::chat_add_text(const char *text, int player_index, int add_id)
{
	SDL_assert( (player_index >= 0) && (player_index < MAX_PLAYERS) );

	if ( !text ) {
		return;
	}

	chatlog_item item;

	item.message = text;

	if (add_id) {
		if ( MULTI_STANDALONE(Net_players[player_index]) ) {
			item.id = XSTR("<SERVER> %s", 924);

			size_t idx = item.id.find(">");

			if (idx != std::string::npos) {
				item.id.erase(idx+1, std::string::npos);
			}
		} else {
			item.id = Net_players[player_index].player->callsign;
		}
	}

	m_chatlog.push_back(std::move(item));

	if (m_chatlog.size() > MAX_CHATLOG_LINES) {
		m_chatlog.pop_front();
	}

	chat_send_text(m_chatlog.back().id, m_chatlog.back().message);
}

void StandaloneUI::chat_send_text(const std::string &id, const std::string &message)
{
	if (m_clients.empty()) {
		return;
	}

	json msg;

	msg["chat"]["id"] = id;
	msg["chat"]["message"] = message;

	add_message(msg);
}

void StandaloneUI::chat_refresh()
{
	// active client only
	if ( !m_active_client ) {
		return;
	}

	json msg;

	for (const auto &item : m_chatlog) {
		chat_send_text(item.id, item.message);
	}
}

void StandaloneUI::pxo_refresh()
{
	// bail if we don't need to be here or if it's not safe to switch pxo state
	if ( !m_pxo_refresh_state || m_num_players ) {
		return;
	}

	m_pxo_refresh_state = false;

	multi_fs_tracker_logout();

	Multi_options_g.pxo = (m_pxo_enabled ? 1 : 0);

	if (m_pxo_enabled) {
		std_tracker_login();
	}
}

void StandaloneUI::reset_all()
{
	// refresh PXO state if it's safe to do so
	pxo_refresh();

	// we should clear chat log here and start fresh
	m_chatlog.clear();
	m_chatlog.shrink_to_fit();

	if (m_clients.empty()) {
		return;
	}

	auto prev_client = m_active_client;

	for (auto &client : m_clients) {
		m_active_client = client.get();
		reset();
	}

	m_active_client = prev_client;
}

void StandaloneUI::reset()
{
	if (m_clients.empty()) {
		return;
	}

	if (m_active_client) {
		m_active_client->m_send_buffer.clear();
	}

	json msg;

	// send gui reset
	msg["reset_gui"] = true;

	add_message(msg);

	// refresh server information
	server_update_vals();

	// refresh various ui elements
	chat_refresh();

	mission_set_time(0.0f);
	mission_update_time();
	mission_set_goals();

	// refresh netgame data
	netgame_set_name();
	netgame_update();

	// refresh connections
	update_connections();

	// popup - only if active
	if ( !m_status_popup.empty() ) {
		msg.clear();
		msg["popup"]["status"] = m_status_popup;

		add_message(msg);
	}

	// refresh client-side state
	if (m_active_client) {
		m_active_client->m_netgame_timestamp = 0;
		m_active_client->m_info_timestamp = 0;
		m_active_client->m_mission_time_timestamp = 0;
		m_active_client->m_active_player = -1;
	}
}

void StandaloneUI::reset_timestamps()
{
	for (auto &client : m_clients) {
		client->m_netgame_timestamp = 0;
		client->m_info_timestamp = 0;
		client->m_mission_time_timestamp = 0;
	}
}

void StandaloneUI::multilog_add_line(const char *line)
{
	SDL_assert(line);

	m_multilog.emplace_back(line);

	if (m_multilog.size() > MAX_MULTILOG_LINES) {
		m_multilog.pop_front();
	}

	if (m_clients.empty()) {
		return;
	}

	json msg;

	auto prev_client = m_active_client;

	for (auto &client : m_clients) {
		m_active_client = client.get();

		if (client->m_multilog_enabled) {
			msg["multilog"] = line;

			add_message(msg);
		}
	}

	m_active_client = prev_client;
}

void StandaloneUI::multilog_refresh()
{
	// active client only
	if ( !m_active_client || !m_active_client->m_multilog_enabled ) {
		return;
	}

	json msg;

	for (const auto &line : m_multilog) {
		msg["multilog"] = line;

		add_message(msg);
	}
}

void StandaloneUI::popup(PopupTypes type, const char *title, const char *message)
{
	if (m_clients.empty()) {
		return;
	}

	json msg;

	if (type == PopupTypes::Status) {
		SDL_assert(title);

		m_status_popup["title"] = title;

		m_status_popup["field1"] = "";
		m_status_popup["field2"] = "";

		msg["popup"]["status"] = m_status_popup;
	} else if (type == PopupTypes::Notice) {
		if ( !message ) {
			Int3();
			return;
		}

		msg["popup"]["notice"]["title"] = title ? title : "Notice";
		msg["popup"]["notice"]["message"] = message;
	} else if (type == PopupTypes::Alert) {
		if ( !message ) {
			Int3();
			return;
		}

		msg["popup"]["alert"]["title"] = title ? title : "Alert!";
		msg["popup"]["alert"]["message"] = message;
	} else {
		return;
	}

	if ( add_message(msg) ) {
		// trigger write callback so we send this message quickly
		for (auto &client : m_clients) {
			lws_callback_on_writable(client->m_wsi);
		}

		lws_service(m_lws_context, -1);
	}
}

void StandaloneUI::popup_set_text(const char *str, int field_num)
{
	SDL_assert(str);

	if (m_clients.empty()) {
		return;
	}

	switch (field_num) {
		case 0:
			m_status_popup["title"] = str;
			break;

		case 1:
			m_status_popup["field1"] = str;
			break;

		case 2:
			m_status_popup["field2"] = str;
			break;

		default:
			return;
	}


	json msg;

	msg["popup"]["status"] = m_status_popup;

	if ( add_message(msg) ) {
		// trigger write callback so we send this message quickly
		for (auto &client : m_clients) {
			lws_callback_on_writable(client->m_wsi);
		}

		lws_service(m_lws_context, -1);
	}
}

void StandaloneUI::popup_close()
{
	if (m_clients.empty()) {
		return;
	}

	json msg;

	m_status_popup.clear();

	msg["popup"]["status"] = false;

	if ( add_message(msg) ) {
		// trigger write callback so we send this message quickly
		for (auto &client : m_clients) {
			lws_callback_on_writable(client->m_wsi);
		}

		lws_service(m_lws_context, -1);
	}
}

void StandaloneUI::mission_set_time(float mission_time)
{
	if (mission_time > 0.0f) {
		m_mission_time = static_cast<int>(SDL_roundf(mission_time));
	} else {
		m_mission_time = -1;	// disable timer in ui
	}
}

void StandaloneUI::mission_update_time()
{
	if (m_clients.empty()) {
		return;
	}

	json msg;

	msg["mission"]["time"] = m_mission_time;

	add_message(msg);
}

void StandaloneUI::mission_set_goals()
{
	if (m_clients.empty()) {
		return;
	}

	if ( !Num_goals ) {
		json msg;

		msg["mission"]["reset_goals"] = true;

		add_message(msg);
		return;
	}

	json primary;
	json secondary;
	json bonus;

	for (int i = 0; i < Num_goals; i++) {
		switch (Mission_goals[i].type & GOAL_TYPE_MASK) {
			case PRIMARY_GOAL: {
				primary.push_back({ { "name", Mission_goals[i].name }, { "status", Mission_goals[i].satisfied } });
				break;
			}

			case SECONDARY_GOAL: {
				secondary.push_back({ { "name", Mission_goals[i].name }, { "status", Mission_goals[i].satisfied } });
				break;
			}

			case BONUS_GOAL: {
				bonus.push_back({ { "name", Mission_goals[i].name }, { "status", Mission_goals[i].satisfied } });
				break;
			}

			default:
				break;
		}
	}

	json msg;

	if ( !primary.empty() ) {
		msg["mission"]["goals"]["primary"] = primary;
	}

	if ( !secondary.empty() ) {
		msg["mission"]["goals"]["secondary"] = secondary;
	}

	if ( !bonus.empty() ) {
		msg["mission"]["goals"]["bonus"] = bonus;
	}

	if ( !msg.empty() ) {
		add_message(msg);
	}
}

} // namespace <anon>


void std_deinit_standalone()
{
	Standalone_running.store(false, std::memory_order_relaxed);

	if (Standalone) {
#ifdef STD_THREADED
		if ( Standalone_thread.joinable() ) {
			Standalone_thread.join();
		}
#endif

		Standalone.reset();
	}
}

void std_init_standalone()
{
	if (Standalone) {
		return;
	}

	// turn off all sound and music
	Cmdline_freespace_no_sound = 1;
	Cmdline_freespace_no_music = 1;

	try {
		Standalone = std::make_unique<StandaloneUI>();

#ifdef STD_THREADED
		Standalone_thread = std::thread(&StandaloneUI::process, Standalone);
#endif
	} catch (std::runtime_error &e) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "** ERROR Creating standalone UI : %s", e.what());
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "** Standalone UI will not be available!");
	}

	atexit(std_deinit_standalone);
}


void std_do_gui_frame()
{
#ifndef STD_THREADED
	if (Standalone) {
		Standalone->process();
	}
#endif
}

void std_debug_set_standalone_state_string(const char *str)
{
	if (Standalone) {
		Standalone->server_set_state(str);
	}
}

void std_connect_set_gamename(const char *name)
{
	if (name == nullptr) {
		// if a	permanent name exists, use that instead of the default
		if ( SDL_strlen(Multi_options_g.std_pname) ) {
			SDL_strlcpy(Netgame.name, Multi_options_g.std_pname, SDL_arraysize(Netgame.name));
		} else {
			SDL_strlcpy(Netgame.name, XSTR("Standalone Server", 916), SDL_arraysize(Netgame.name));
		}
	} else if (name != Netgame.name) {
		SDL_strlcpy(Netgame.name, name, SDL_arraysize(Netgame.name));
	}

	if (Standalone) {
		Standalone->netgame_set_name();
	}
}

int std_connect_set_connect_count()
{
	auto count = multi_num_connections();

	if (Standalone) {
		Standalone->server_set_num_players(count);
	}

	return count;
}

void std_add_player(net_player *p)
{
	if ( !p ) {
		return;
	}

	if (Standalone) {
		Standalone->player_add(p);
	}
}

int std_remove_player(net_player *p)
{
	if ( !p ) {
		return 0;
	}

	if (Standalone) {
		Standalone->player_remove(p);
	}

	// if all players are gone then reset
	if ( !multi_num_connections() ) {
		multi_quit_game(PROMPT_NONE);
		return 1;
	}

	return 0;
}

void std_update_player_ping(net_player *p)
{
	if ( !p ) {
		return;
	}

	if (Standalone) {
		Standalone->player_update(p);
	}
}

void std_add_chat_text(const char *text, int player_index, int add_id)
{
	if ( (player_index < 0) || (player_index >= MAX_PLAYERS) ) {
		return;
	}

	if (Standalone) {
		Standalone->chat_add_text(text, player_index, add_id);
	}
}

void std_reset_timestamps()
{
	if (Standalone) {
		Standalone->reset_timestamps();
	}
}

void std_add_ban(const char *name)
{
	if ( !name || !name[0] ) {
		return;
	}

	if (Standalone_ban_list.size() >= STANDALONE_MAX_BAN) {
		return;
	}

	Standalone_ban_list.push_back(name);
}

int std_player_is_banned(const char *name)
{
	if (Standalone_ban_list.empty()) {
		return 0;
	}

	for (const auto &item : Standalone_ban_list) {
		if ( !SDL_strcasecmp(name, item.c_str()) ) {
			return 1;
		}
	}

	return 0;
}

int std_is_host_passwd()
{
	return (SDL_strlen(Multi_options_g.std_passwd) > 0) ? 1 : 0;
}

void std_multi_set_standalone_mission_name(const char *mission_name)
{
	// The "netgame" message sets and updates the mission name
	(void)mission_name;
}

void std_multi_set_standalone_missiontime(float mission_time)
{
	if (Standalone) {
		Standalone->mission_set_time(mission_time);
	}
}

void std_multi_update_netgame_info_controls()
{
	if (Standalone) {
		Standalone->netgame_update();
	}
}

void std_set_standalone_fps(float fps)
{
	if (Standalone) {
		Standalone->server_set_realized_fps(static_cast<int>(fps));
	}
}

void std_multi_setup_goal_tree()
{
	if (Standalone) {
		Standalone->mission_set_goals();
	}
}

void std_multi_add_goals()
{
	std_multi_setup_goal_tree();
}

void std_multi_update_goals()
{
	std_multi_setup_goal_tree();
}

void std_reset_standalone_gui()
{
	if (Standalone) {
		Standalone->reset_all();
	}
}

void std_create_gen_dialog(const char *title)
{
	if ( !title ) {
		return;
	}

	if (Standalone) {
		Standalone->popup(PopupTypes::Status, title);
	}
}

void std_destroy_gen_dialog()
{
	if (Standalone) {
		Standalone->popup_close();
	}
}

void std_gen_set_text(const char *str, int field_num)
{
	if ( !str ) {
		return;
	}

	if (Standalone) {
		Standalone->popup_set_text(str, field_num);
	}
}

void std_tracker_notify_login_fail()
{
	if (Standalone) {
		Standalone->popup(PopupTypes::Alert, "Error",
						  XSTR("The standalone server has failed to log in to Parallax Online!",922));
	}
}

void std_tracker_login()
{
	if ( !Multi_options_g.pxo ) {
		return;
	}

	multi_fs_tracker_init();

	if ( !multi_fs_tracker_inited() ) {
		std_tracker_notify_login_fail();
		return;
	}

	multi_fs_tracker_login_freespace();
}

void std_connect_set_host_connect_status()
{
	bool host_connected = false;

	for (const auto &player : Net_players) {
		if (MULTI_CONNECTED(player) && MULTI_HOST(player)) {
			host_connected = true;
			break;
		}
	}

	if (Standalone) {
		Standalone->server_set_host_status(host_connected);
	}
}

void std_multilog_add_line(const char *line)
{
	if ( !line || !line[0] ) {
		return;
	}

	if (Standalone) {
		Standalone->multilog_add_line(line);
	}
}

#else

void std_init_standalone(){}
void std_do_gui_frame(){}
void std_debug_set_standalone_state_string(const char *){}
void std_connect_set_gamename(const char *){}
int std_connect_set_connect_count(){return 0;}
void std_add_player(net_player *){}
int std_remove_player(net_player *){return 0;}
void std_update_player_ping(net_player *){}
void std_add_chat_text(const char *, int , int ){}
void std_reset_timestamps(){}
void std_add_ban(const char *){}
int std_player_is_banned(const char *){return 0;}
int std_is_host_passwd(){return 0;}
void std_multi_set_standalone_mission_name(const char *){}
void std_multi_set_standalone_missiontime(float mission_time){}
void std_multi_update_netgame_info_controls(){}
void std_set_standalone_fps(float ){}
void std_multi_setup_goal_tree(){}
void std_multi_add_goals(){}
void std_multi_update_goals(){}
void std_reset_standalone_gui(){}
void std_create_gen_dialog(const char *){}
void std_destroy_gen_dialog(){}
void std_gen_set_text(const char *, int ){}
void std_tracker_notify_login_fail(){}
void std_tracker_login(){}
void std_connect_set_host_connect_status(){}
void std_multilog_add_line(const char *line){};

#endif
