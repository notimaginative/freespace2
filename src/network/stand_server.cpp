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
#include "stand_server.h"
#include "multi_pmsg.h"
#include "multi_endgame.h"
#include "multimsgs.h"
#include "multiutil.h"
#include "freespace.h"
#include "missiongoals.h"
#include "cmdline.h"
#include "multi_kick.h"
#include "multi_fstracker.h"
#include "osregistry.h"
#include "standalone_html.h"

#include <libwebsockets.h>
#include <string>
#include <vector>
#include <list>


struct std_state {
	std::string title;
	std::string debug_txt;
	std::string popup_title;
	std::string popup_field1;
	std::string popup_field2;

	std::string active_player;
};

static std_state Standalone_state;

static std::list<std::string> Standalone_send_buf;


#define STANDALONE_MAX_BAN		50
static std::vector<std::string> Standalone_ban_list;

#define STD_STATS_UPDATE_TIME		500		// ms between updating player stats
#define STD_NG_UPDATE_TIME			1500	// ms between updating netgame information
#define STD_PING_UPDATE_TIME		1000	// ms between updating pings
#define STD_FPS_UPDATE_TIME			250		// ms between fps updates

static int Standalone_stats_stamp = -1;
static int Standalone_ng_stamp = -1;
static int Standalone_ping_stamp = -1;
static int Standalone_fps_stamp = -1;

static lws_context *stand_context = NULL;

static int startup_reset_stamp;
static struct lws *active_wsi = NULL;


static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + standalone_html_len + LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	unsigned char *start = p;
	unsigned char *end = p + standalone_html_len;
	bool try_reuse = false;
	int rval;
	int size;
	static unsigned int sent = 0;

	switch (reason) {
		case LWS_CALLBACK_HTTP: {
			if (len < 1) {
				lws_return_http_status(wsi, HTTP_STATUS_BAD_REQUEST, NULL);
				try_reuse = true;

				break;
			}

			// no favicon so return 404
			if ( in && !SDL_strcmp((const char *)in, "/favicon.ico") ) {
				lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);
				try_reuse = true;

				break;
			}

			// any other request will get our basic html ...

#ifndef NDEBUG
			FILE *html = fopen("./standalone.html", "rb");

			if (html) {
				fclose(html);

				rval = lws_serve_http_file(wsi, "./standalone.html", "text/html", NULL, 0);

				if ( (rval < 0) || ((rval > 0) && lws_http_transaction_completed(wsi)) ) {
					// error or can't reuse connection, close the socket
					return -1;
				}
			} else
#endif
			{
				if ( lws_add_http_header_status(wsi, 200, &p, end) ) {
					return 1;
				}

				if ( lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_SERVER, (unsigned char *)"libwebsockets", 13, &p, end) ) {
					return 1;
				}

				if ( lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE, (unsigned char *)"text/html", 9, &p, end) ) {
					return 1;
				}

				if ( lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_ENCODING, (unsigned char *)"gzip", 4, &p, end) ) {
					return 1;
				}

				if ( lws_add_http_header_content_length(wsi, standalone_html_len, &p, end) ) {
					return 1;
				}

				if ( lws_finalize_http_header(wsi, &p, end) ) {
					return 1;
				}

				rval = lws_write(wsi, start, p - start, LWS_WRITE_HTTP_HEADERS);

				if (rval != (p - start)) {
					return -1;
				}

				sent = 0;

				lws_callback_on_writable(wsi);
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

		case LWS_CALLBACK_HTTP_WRITEABLE: {
			while ( !lws_send_pipe_choked(wsi) && (sent < standalone_html_len) ) {
				size = standalone_html_len - sent;

				int pwa = lws_get_peer_write_allowance(wsi);

				if (pwa == 0) {
					lws_callback_on_writable(wsi);

					break;
				}

				if ( (pwa != -1) && (pwa < size) ) {
					size = pwa;
				}

				memcpy(p, standalone_html + sent, size);

				rval = lws_write(wsi, p, size, LWS_WRITE_HTTP);

				if (rval < 0) {
					return -1;
				}

				if (rval) {
					// while still active, extent timeout
					lws_set_timeout(wsi, PENDING_TIMEOUT_HTTP_CONTENT, 5);
				}

				sent += rval;
			}

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
	#define MAX_BUF_SIZE	1024
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_BUF_SIZE + LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	int rval;
	int size;

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED: {
			active_wsi = wsi;

			if ( timestamp_elapsed(startup_reset_stamp) ) {
				std_reset_standalone_gui();
			}

			break;
		}

		case LWS_CALLBACK_CLOSED: {
			active_wsi = NULL;

			break;
		}

		case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: {
			if (active_wsi) {
				return -1;
			}

			break;
		}

		case LWS_CALLBACK_SERVER_WRITEABLE: {
			while ( !Standalone_send_buf.empty() ) {
				size = SDL_strlcpy((char *)p, Standalone_send_buf.front().c_str(), MAX_BUF_SIZE);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending buffer!\n");
					lws_close_reason(wsi, LWS_CLOSE_STATUS_UNEXPECTED_CONDITION, (unsigned char *)"write error", 11);

					return -1;
				}

				Standalone_send_buf.pop_front();

				if ( lws_send_pipe_choked(wsi) ) {
					lws_callback_on_writable(wsi);

					break;
				}
			}

			break;
		}

		case LWS_CALLBACK_RECEIVE: {
			if (in != NULL && len > 0) {
				const char *msg = (const char *)in;
				char mtype = msg[0];

				if ( !SDL_strcmp(msg, "shutdown") ) {
					gameseq_post_event(GS_EVENT_QUIT_GAME);
					lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"shutdown", 8);

					return -1;
				}

				if ( !SDL_strcmp(msg, "reset") ) {
					multi_quit_game(PROMPT_NONE);
					std_reset_standalone_gui();

					break;
				}

				// server tab
				if (mtype == 'S') {
					if (len >= 7) {
						if ( !SDL_strncmp(msg+2, "name ", 5) ) {
							SDL_strlcpy(Netgame.name, msg+7, SDL_arraysize(Netgame.name));
							SDL_strlcpy(Multi_options_g.std_pname, Netgame.name, SDL_arraysize(Multi_options_g.std_pname));
						} else if ( !SDL_strncmp(msg+2, "pass ", 5) ) {
							SDL_strlcpy(Multi_options_g.std_passwd, msg+7, SDL_arraysize(Multi_options_g.std_passwd));
						} else if ( !SDL_strncmp(msg+2, "kick ", 5) ) {
							char ip_string[60];

							for (int i = 0; i < MAX_PLAYERS; i++) {
								if ( MULTI_CONNECTED(Net_players[i]) ) {
									psnet_addr_to_string(ip_string, SDL_arraysize(ip_string), &Net_players[i].p_info.addr);

									if ( !SDL_strcmp(msg+7, ip_string) ) {
										multi_kick_player(i, 0);

										break;
									}
								}
							}
						}
					}
				}
				// multi-player tab
				else if (mtype == 'M') {
					if (len >= 6) {
						if ( !SDL_strncmp(msg+2, "fps ", 4) ) {
							int fps = SDL_atoi(msg+6);
							CAP(fps, 10, 120);

							Multi_options_g.std_framecap = fps;
						}
					}
				}
				// player info tab
				else if (mtype == 'P') {
					if (len >= 7) {
						if ( !SDL_strncmp(msg+2, "info ", 5) ) {
							int i;

							for (i = 0; i < MAX_PLAYERS; i++) {
								net_player *np = &Net_players[i];

								if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
									if ( !SDL_strcmp(msg+7, np->player->callsign) ) {
										Standalone_state.active_player = msg+7;
										std_pinfo_display_player_info(np);

										break;
									}
								}
							}

							if (i == MAX_PLAYERS) {
								Standalone_state.active_player.clear();
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

static void std_lws_logger(int level, const char *line)
{
	if (level & (LLL_WARN|LLL_ERR)) {
		mprintf(("STD: %s", line));
	} else if (level & LLL_NOTICE) {
		nprintf(("lws", "STD: %s", line));
	}
}



static void std_add_ws_message(const char *id, const char *val)
{
	std::string msg;

	// if no client, and startup stamp elapsed, then don't add more messages
	if ( (active_wsi == NULL) && timestamp_elapsed(startup_reset_stamp) ) {
		return;
	}

	msg.assign(id);

	if (val) {
		msg.append(val);
	}

	Standalone_send_buf.push_back(msg);
}

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

	// basic security measure for admin interface
	//   "1" bind to loopback iface only *default*
	//   "0" bind to any/all
	//   any other value should be ip or iface name to bind
	//   (invalid values will trigger error)

	const char *stand_iface = os_config_read_string("Network", "RestrictStandAdmin", "1");

	if ( !SDL_strcmp(stand_iface, "1") ) {
		info.iface = "127.0.0.1";
	} else if ( !SDL_strcmp(stand_iface, "0") ) {
		info.iface = NULL;
	} else {
		info.iface = stand_iface;
	}

	info.port = Multi_options_g.port;

	info.protocols = stand_protocols;

	info.gid = -1;
	info.uid = -1;

	lws_set_log_level(LLL_ERR|LLL_WARN|LLL_NOTICE, std_lws_logger);

	stand_context = lws_create_context(&info);

	if (stand_context == NULL) {
		Error(LOCATION, "Unable to initialize standalone server!");
	}

	atexit(std_deinit_standalone);

	// turn off all sound and music
	Cmdline_freespace_no_sound = 1;
	Cmdline_freespace_no_music = 1;

	char title[64];
	SDL_snprintf(title, SDL_arraysize(title), "%s %d.%02d.%02d", XSTR("FreeSpace Standalone", 935), FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_VERSION_BUILD);
	Standalone_state.title = title;

	// connections > 5 sec after startup should get gui reset
	startup_reset_stamp = timestamp(5000);

	std_reset_standalone_gui();

	std_multi_update_netgame_info_controls();
}

static void std_update_ping_all()
{
	std::string ping_upd;
	char ping_str[10];

	for (int i = 0, idx = 0; i < MAX_PLAYERS; i++) {
		net_player *np = &Net_players[i];

		if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
			if (np->s_info.ping.ping_avg > -1) {
				if (np->s_info.ping.ping_avg >= 1000) {
					SDL_snprintf(ping_str, SDL_arraysize(ping_str), "%s", XSTR("> 1 sec", 914));
				} else {
					SDL_snprintf(ping_str, SDL_arraysize(ping_str), "%d%s", np->s_info.ping.ping_avg, XSTR(" ms", 915));
				}
			} else {
				SDL_zero(ping_str);
			}

			// append separator if not first
			if (idx++) {
				ping_upd.append(",");
			}

			ping_upd.append(ping_str);
		}
	}

	if ( !ping_upd.empty() ) {
		std_add_ws_message("S:ping ", ping_upd.c_str());
	}
}

static void std_update_connections()
{
	std::string conn_str;
	char ip_address[60];

	conn_str.reserve(1024);

	for (int i = 0, idx = 0; i < MAX_PLAYERS; i++) {
		net_player *np = &Net_players[i];

		if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
			// append seperator if not first
			if (idx++) {
				conn_str.append(";");
			}

			conn_str.append(np->player->callsign);
			conn_str.append(",");

			psnet_addr_to_string(ip_address, SDL_arraysize(ip_address), &np->p_info.addr);
			conn_str.append(ip_address);
		}
	}

	SDL_assert(conn_str.length() < 1024);

	if ( !conn_str.empty() ) {
		std_add_ws_message("S:conn ", conn_str.c_str());
	}
}

void std_do_gui_frame()
{
	// maybe update selected player stats
	if ( ((Standalone_stats_stamp == -1) || timestamp_elapsed(Standalone_stats_stamp)) && !Standalone_state.active_player.empty() ) {
		Standalone_stats_stamp = timestamp(STD_STATS_UPDATE_TIME);

		for (int i = 0; i < MAX_PLAYERS; i++) {
			net_player *np = &Net_players[i];

			if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
				if ( !SDL_strcmp(Standalone_state.active_player.c_str(), np->player->callsign) ) {
					std_pinfo_display_player_info(np);

					break;
				}
			}
		}
	}

	// maybe update netgame info
	if ( (Standalone_ng_stamp == -1) || timestamp_elapsed(Standalone_ng_stamp) ) {
		Standalone_ng_stamp = timestamp(STD_NG_UPDATE_TIME);

		std_multi_update_netgame_info_controls();
	}

	// update connection ping times
	if ( ((Standalone_ping_stamp == -1) || timestamp_elapsed(Standalone_ping_stamp)) ) {
		Standalone_ping_stamp = timestamp(STD_PING_UPDATE_TIME);

		std_update_ping_all();
	}

	if ( !Standalone_send_buf.empty() ) {
		lws_callback_on_writable_all_protocol(stand_context, &stand_protocols[1]);
	}

	lws_service(stand_context, 0);
}

void std_debug_set_standalone_state_string(const char *str)
{
	Standalone_state.debug_txt = str;

	std_add_ws_message("D:", str);
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

	std_add_ws_message("S:name ", Netgame.name);
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
	std_update_connections();

	// clear active player, client should reset if needed
	Standalone_state.active_player.clear();

	// check to see if this guy is the host
	std_connect_set_host_connect_status();
}

int std_remove_player(net_player *p)
{
	int count;

	std_update_connections();

	// clear active player, client should reset if needed
	Standalone_state.active_player.clear();

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
}

void std_pinfo_display_player_info(net_player *p)
{
	char sml_ping[30];
	std::string pinfo;

	pinfo.reserve(256);

	// ship type
	pinfo.append(Ship_info[p->p_info.ship_class].name);
	pinfo.append(";");

	// avg ping time
	if (p->s_info.ping.ping_avg > 1000) {
		SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%s", XSTR("> 1 sec", 914));
	} else {
		SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d%s", p->s_info.ping.ping_avg, XSTR(" ms", 915));
	}

	pinfo.append(sml_ping);
	pinfo.append(";");

	scoring_struct *ptr = &p->player->stats;

	// all-time stats
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_fired);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_hit);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_bonehead_hits);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_fired ? (int)(100.0f * ((float)ptr->p_shots_hit / (float)ptr->p_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->p_shots_fired ? (int)(100.0f * ((float)ptr->p_bonehead_hits / (float)ptr->p_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_fired);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_hit);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_bonehead_hits);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_fired ? (int)(100.0f * ((float)ptr->s_shots_hit / (float)ptr->s_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->s_shots_fired ? (int)(100.0f * ((float)ptr->s_bonehead_hits / (float)ptr->s_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->assists);
	pinfo.append(sml_ping);
	pinfo.append(";");	// <- end of block

	// mission stats
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_fired);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_hit);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_bonehead_hits);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_fired ? (int)(100.0f * ((float)ptr->mp_shots_hit / (float)ptr->mp_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->mp_shots_fired ? (int)(100.0f * ((float)ptr->mp_bonehead_hits / (float)ptr->mp_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_fired);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_hit);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_bonehead_hits);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_fired ? (int)(100.0f * ((float)ptr->ms_shots_hit / (float)ptr->ms_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->ms_shots_fired ? (int)(100.0f * ((float)ptr->ms_bonehead_hits / (float)ptr->ms_shots_fired)) : 0);
	pinfo.append(sml_ping);
	pinfo.append(",");
	SDL_snprintf(sml_ping, SDL_arraysize(sml_ping), "%d", ptr->m_assists);
	pinfo.append(sml_ping);

	std_add_ws_message("P:info ", pinfo.c_str());
}

void std_add_chat_text(const char *text, int player_index, int add_id)
{
	char id[32];
	std::string msg;

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

		msg.append(id);
	}

	msg.append(text);
	msg.append("\n");

	std_add_ws_message("G:mesg ", msg.c_str());
}

void std_reset_timestamps()
{
	// reset the stats update stamp
	Standalone_stats_stamp = timestamp(STD_STATS_UPDATE_TIME);

	// reset the netgame controls update timestamp
	Standalone_ng_stamp = timestamp(STD_NG_UPDATE_TIME);

	// reset the ping update stamp
	Standalone_ping_stamp = timestamp(STD_PING_UPDATE_TIME);

	// reset fps update stamp
	Standalone_fps_stamp = timestamp(STD_FPS_UPDATE_TIME);
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

void std_multi_set_standalone_mission_name(const char *mission_name)
{
	std_add_ws_message("M:name ", mission_name);
}

void std_multi_set_standalone_missiontime(float mission_time)
{
	char txt[80];
	char timestr[50];
	fix m_time = fl2f(mission_time);

	// format the time string and set the text
	game_format_time(m_time, timestr, SDL_arraysize(timestr));
	SDL_snprintf(txt, SDL_arraysize(txt), "%s  :  %.1f", timestr, mission_time);

	std_add_ws_message("M:time ", txt);
}

void std_multi_update_netgame_info_controls()
{
	char nginfo[50];

	SDL_snprintf(nginfo, SDL_arraysize(nginfo), "%d,%d,%d,%d", Netgame.max_players, Netgame.options.max_observers, Netgame.security, Netgame.respawn);

	std_add_ws_message("M:info ", nginfo);
}

void std_set_standalone_fps(float fps)
{
	if ( (Standalone_fps_stamp == -1) || timestamp_elapsed(Standalone_fps_stamp) ) {
		Standalone_fps_stamp = timestamp(STD_FPS_UPDATE_TIME);

		char rfps[10];

		SDL_snprintf(rfps, SDL_arraysize(rfps), "%.1f", fps);

		std_add_ws_message("M:rfps ", rfps);
	}
}

void std_multi_setup_goal_tree()
{
	std::string mission_goals;
	std::string primary;
	std::string secondary;
	std::string bonus;
	std::string status;

	for (int i = 0; i < Num_goals; i++) {
		switch (Mission_goals[i].satisfied) {
			case GOAL_FAILED: {
				status = "f ";
				break;
			}

			case GOAL_COMPLETE: {
				status = "c ";
				break;
			}

			case GOAL_INCOMPLETE:
			default: {
				status = "i ";
				break;
			}
		}

		switch (Mission_goals[i].type & GOAL_TYPE_MASK) {
			case PRIMARY_GOAL: {
				primary.append(status);
				primary.append(Mission_goals[i].name);
				primary.append(",");

				break;
			}

			case SECONDARY_GOAL: {
				secondary.append(status);
				secondary.append(Mission_goals[i].name);
				secondary.append(",");

				break;
			}

			case BONUS_GOAL: {
				bonus.append(status);
				bonus.append(Mission_goals[i].name);
				bonus.append(",");

				break;
			}

			default:
				break;
		}
	}

	if ( primary.empty() ) {
		mission_goals.append("i none");
	} else {
		mission_goals.append(primary.substr(0, primary.size()-1));
	}

	mission_goals.append(";");

	if ( secondary.empty() ) {
		mission_goals.append("i none");
	} else {
		mission_goals.append(secondary.substr(0, secondary.size()-1));
	}

	mission_goals.append(";");

	if ( bonus.empty() ) {
		mission_goals.append("i none");
	} else {
		mission_goals.append(bonus.substr(0, bonus.size()-1));
	}

	std_add_ws_message("M:goal ", mission_goals.c_str());
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
	Standalone_send_buf.clear();

	std_add_ws_message("reset", NULL);

	std_add_ws_message("T:", Standalone_state.title.c_str());
	std_add_ws_message("D:", Standalone_state.debug_txt.c_str());

	std_add_ws_message("S:name ", Netgame.name);
	std_add_ws_message("S:pass ", Multi_options_g.std_passwd);

	std_update_connections();
	std_set_standalone_fps(0.0f);
	std_multi_set_standalone_missiontime(0.0f);
	std_multi_update_netgame_info_controls();

	Standalone_fps_stamp = -1;
	Standalone_ng_stamp = -1;
	Standalone_ping_stamp = -1;
	Standalone_stats_stamp = -1;

	Standalone_state.active_player.clear();
}


void std_create_gen_dialog(const char *title)
{
	Standalone_state.popup_title = title;

	Standalone_state.popup_field1 = "";
	Standalone_state.popup_field2 = "";
}

void std_destroy_gen_dialog()
{
	std_add_ws_message("popup ", NULL);
}

void std_gen_set_text(const char *str, int field_num)
{
	std::string popup_str;

	switch (field_num) {
		case 0:
			Standalone_state.popup_title = str;
			break;

		case 1:
			Standalone_state.popup_field1 = str;
			break;

		case 2:
			Standalone_state.popup_field2 = str;
			break;

		default:
			break;
	}

	popup_str.append(Standalone_state.popup_title);
	popup_str.append(";");
	popup_str.append(Standalone_state.popup_field1);
	popup_str.append(";");
	popup_str.append(Standalone_state.popup_field2);

	std_add_ws_message("popup ", popup_str.c_str());

	// force ws write since do_frame() may not happen until popup is done
	lws_callback_on_writable_all_protocol(stand_context, &stand_protocols[1]);
	lws_service(stand_context, 0);
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
