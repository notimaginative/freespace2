/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/

#ifndef STAND_GUI_H
#define STAND_GUI_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include "wx/notebook.h"
#include "wx/treectrl.h"
#include "wx/evtloop.h"

#include <libwebsockets.h>
#include <list>
#include <vector>


class Standalone;

class StandaloneTimer : public wxTimer
{
	private:
		Standalone *m_stand;

	public:
		StandaloneTimer(Standalone *stand);

		~StandaloneTimer();

		void Notify();
};

class StandPopup : public wxFrame
{
	private:

	protected:
		wxStaticText* m_Label1;
		wxStaticText* m_Label2;

	public:
		StandPopup( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Popup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxFRAME_FLOAT_ON_PARENT|wxFRAME_TOOL_WINDOW );

		~StandPopup();

		void SetLabel1(const char *val)
		{
			m_Label1->SetLabel(val);
		}

		void SetLabel2(const char *val)
		{
			m_Label2->SetLabel(val);
		}
};

class Standalone : public wxDialog
{
	private:
		void createTab_Server(wxNotebook *parent);
		void createTab_Multi(wxNotebook *parent);
		void createTab_Player(wxNotebook *parent);
		void createTab_GodStuff(wxNotebook *parent);
		void createTab_Debug(wxNotebook *parent);

		long fspid;
		int fsport;
		struct lws_context *stand_context;
		struct lws *wsi_standalone;
		struct lws_client_connect_info ccinfo;
		StandaloneTimer *m_timer;
		int m_rate_limit;
		std::list<std::string> send_buf;

		wxDECLARE_EVENT_TABLE();

	protected:
		enum
		{
			ID_B_KICK = 1000,
			ID_B_MREFRESH,
			ID_B_RESET_ALL,
			ID_FPS_SLIDER,
			ID_B_SHUTDOWN,
			ID_T_SERVER_NAME,
			ID_T_HOST_PASS,
			ID_T_MSG,
			ID_C_P_PLAYERS
		};

		StandPopup *m_popup;

		wxTextCtrl* m_S_ServerName;
		wxTextCtrl* m_S_HostPass;
		wxStaticText* m_S_NumConn;
		wxTextCtrl* m_S_Connections;
		wxButton* m_S_btnKick;
		wxButton* m_S_btnMRefresh;
		wxButton* m_S_btnResetAll;

		wxSlider* m_M_sliderFPS;
		wxStaticText* m_M_FPS;
		wxStaticText* m_M_FPSRel;
		wxStaticText* m_M_MissionName;
		wxStaticText* m_M_MissionTime;
		wxStaticText* m_M_ngMaxPlayers;
		wxStaticText* m_M_ngMaxObservers;
		wxStaticText* m_M_ngSecurity;
		wxStaticText* m_M_ngRespawns;
		wxTreeCtrl* m_M_Goals;

		wxChoice* m_P_Players;
		wxStaticText* m_P_ShipType;
		wxStaticText* m_P_AvgPing;
		wxStaticText* m_P_atsPriShots;
		wxStaticText* m_P_atsPriHits;
		wxStaticText* m_P_atsPriBHHits;
		wxStaticText* m_P_atsPriHitPer;
		wxStaticText* m_P_atsPriBHHitPer;
		wxStaticText* m_P_atsSecShots;
		wxStaticText* m_P_atsSecHits;
		wxStaticText* m_P_atsSecBHHits;
		wxStaticText* m_P_atsSecHitPer;
		wxStaticText* m_P_atsSecBHHitPer;
		wxStaticText* m_P_atsAssists;
		wxStaticText* m_P_msPriShots;
		wxStaticText* m_P_msPriHits;
		wxStaticText* m_P_msPriBHHits;
		wxStaticText* m_P_msPriHitPer;
		wxStaticText* m_P_msPriBHHitPer;
		wxStaticText* m_P_msSecShots;
		wxStaticText* m_P_msSecHits;
		wxStaticText* m_P_msSecBHHits;
		wxStaticText* m_P_msSecHitPer;
		wxStaticText* m_P_msSecBHHitPer;
		wxStaticText* m_P_msAssists;

		wxChoice* m_GS_Players;
		wxTextCtrl* m_GS_msg;
		wxTextCtrl* m_GS_Messages;

		wxStaticText* m_D_State;

		void ResetAll();
		void Shutdown();

		void OnClose( wxCloseEvent& event );
		void OnShutdown( wxCommandEvent& event );
		void OnServerNameChange( wxCommandEvent& event );
		void OnHostPassChange( wxCommandEvent& event );
		void OnKick( wxCommandEvent& event );
		void OnMissionRefresh( wxCommandEvent& event );
		void OnResetAll( wxCommandEvent& event );
		void OnFPSSel( wxCommandEvent& event );
		void OnServerMsg( wxCommandEvent& event );
		void OnPinfoPlayer( wxCommandEvent& event );

	public:

		Standalone( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Freespace Standalone"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxCLOSE_BOX|wxDIALOG_NO_PARENT );
		~Standalone();

		bool startFreeSpace(int argc, wxCmdLineArgsArray& argv);

		bool wsInitialize();
		void wsDoFrame();
		void wsDisconnect();
		void wsMessage(const char *msg, size_t len);
		void wsSend(std::string &msg);

		std::list<std::string> &wsGetSendBuffer()
		{
			return send_buf;
		}
};

class StandaloneApp: public wxApp
{
	private:
		Standalone *std_client;

	public:
		virtual bool OnInit();

		StandaloneApp() : std_client(nullptr)
		{
		}

		Standalone &Client()
		{
			wxASSERT(std_client != nullptr);

			return *std_client;
		}
};

#endif // STAND_GUI_H
