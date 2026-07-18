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

#include "ext/json.hpp"

#include <libwebsockets.h>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <atomic>

#include <iostream>

using json = nlohmann::json;

// Define this to use standalone ui in a seperate thread
//#define STD_THREADED

#ifdef STD_THREADED
#include <thread>

static std::thread Standalone_thread;
#endif

#define STANDALONE_MAX_BAN		50
static std::vector<std::string> Standalone_ban_list;

enum UpdateTimes {
	stats			= 1500,
	netgame			= 2500,
	mission_time	= 10000,
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

	uint64_t m_stats_timestamp;
	uint64_t m_netgame_timestamp;
	uint64_t m_mission_time_timestamp;

	Standalone_client(uint32_t id, struct lws *wsi) :
		m_id(id), m_wsi(wsi), m_active_player(-1), m_multilog_enabled(true),
		m_stats_timestamp(0), m_netgame_timestamp(0), m_mission_time_timestamp(0)
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

	struct lws_http_mount *m_lws_mounts;

	std::string m_title;
	std::string m_state_text;
	json m_popup;

	uint32_t m_next_client_id;

	float m_mission_time;

	static constexpr size_t MAX_MULTILOG_LINES = 100;
	std::deque<std::string> m_multilog;

	static constexpr size_t MAX_CHATLOG_LINES = 50;
	std::deque<chatlog_item> m_chatlog;

	static constexpr size_t MAX_STD_CLIENTS = 5;
	std::list<Standalone_client *> m_clients;

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

	void popup_open(const char *title);
	void popup_set_text(const char *str, int field_num);
	void popup_close();

	void mission_set_time(float mission_time);
	void mission_update_time();
	void mission_set_goals();
};


static StandaloneUI *Standalone = nullptr;
static std::atomic<bool> Standalone_terminate(false);

static void std_lws_logger(int level, const char *line)
{
	if (level & LLL_WARN) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "StandaloneUI: %s", line);
	} else if (level & LLL_ERR) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "StandaloneUI: %s", line);
	} else if (level & LLL_NOTICE) {
		nprintf(("lws", "STD: %s", line));
	}
}


StandaloneUI::StandaloneUI()
{
	struct lws_context_creation_info info;

	SDL_zero(info);

	// only needs the single mount
	m_lws_mounts = new struct lws_http_mount;

	SDL_zerop(m_lws_mounts);

	m_lws_mounts->mountpoint = "/";
	m_lws_mounts->mountpoint_len = 1;
	m_lws_mounts->origin = "./standalone-web";
	m_lws_mounts->origin_protocol = LWSMPRO_FILE;
	m_lws_mounts->def = "index.html";

	if ( !SDL_strlen(Multi_options_g.std_listen_addr) ) {
		// this option is to get around a libwebsockets bug that prevented binding
		// to a IPv4 iface address properly
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
	info.mounts = m_lws_mounts;

	info.gid = static_cast<gid_t>(-1);
	info.uid = static_cast<uid_t>(-1);

	lws_set_log_level(LLL_ERR|LLL_WARN|LLL_NOTICE, std_lws_logger);

	m_lws_context = lws_create_context(&info);

	if (m_lws_context == nullptr) {
		Error(LOCATION, "Unable to initialize standalone server!");
	}

	m_start_time = time(nullptr);

	char title[64];
	SDL_snprintf(title, SDL_arraysize(title), "%s %d.%02d.%02d", XSTR("FreeSpace Standalone", 935), FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_VERSION_BUILD);
	m_title = title;

	m_next_client_id = 1;

	m_active_client = nullptr;

	m_mission_time = 0.0f;
}

StandaloneUI::~StandaloneUI()
{
	shutdown();
}

int StandaloneUI::callback_standalone(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	#define MAX_BUF_SIZE	2048
	unsigned char buf[LWS_PRE + MAX_BUF_SIZE];
	unsigned char *p = &buf[LWS_PRE];
	int exit_val = 0;

	uint32_t *client_id = reinterpret_cast<uint32_t *>(user);

	if (client_id && *client_id) {
		for (auto &cl : m_clients) {
			if (cl->m_id == *client_id) {
				m_active_client = cl;
				break;
			}
		}
	}

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED: {
			SDL_assert(m_active_client == nullptr);

			uint32_t cid = getNewClientId();

			Standalone_client *new_client = new Standalone_client(cid, wsi);
			m_clients.push_back(new_client);

			*client_id = cid;
			m_active_client = new_client;

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
					lwsl_warn("Message size (%zu) exceeds buffer size (%d)!  Discarding...\n", m_active_client->m_send_buffer.size(), MAX_BUF_SIZE);
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

				if ( msg.find("shutdown") != msg.end() ) {
					lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"shutdown", 8);
					gameseq_post_event(GS_EVENT_QUIT_GAME);
					Standalone_terminate = true;

					exit_val = -1;
					break;
				}

				if ( msg.find("reset_all") != msg.end() ) {
					multi_quit_game(PROMPT_NONE);
					reset_all();

					break;
				}

				// name
				auto name_key = msg.find("server_name");

				if ( name_key != msg.end() ) {
					std::string name = *name_key;

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
				auto password_key = msg.find("server_password");

				if ( password_key != msg.end() ) {
					std::string pass = *password_key;
					SDL_strlcpy(Multi_options_g.std_passwd, pass.c_str(), SDL_arraysize(Multi_options_g.std_passwd));
				}

				// allow voice
				auto voice_key = msg.find("server_voice");

				if ( voice_key != msg.end() ) {
					bool voice = *voice_key;
					Multi_options_g.std_voice = voice ? 1 : 0;
				}

				// server update rate
				auto update_rate_key = msg.find("server_update_rate");

				if ( update_rate_key != msg.end() ) {
					int obj_update = *update_rate_key;

					if ( (obj_update >= 0) && (obj_update < MAX_OBJ_UPDATE_LEVELS) ) {
						Multi_options_g.std_datarate = obj_update;
						Net_player->p_info.options.obj_update_level = obj_update;
					}
				}

				// max players
				auto max_players_key = msg.find("server_max_players");

				if ( max_players_key != msg.end() ) {
					int max_players = *max_players_key;

					if ( (max_players == -1) || !((max_players < 1) || (max_players > MAX_PLAYERS)) ) {
						Multi_options_g.std_max_players = max_players;
					}
				}

				// kick player
				auto kick_key = msg.find("player_kick");

				if ( kick_key != msg.end() ) {
					short player_id = *kick_key;
					int idx = find_player_id(player_id);

					multi_kick_player(idx, 0);
				}

				// player info/stats
				auto info_key = msg.find("player_info");

				if ( info_key != msg.end() ) {
					short player_id = *info_key;
					int idx = find_player_id(player_id);

					if (idx >= 0) {
						player_info(&Net_players[idx]);
						m_active_client->m_active_player = player_id;
					} else {
						m_active_client->m_active_player = -1;
					}
				}

				// fps
				auto fps_key = msg.find("fps");

				if ( fps_key != msg.end() ) {
					int fps = *fps_key;
					CAP(fps, 10, 120);

					Multi_options_g.std_framecap = fps;
				}

				// chat
				auto chat_key = msg.find("chat");

				if ( chat_key != msg.end() ) {
					std::string txt = *chat_key;

					if ( !txt.empty() ) {
						send_game_chat_packet(Net_player, txt.c_str(), MULTI_MSG_ALL, nullptr);

						std_add_chat_text(txt.c_str(), MY_NET_PLAYER_NUM, 1);
					}
				}

				// revalidate missions/tables
				if ( msg.find("validate") != msg.end() ) {
					cf_delete(MULTI_VALID_MISSION_FILE, CF_TYPE_DATA);

					multi_update_valid_missions();
				}

				// enable/disable sending of multi log (per client)
				auto multilog_key = msg.find("multilog");

				if ( multilog_key != msg.end() ) {
					bool enabled = *multilog_key;

					m_active_client->m_multilog_enabled = enabled ? true : false;

					// if we are enabling the multilog then send all that we have to the client
					multilog_refresh();
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
	for (int i = 0; i < MAX_PLAYERS; i++) {
		net_player *np = &Net_players[i];

		if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
			player_add(np);
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
	do {
		do_frame();

		lws_service(m_lws_context, -1);

		SDL_Delay(1000/30);
	} while ( !Standalone_terminate );

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
		m_active_client = client;

		// maybe update netgame info
		if (multi_num_connections()) {
			if ( !client->m_netgame_timestamp || (cur_time_ms > client->m_netgame_timestamp) ) {
				client->m_netgame_timestamp = cur_time_ms + UpdateTimes::netgame;

				netgame_update();
			}
		}

		// maybe update mission time
		if (m_mission_time != 0.0f) {
			if ( !client->m_mission_time_timestamp || (cur_time_ms > client->m_mission_time_timestamp) ) {
				client->m_mission_time_timestamp = cur_time_ms + UpdateTimes::mission_time;

				mission_update_time();
			}
		}

		// maybe update selected player stats
		if (client->m_active_player != -1) {
			if ( !client->m_stats_timestamp || (cur_time_ms > client->m_stats_timestamp) ) {
				client->m_stats_timestamp = cur_time_ms + UpdateTimes::stats;

				int player_idx = find_player_id(client->m_active_player);

				if (player_idx >= 0) {
					player_info(&Net_players[player_idx]);
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

	json msg;

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
	msg["server"]["pxo"] = Multi_options_g.pxo;
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
			client->m_stats_timestamp = 0;
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

	m_chatlog.emplace_back(std::move(item));

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

	for (auto &item : m_chatlog) {
		chat_send_text(item.id, item.message);
	}
}

void StandaloneUI::reset_all()
{
	if (m_clients.empty()) {
		return;
	}

	// we should clear chat log here and start fresh
	m_chatlog.clear();
	m_chatlog.shrink_to_fit();

	Standalone_client *prev_client = m_active_client;

	for (auto &client : m_clients) {
		m_active_client = client;
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
	std::string build;

	// send gui reset
	msg["reset_gui"] = true;

	add_message(msg);

	// basic server information
	msg.clear();
	msg["server_info"]["address"] = m_interface;
	msg["server_info"]["start_time"] = m_start_time;
	msg["server_info"]["title"] = m_title;
	msg["server_info"]["state"] = m_state_text;
	msg["server_info"]["multi_version"] = MULTI_FS_SERVER_VERSION;

	build = Osreg_title;
	build += " ";
	build += version_get_string_full();

	msg["server_info"]["build"] = build.c_str();

	add_message(msg);

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
	if ( !m_popup.empty() ) {
		msg.clear();
		msg["popup"] = m_popup;

		add_message(msg);
	}

	// refresh client-side state
	if (m_active_client) {
		m_active_client->m_netgame_timestamp = 0;
		m_active_client->m_stats_timestamp = 0;
		m_active_client->m_active_player = -1;
	}
}

void StandaloneUI::reset_timestamps()
{
	for (auto &client : m_clients) {
		client->m_netgame_timestamp = 0;
		client->m_stats_timestamp = 0;
	}
}

void StandaloneUI::multilog_add_line(const char *line)
{
	SDL_assert(line);

	m_multilog.push_back(line);

	if (m_multilog.size() > MAX_MULTILOG_LINES) {
		m_multilog.pop_front();
	}

	if (m_clients.empty()) {
		return;
	}

	json msg;

	auto prev_client = m_active_client;

	for (auto &client : m_clients) {
		m_active_client = client;

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

	for (auto &line : m_multilog) {
		msg["multilog"] = line;

		add_message(msg);
	}
}

void StandaloneUI::popup_open(const char *title)
{
	SDL_assert(title);

	if (m_clients.empty()) {
		return;
	}

	m_popup["title"] = title;

	m_popup["field1"] = "";
	m_popup["field2"] = "";

	json msg;

	msg["popup"] = m_popup;

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
			m_popup["title"] = str;
			break;

		case 1:
			m_popup["field1"] = str;
			break;

		case 2:
			m_popup["field2"] = str;
			break;

		default:
			return;
	}


	json msg;

	msg["popup"] = m_popup;

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

	json msg = {{ "popup", false }};

	m_popup.clear();

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
	m_mission_time = mission_time;
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


void std_deinit_standalone()
{
	Standalone_terminate = true;

#ifdef STD_THREADED
	if ( Standalone_thread.joinable() ) {
		Standalone_thread.join();
	}
#endif

	if (Standalone) {
		delete Standalone;
		Standalone = nullptr;
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

	Standalone = new StandaloneUI();

#ifdef STD_THREADED
	Standalone_thread = std::thread(&StandaloneUI::process, Standalone);
#endif

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
	if ( !Standalone ) {
		return;
	}

	Standalone->server_set_state(str);
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

	if ( !Standalone ) {
		return;
	}

	Standalone->netgame_set_name();
}

int std_connect_set_connect_count()
{
	return 0;
}

void std_add_player(net_player *p)
{
	if ( !p || !Standalone ) {
		return;
	}

	Standalone->player_add(p);
}

int std_remove_player(net_player *p)
{
	if ( !p || !Standalone ) {
		return 0;
	}

	Standalone->player_remove(p);

	// if all players are gone then reset
	if ( !multi_num_connections() ) {
		multi_quit_game(PROMPT_NONE);
		return 1;
	}

	return 0;
}

void std_update_player_ping(net_player *p)
{
	if ( !p || !Standalone ) {
		return;
	}

	Standalone->player_update(p);
}

void std_add_chat_text(const char *text, int player_index, int add_id)
{
	if ( (player_index < 0) || (player_index >= MAX_PLAYERS) ) {
		return;
	}

	if ( !Standalone ) {
		return;
	}

	Standalone->chat_add_text(text, player_index, add_id);
}

void std_reset_timestamps()
{
	if ( !Standalone ) {
		return;
	}

	Standalone->reset_timestamps();
}

void std_add_ban(const char *name)
{
	if ( (name == nullptr) || !SDL_strlen(name) ) {
		return;
	}

	if (Standalone_ban_list.size() >= STANDALONE_MAX_BAN) {
		return;
	}

	Standalone_ban_list.push_back(name);
}

int std_player_is_banned(const char *name)
{
	if ( Standalone_ban_list.empty() ) {
		return 0;
	}

	for (size_t i = 0; i < Standalone_ban_list.size(); i++) {
		if ( !SDL_strcasecmp(name, Standalone_ban_list[i].c_str()) ) {
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
	if ( !Standalone ) {
		return;
	}

	Standalone->reset_all();
}

void std_create_gen_dialog(const char *title)
{
	if ( !title || !Standalone ) {
		return;
	}

	Standalone->popup_open(title);
}

void std_destroy_gen_dialog()
{
	if (Standalone) {
		Standalone->popup_close();
	}
}

void std_gen_set_text(const char *str, int field_num)
{
	if ( !str || !Standalone ) {
		return;
	}

	Standalone->popup_set_text(str, field_num);
}

void std_tracker_notify_login_fail()
{
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
}

void std_multilog_add_line(const char *line)
{
	if ( !line || !line[0] ) {
		return;
	}

	if ( !Standalone ) {
		return;
	}

	Standalone->multilog_add_line(line);
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
