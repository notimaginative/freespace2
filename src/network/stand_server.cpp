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

#include <libwebsockets.h>
#include <string>
#include <vector>



static std::string Standalone_debug_state = "";
static std::string Standalone_ping_str;
static std::string Standalone_player_info;
static std::string Standalone_message;
static std::string Standalone_pinfo_active_player;
static std::string Standalone_mission_name = "";
static std::string Standalone_mission_time = "";
static std::string Standalone_netgame_info;
static std::string Standalone_mission_goals = "";
static std::string Standalone_popup_title;
static std::string Standalone_popup_field1 = "";
static std::string Standalone_popup_field2 = "";
static float Standalone_fps = 0.0f;

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

static int Standalone_update_flags = 0;

#define STD_UFLAG_DEBUG_STATE		(1<<0)
#define STD_UFLAG_TITLE				(1<<1)
#define STD_UFLAG_CONN				(1<<2)
#define STD_UFLAG_RESET				(1<<3)

#define STD_UFLAG_SERVER_NAME		(1<<4)
#define STD_UFLAG_HOST_PASS			(1<<5)
#define STD_UFLAG_SET_PING			(1<<6)

#define STD_UFLAG_FPS				(1<<7)
#define STD_UFLAG_MISSION_NAME		(1<<8)
#define STD_UFLAG_MISSION_TIME		(1<<9)
#define STD_UFLAG_NETGAME_INFO		(1<<10)
#define STD_UFLAG_MISSION_GOALS		(1<<11)

#define STD_UFLAG_PLAYER_INFO		(1<<12)

#define STD_UFLAG_S_MESSAGE			(1<<13)

#define STD_UFLAG_POPUP				(1<<14)		// DO NOT INCLUDE IN STD_UFLAG_ALL!!

#define STD_UFLAG_GENERAL			(STD_UFLAG_DEBUG_STATE|STD_UFLAG_TITLE|STD_UFLAG_RESET)
#define STD_UFLAG_TAB_SERVER		(STD_UFLAG_SERVER_NAME|STD_UFLAG_HOST_PASS|STD_UFLAG_CONN|STD_UFLAG_SET_PING)
#define STD_UFLAG_TAB_MULTI			(STD_UFLAG_FPS|STD_UFLAG_MISSION_NAME|STD_UFLAG_MISSION_TIME|STD_UFLAG_NETGAME_INFO|STD_UFLAG_MISSION_GOALS)
#define STD_UFLAG_TAB_PLAYER		(STD_UFLAG_PLAYER_INFO)
#define STD_UFLAG_TAB_GS			(STD_UFLAG_S_MESSAGE)

#define STD_UFLAG_ALL				(STD_UFLAG_GENERAL|STD_UFLAG_TAB_SERVER|STD_UFLAG_TAB_MULTI|STD_UFLAG_TAB_PLAYER|STD_UFLAG_TAB_GS)


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
			std_reset_standalone_gui();

			break;
		}

		case LWS_CALLBACK_SERVER_WRITEABLE: {
			// RESET: *must* come first
			if (Standalone_update_flags & STD_UFLAG_RESET) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "reset");

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending reset command!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_RESET;

				lws_callback_on_writable(wsi);

				break;
			}

			// general messages
			if (Standalone_update_flags & STD_UFLAG_TITLE) {
				size = SDL_snprintf((char *)p, 64, "T:%s %d.%02d.%02d", XSTR("FreeSpace Standalone", 935), FS_VERSION_MAJOR, FS_VERSION_MINOR, FS_VERSION_BUILD);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending title string!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_TITLE;

				lws_callback_on_writable(wsi);

				break;
			}

			if (Standalone_update_flags & STD_UFLAG_DEBUG_STATE) {
				size = SDL_snprintf((char *)p, 32, "D:%s", Standalone_debug_state.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending debug state!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_DEBUG_STATE;

				lws_callback_on_writable(wsi);

				break;
			}

			if (Standalone_update_flags & STD_UFLAG_POPUP) {
				if ( !Standalone_popup_title.empty() ) {
					size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "popup %s;%s;%s", Standalone_popup_title.c_str(), Standalone_popup_field1.c_str(), Standalone_popup_field2.c_str());
				} else {
					size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "popup ");
				}

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending popup!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_POPUP;

				lws_callback_on_writable(wsi);

				break;
			}

			// server tab
			if (Standalone_update_flags & STD_UFLAG_SERVER_NAME) {
				size = SDL_snprintf((char *)p, MAX_GAMENAME_LEN, "S:name %s", Netgame.name);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending server name!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_SERVER_NAME;

				lws_callback_on_writable(wsi);

				break;
			}

			if (Standalone_update_flags & STD_UFLAG_HOST_PASS) {
				size = SDL_snprintf((char *)p, STD_PASSWD_LEN, "S:pass %s", Multi_options_g.std_passwd);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending host password!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_HOST_PASS;

				lws_callback_on_writable(wsi);

				break;
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
					lwsl_err("ERROR sending connetions!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_CONN;

				lws_callback_on_writable(wsi);

				break;
			}

			if ( (Standalone_update_flags & STD_UFLAG_SET_PING) && !Standalone_ping_str.empty() ) {
				SDL_assert(Standalone_ping_str.length() < 1024);

				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "S:ping %s", Standalone_ping_str.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending conn ping!\n");
					return -1;
				}

				Standalone_ping_str.clear();
				Standalone_update_flags &= ~ STD_UFLAG_SET_PING;

				lws_callback_on_writable(wsi);

				break;
			}

			// multi-player tab
			if (Standalone_update_flags & STD_UFLAG_MISSION_NAME) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "M:name %s", Standalone_mission_name.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending mission name!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_MISSION_NAME;

				lws_callback_on_writable(wsi);

				break;
			}

			if (Standalone_update_flags & STD_UFLAG_MISSION_TIME) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "M:time %s", Standalone_mission_time.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending mission time!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_MISSION_TIME;

				lws_callback_on_writable(wsi);

				break;
			}

			if (Standalone_update_flags & STD_UFLAG_NETGAME_INFO) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "M:info %s", Standalone_netgame_info.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending netgame info!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_NETGAME_INFO;

				lws_callback_on_writable(wsi);

				break;
			}

			if (Standalone_update_flags & STD_UFLAG_FPS) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "M:fps %.1f", Standalone_fps);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending fps!\n");
					return -1;
				}

				Standalone_update_flags &= ~STD_UFLAG_FPS;

				lws_callback_on_writable(wsi);

				break;
			}

			if ( (Standalone_update_flags & STD_UFLAG_MISSION_GOALS) && !Standalone_mission_goals.empty() ) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "M:goal %s", Standalone_mission_goals.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending mission goals!\n");
					return -1;
				}

				Standalone_mission_goals.clear();
				Standalone_update_flags &= ~STD_UFLAG_MISSION_GOALS;

				lws_callback_on_writable(wsi);

				break;
			}

			// player tab
			if ( (Standalone_update_flags & STD_UFLAG_PLAYER_INFO) && !Standalone_player_info.empty() ) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "P:info %s", Standalone_player_info.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending player info!\n");
					return -1;
				}

				Standalone_player_info.clear();
				Standalone_update_flags &= ~STD_UFLAG_PLAYER_INFO;

				lws_callback_on_writable(wsi);

				break;
			}

			// god stuff tab
			if ( (Standalone_update_flags & STD_UFLAG_S_MESSAGE) && !Standalone_message.empty() ) {
				size = SDL_snprintf((char *)p, MAX_BUF_SIZE, "G:mesg %s", Standalone_message.c_str());

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending chat message!\n");
					return -1;
				}

				Standalone_message.clear();
				Standalone_update_flags &= ~STD_UFLAG_S_MESSAGE;

				lws_callback_on_writable(wsi);

				break;
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
										Standalone_pinfo_active_player = msg+7;
										std_pinfo_display_player_info(np);

										break;
									}
								}
							}

							if (i == MAX_PLAYERS) {
								Standalone_pinfo_active_player.clear();
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

	info.ka_time = 0;
	info.ka_probes = 0;
	info.ka_interval = 0;

	lws_set_log_level(LLL_ERR|LLL_WARN|LLL_NOTICE, std_lws_logger);

	stand_context = lws_create_context(&info);

	if (stand_context == NULL) {
		Error(LOCATION, "Unable to initialize standalone server!");
	}

	atexit(std_deinit_standalone);

	// turn off all sound and music
	Cmdline_freespace_no_sound = 1;
	Cmdline_freespace_no_music = 1;

	std_reset_standalone_gui();

	std_multi_update_netgame_info_controls();
}

void std_do_gui_frame()
{
	// maybe update selected player stats
	if ( ((Standalone_stats_stamp == -1) || timestamp_elapsed(Standalone_stats_stamp)) && !Standalone_pinfo_active_player.empty() ) {
		Standalone_stats_stamp = timestamp(STD_STATS_UPDATE_TIME);

		for (int i = 0; i < MAX_PLAYERS; i++) {
			net_player *np = &Net_players[i];

			if ( MULTI_CONNECTED((*np)) && (Net_player != np) ) {
				if ( !SDL_strcmp(Standalone_pinfo_active_player.c_str(), np->player->callsign) ) {
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
	if ( ((Standalone_ping_stamp == -1) || timestamp_elapsed(Standalone_ping_stamp)) && !Standalone_ping_str.empty() ) {
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

		// only add it if address isn't already queued up
		if (Standalone_ping_str.find(ip_address) == std::string::npos) {
			Standalone_ping_str.append(ip_address);

			if (p->s_info.ping.ping_avg > 1000) {
				SDL_snprintf(ip_address, SDL_arraysize(ip_address), ",%s;", XSTR("> 1 sec", 914));
			} else {
				SDL_snprintf(ip_address, SDL_arraysize(ip_address), ",%d%s;", p->s_info.ping.ping_avg, XSTR(" ms", 915));
			}

			Standalone_ping_str.append(ip_address);
		}
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
	Standalone_mission_name = mission_name;
	Standalone_update_flags |= STD_UFLAG_MISSION_NAME;
}

void std_multi_set_standalone_missiontime(float mission_time)
{
	char txt[80];
	char timestr[50];
	fix m_time = fl2f(mission_time);

	// format the time string and set the text
	game_format_time(m_time, timestr, SDL_arraysize(timestr));
	SDL_snprintf(txt, SDL_arraysize(txt), "%s  :  %.1f", timestr, mission_time);

	Standalone_mission_time = txt;
	Standalone_update_flags |= STD_UFLAG_MISSION_TIME;
}

void std_multi_update_netgame_info_controls()
{
	char nginfo[50];

	SDL_snprintf(nginfo, SDL_arraysize(nginfo), "%d,%d,%d,%d", Netgame.max_players, Netgame.options.max_observers, Netgame.security, Netgame.respawn);

	Standalone_netgame_info = nginfo;
	Standalone_update_flags |= STD_UFLAG_NETGAME_INFO;
}

void std_set_standalone_fps(float fps)
{
	if ( (Standalone_fps_stamp == -1) || timestamp_elapsed(Standalone_fps_stamp) ) {
		Standalone_fps_stamp = timestamp(STD_FPS_UPDATE_TIME);

		Standalone_fps = fps;
		Standalone_update_flags |= STD_UFLAG_FPS;
	}
}

void std_multi_setup_goal_tree()
{
	std::string primary;
	std::string secondary;
	std::string bonus;
	std::string status;

	Standalone_mission_goals.clear();

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
		Standalone_mission_goals.append("i none");
	} else {
		Standalone_mission_goals.append(primary.substr(0, primary.size()-1));
	}

	Standalone_mission_goals.append(";");

	if ( secondary.empty() ) {
		Standalone_mission_goals.append("i none");
	} else {
		Standalone_mission_goals.append(secondary.substr(0, secondary.size()-1));
	}

	Standalone_mission_goals.append(";");

	if ( bonus.empty() ) {
		Standalone_mission_goals.append("i none");
	} else {
		Standalone_mission_goals.append(bonus.substr(0, bonus.size()-1));
	}

	Standalone_update_flags |= STD_UFLAG_MISSION_GOALS;
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
	Standalone_stats_stamp = -1;
	Standalone_ng_stamp = -1;
	Standalone_ping_stamp = -1;
	Standalone_fps_stamp = -1;

	Standalone_ping_str.clear();
	Standalone_player_info.clear();
	Standalone_message.clear();
	Standalone_pinfo_active_player.clear();
	Standalone_mission_name = "";
	Standalone_mission_time = "";
	Standalone_netgame_info.clear();
	Standalone_popup_title.clear();
	Standalone_popup_field1 = "";
	Standalone_popup_field2 = "";

	std_set_standalone_fps(0.0f);
	std_multi_set_standalone_missiontime(0.0f);

	Standalone_update_flags |= STD_UFLAG_ALL;
}

void std_create_gen_dialog(const char *title)
{
	Standalone_popup_title = title;
}

void std_destroy_gen_dialog()
{
	Standalone_popup_title.clear();

	Standalone_update_flags |= STD_UFLAG_POPUP;
}

void std_gen_set_text(const char *str, int field_num)
{
	switch (field_num) {
		case 0:
			Standalone_popup_title = str;
			break;

		case 1:
			Standalone_popup_field1 = str;
			break;

		case 2:
			Standalone_popup_field2 = str;
			break;

		default:
			break;
	}

	Standalone_update_flags |= STD_UFLAG_POPUP;

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
