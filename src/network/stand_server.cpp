/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/


#include "pstypes.h"
#include "osregistry.h"
#include "multi_options.h"
#include "gamesequence.h"
#include "timer.h"
#include "version.h"
#include "multi.h"
#include "stand_gui.h"
#include "multi_pmsg.h"
#include "multi_endgame.h"
#include "multimsgs.h"
#include "multiutil.h"

#include <libwebsockets.h>
#include <string>
#include <vector>



static std::string Standalone_debug_state;
static std::string Standalone_ping_str;
static std::string Standalone_player_info;
static std::string Standalone_message;

#define STANDALONE_MAX_BAN		50
static std::vector<std::string> Standalone_ban_list;

#define STD_STATS_UPDATE_TIME		500		// ms between updating player stats
#define STD_NG_UPDATE_TIME			100		// ms between updating netgame information
#define STD_PING_UPDATE_TIME		1000	// ms between updating pings

static int Standalone_stats_stamp = -1;
static int Standalone_ng_stamp = -1;
static int Standalone_ping_stamp = -1;

static int Standalone_update_flags = 0;

#define STD_UFLAG_DEBUG_STATE		(1<<0)
#define STD_UFLAG_TITLE				(1<<1)
#define STD_UFLAG_CONN				(1<<2)

#define STD_UFLAG_SERVER_NAME		(1<<3)
#define STD_UFLAG_HOST_PASS			(1<<4)
#define STD_UFLAG_SET_PING			(1<<5)

#define STD_UFLAG_PLAYER_INFO		(1<<6)

#define STD_UFLAG_S_MESSAGE			(1<<7)

#define STD_UFLAG_GENERAL			(STD_UFLAG_DEBUG_STATE|STD_UFLAG_TITLE)
#define STD_UFLAG_TAB_SERVER		(STD_UFLAG_SERVER_NAME|STD_UFLAG_HOST_PASS|STD_UFLAG_CONN|STD_UFLAG_SET_PING)
#define STD_UFLAG_TAB_PLAYER		(STD_UFLAG_PLAYER_INFO)
#define STD_UFLAG_TAB_GS			(STD_UFLAG_S_MESSAGE)

#define STD_UFLAG_ALL				(STD_UFLAG_GENERAL|STD_UFLAG_TAB_SERVER|STD_UFLAG_TAB_PLAYER)


static lws_context *stand_context = NULL;


static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	bool try_reuse = false;

	switch (reason) {
		case LWS_CALLBACK_HTTP: {
			if (len < 1) {
				lws_return_http_status(wsi, HTTP_STATUS_BAD_REQUEST, NULL);
				try_reuse = true;

				break;
			}

			int	ret = lws_serve_http_file(wsi, "./standalone.html", "text/html", NULL, 0);

			if ( (ret < 0) || ((ret > 0) && lws_http_transaction_completed(wsi)) ) {
				// error or can't reuse connection, close the socket
				return -1;
			}

			break;
		}

		case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
			lws_return_http_status(wsi, HTTP_STATUS_OK, NULL);
			try_reuse = true;

			break;
		}

		case LWS_CALLBACK_HTTP_FILE_COMPLETION: {
			try_reuse = true;

			break;
		}

		default:
			break;
	}

	if (try_reuse) {
		if (lws_http_transaction_completed(wsi)) {
			return -1;
		}
	}

	return 0;
}

static int callback_standalone(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	#define MAX_BUF_SIZE	1050
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_BUF_SIZE + LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	int rval;
	int size;

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED: {
			Standalone_stats_stamp = -1;
			Standalone_ng_stamp = -1;

			Standalone_update_flags = STD_UFLAG_ALL;

			break;
		}

		case LWS_CALLBACK_SERVER_WRITEABLE: {
			if (Standalone_update_flags & STD_UFLAG_TITLE) {
				size = SDL_snprintf((char *)p, 64, "T:%s %d.%02d.%02d", XSTR("FreeSpace Standalone", 935), FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_VERSION_BUILD);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending title string!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_TITLE;
			}

			if (Standalone_update_flags & STD_UFLAG_DEBUG_STATE) {
				size = SDL_snprintf((char *)p, 32, "D:%s", Standalone_debug_state.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_DEBUG_STATE;
			}

			if (Standalone_update_flags & STD_UFLAG_SERVER_NAME) {
				size = SDL_snprintf((char *)p, MAX_GAMENAME_LEN, "S:name %s", Netgame.name);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_SERVER_NAME;
			}

			if (Standalone_update_flags & STD_UFLAG_HOST_PASS) {
				size = SDL_snprintf((char *)p, STD_PASSWD_LEN, "S:pass %s", Multi_options_g.std_passwd);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_HOST_PASS;
			}

			if (Standalone_update_flags & STD_UFLAG_CONN) {
				std::string conn_str;
				char ip_address[60];

				conn_str.reserve(1024);

				for (int i = 0; i < MAX_PLAYERS; i++) {
					net_player *np = &Net_players[i];

					if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
						conn_str.append(np->player->callsign);
						conn_str.append(",");

						psnet_addr_to_string(ip_address, SDL_arraysize(ip_address), &np->p_info.addr);
						conn_str.append(ip_address);
						conn_str.append(",");

						if (np->s_info.ping.ping_avg > -1) {
							if (np->s_info.ping.ping_avg >= 1000) {
								SDL_snprintf(ip_address, SDL_arraysize(ip_address), "%s", XSTR("> 1 sec", 914));
							} else {
								SDL_snprintf(ip_address, SDL_arraysize(ip_address), "%d%s", np->s_info.ping.ping_avg, XSTR(" ms", 915));
							}
						}

						conn_str.append(";");
					}
				}

				SDL_assert(conn_str.length() < 1024);

				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "S:conn %s", conn_str.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_CONN;
			}

			if ( (Standalone_update_flags & STD_UFLAG_SET_PING) && !Standalone_ping_str.empty() ) {
				SDL_assert(Standalone_ping_str.length() < 1024);

				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "S:ping %s", Standalone_ping_str.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_ping_str.clear();
				Standalone_update_flags &= ~ STD_UFLAG_SET_PING;
			}

			if ( (Standalone_update_flags & STD_UFLAG_PLAYER_INFO) && !Standalone_player_info.empty() ) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "P:info %s", Standalone_player_info.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_player_info.clear();
				Standalone_update_flags &= ~STD_UFLAG_PLAYER_INFO;
			}

			if ( (Standalone_update_flags & STD_UFLAG_S_MESSAGE) && !Standalone_message.empty() ) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "G:mesg %s", Standalone_message.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_message.clear();
				Standalone_update_flags &= ~STD_UFLAG_S_MESSAGE;
			}

			break;
		}

		case LWS_CALLBACK_RECEIVE: {
			if (in != NULL && len > 0) {
				const char *msg = (const char *)in;
				char mtype = msg[0];

				if ( !SDL_strcmp(msg, "shutdown") ) {
					gameseq_post_event(GS_EVENT_QUIT_GAME);
					return -1;
				}

				// server tab
				if (mtype == 'S') {
					if (len >= 7) {
						if ( !SDL_strncmp(msg+2, "name ", 5) ) {
							SDL_strlcpy(Netgame.name, msg+7, SDL_arraysize(Netgame.name));
							SDL_strlcpy(Multi_options_g.std_pname, Netgame.name, SDL_arraysize(Multi_options_g.std_pname));
						} else if ( !SDL_strncmp(msg+2, "pass ", 5) ) {
							SDL_strlcpy(Multi_options_g.std_passwd, msg+7, SDL_arraysize(Multi_options_g.std_passwd));
						}
					}
				}
				// multi-player tab
				else if (mtype == 'M') {

				}
				// player info tab
				else if (mtype == 'P') {
					if (len >= 7) {
						if ( !SDL_strncmp(msg+2, "info ", 5) ) {
							for (int i = 0; i < MAX_PLAYERS; i++) {
								net_player *np = &Net_players[i];

								if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
									if ( !SDL_strcmp(msg+7, np->player->callsign) ) {
										std_pinfo_display_player_info(np);

										break;
									}
								}
							}
						}
					}
				}
				// god stuff tab
				else if (mtype == 'G') {
					if (len >= 7) {
						if ( !SDL_strncmp(msg+2, "smsg ", 5) ) {
							char txt[256];

							SDL_strlcpy(txt, msg+7, SDL_arraysize(txt));

							if (SDL_strlen(txt) > 0) {
								send_game_chat_packet(Net_player, txt, MULTI_MSG_ALL, NULL);

								std_add_chat_text(txt, MY_NET_PLAYER_NUM, 1);
							}
						} else if ( !SDL_strcmp(msg+2, "mrefresh") ) {
							if (MULTI_IS_TRACKER_GAME) {
								cf_delete(MULTI_VALID_MISSION_FILE, CF_TYPE_DATA);

								multi_update_valid_missions();
							}
						}
					}
				}
			}

			break;
		}

		default:
			break;
	}

	return 0;
}

static struct lws_protocols stand_protocols[] = {
	{
		"http-only",
		callback_http,
		0,
		0
	},
	{
		"standalone",
		callback_standalone,
		0,
		0
	},
	// terminator
	{
		NULL,
		NULL,
		0,
		0
	}
};




void std_deinit_standalone()
{
	if (stand_context) {
		lws_cancel_service(stand_context);
		lws_context_destroy(stand_context);
		stand_context = NULL;
	}
}

void std_init_standalone()
{
	struct lws_context_creation_info info;

	if (stand_context) {
		return;
	}

	SDL_zero(info);

	info.port = Multi_options_g.port;

	info.protocols = stand_protocols;

	info.gid = -1;
	info.uid = -1;

	info.ka_time = 0;
	info.ka_probes = 0;
	info.ka_interval = 0;

	stand_context = lws_create_context(&info);

	if (stand_context == NULL) {
		SDL_assert_always(1);
	}

	Standalone_stats_stamp = -1;
	Standalone_ng_stamp = -1;
	Standalone_ping_stamp = -1;

	Standalone_update_flags = STD_UFLAG_ALL;
}

void std_do_gui_frame()
{
	// maybe update selected player stats
	if ( (Standalone_stats_stamp == -1) || timestamp_elapsed(Standalone_stats_stamp) ) {
		Standalone_stats_stamp = timestamp(STD_STATS_UPDATE_TIME);
	}

	// maybe update netgame info
	if ( (Standalone_ng_stamp == -1) || timestamp_elapsed(Standalone_ng_stamp) ) {
		Standalone_ng_stamp = timestamp(STD_NG_UPDATE_TIME);
	}

	// update connection ping times
	if ( !Standalone_ping_str.empty() && ((Standalone_ping_stamp == -1) || timestamp_elapsed(Standalone_ping_stamp)) ) {
		Standalone_ping_stamp = timestamp(STD_PING_UPDATE_TIME);
		Standalone_update_flags |= STD_UFLAG_SET_PING;
	}

	if (Standalone_update_flags) {
		lws_callback_on_writable_all_protocol(stand_context, &stand_protocols[1]);
	}

	lws_service(stand_context, 0);
}

void std_debug_set_standalone_state_string(const char *str)
{
	Standalone_debug_state = str;

	Standalone_update_flags |= STD_UFLAG_DEBUG_STATE;
}

void std_connect_set_gamename(const char *name)
{
	if (name == NULL) {
		// if a	permanent name exists, use that instead of the default
		if ( SDL_strlen(Multi_options_g.std_pname) ) {
			SDL_strlcpy(Netgame.name, Multi_options_g.std_pname, SDL_arraysize(Netgame.name));
		} else {
			SDL_strlcpy(Netgame.name, XSTR("Standalone Server", 916), SDL_arraysize(Netgame.name));
		}
	} else {
		SDL_strlcpy(Netgame.name, name, SDL_arraysize(Netgame.name));
	}

	Standalone_update_flags |= STD_UFLAG_SERVER_NAME;
}

int std_connect_set_connect_count()
{
	int count = 0;

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (MULTI_CONNECTED(Net_players[i]) && (Net_player != &Net_players[i]) ) {
			count++;
		}
	}

	return count;
}

void std_add_player(net_player *p)
{
	Standalone_update_flags |= STD_UFLAG_CONN;

	// check to see if this guy is the host
	std_connect_set_host_connect_status();
}

int std_remove_player(net_player *p)
{
	int count;

	Standalone_update_flags |= STD_UFLAG_CONN;

	// update the host connect count
	std_connect_set_host_connect_status();

	// update the currently connected players
	count = std_connect_set_connect_count();

	if (count == 0) {
		multi_quit_game(PROMPT_NONE);
		return 1;
	}

	return 0;
}

void std_update_player_ping(net_player *p)
{
	char ip_address[60];

	if (p->s_info.ping.ping_avg > -1) {
		psnet_addr_to_string(ip_address, SDL_arraysize(ip_address), &p->p_info.addr);

		Standalone_ping_str.append(ip_address);

		if (p->s_info.ping.ping_avg > 1000) {
			SDL_snprintf(ip_address, SDL_arraysize(ip_address), ",%s;", XSTR("> 1 sec", 914));
		} else {
			SDL_snprintf(ip_address, SDL_arraysize(ip_address), ",%d%s;", p->s_info.ping.ping_avg, XSTR(" ms", 915));
		}

		Standalone_ping_str.append(ip_address);
	}
}

void std_pinfo_display_player_info(net_player *p)
{
	char sml_ping[30];

	Standalone_player_info.clear();
	Standalone_player_info.reserve(256);

	// ship type
	Standalone_player_info.append(Ship_info[p->p_info.ship_class].name);
	Standalone_player_info.append(";");

	// avg ping time
	if (p->s_info.ping.ping_avg > 1000) {
		SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%s", XSTR("> 1 sec", 914));
	} else {
		SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d%s", p->s_info.ping.ping_avg, XSTR(" ms", 915));
	}

	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(";");

	scoring_struct *ptr = &p->player->stats;

	// all-time stats
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_fired);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_hit);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_bonehead_hits);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_fired ? (int)(100.0f * ((float)ptr->p_shots_hit / (float)ptr->p_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_fired ? (int)(100.0f * ((float)ptr->p_bonehead_hits / (float)ptr->p_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_fired);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_hit);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_bonehead_hits);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_fired ? (int)(100.0f * ((float)ptr->s_shots_hit / (float)ptr->s_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_fired ? (int)(100.0f * ((float)ptr->s_bonehead_hits / (float)ptr->s_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->assists);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(";");	// <- end of block

	// mission stats
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_fired);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_hit);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_bonehead_hits);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_fired ? (int)(100.0f * ((float)ptr->mp_shots_hit / (float)ptr->mp_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_fired ? (int)(100.0f * ((float)ptr->mp_bonehead_hits / (float)ptr->mp_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_fired);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_hit);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_bonehead_hits);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_fired ? (int)(100.0f * ((float)ptr->ms_shots_hit / (float)ptr->ms_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_fired ? (int)(100.0f * ((float)ptr->ms_bonehead_hits / (float)ptr->ms_shots_fired)) : 0);
	Standalone_player_info.append(sml_ping);
	Standalone_player_info.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->m_assists);
	Standalone_player_info.append(sml_ping);

	Standalone_update_flags |= STD_UFLAG_PLAYER_INFO;
}

void std_add_chat_text(const char *text, int player_index, int add_id)
{
	char id[32];

	if ( (player_index < 0) || (player_index >= MAX_PLAYERS) ) {
		return;
	}

	// format the chat text nicely
	if (add_id) {
		if ( MULTI_STANDALONE(Net_players[player_index]) ) {
			SDL_snprintf(id, SDL_arraysize(id), XSTR("<SERVER> %s", 924), "");
		} else {
			SDL_snprintf(id, SDL_arraysize(id), "%s: ", Net_players[player_index].player->callsign);
		}

		Standalone_message.append(id);
	}

	Standalone_message.append(text);
	Standalone_message.append("\n");

	Standalone_update_flags |= STD_UFLAG_S_MESSAGE;
}

void std_reset_timestamps()
{
	// reset the stats update stamp
	Standalone_stats_stamp = timestamp(STD_STATS_UPDATE_TIME);

	// reset the netgame controls update timestamp
	Standalone_ng_stamp = timestamp(STD_NG_UPDATE_TIME);

	// reset the ping update stamp
	Standalone_ping_stamp = timestamp(STD_PING_UPDATE_TIME);
}

void std_add_ban(const char *name)
{
	if ( (name == NULL) || !SDL_strlen(name) ) {
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

















int std_pinfo_maybe_update_player_info(net_player *p)
{
	STUB_FUNCTION;

	return 0;
}

void std_connect_set_host_connect_status()
{
	STUB_FUNCTION;
}

void std_create_gen_dialog(const char *title)
{
	STUB_FUNCTION;
}

void std_destroy_gen_dialog()
{
	STUB_FUNCTION;
}

void std_gen_set_text(const char *str, int field_num)
{
	STUB_FUNCTION;
}

void std_multi_add_goals()
{
	STUB_FUNCTION;
}

void std_multi_set_standalone_mission_name(const char *mission_name)
{
	STUB_FUNCTION;
}

void std_multi_set_standalone_missiontime(float mission_time)
{
	STUB_FUNCTION;
}

void std_multi_setup_goal_tree()
{
	STUB_FUNCTION;
}

void std_multi_update_goals()
{
	STUB_FUNCTION;
}

void std_multi_update_netgame_info_controls()
{
	STUB_FUNCTION;
}

void std_reset_standalone_gui()
{
	STUB_FUNCTION;
}

void std_set_standalone_fps(float fps)
{
	STUB_FUNCTION;
}
