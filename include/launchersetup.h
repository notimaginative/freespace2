/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef LAUNCHERSETUP_H
#define LAUNCHERSETUP_H


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include "wx/notebook.h"


class LauncherSetup : public wxDialog
{
	private:
		void initTab_Video(wxNotebook* parent);
		void initTab_Audio(wxNotebook* parent);
		void initTab_Joystick(wxNotebook* parent);
		void initTab_Speed(wxNotebook* parent);
		void initTab_Network(wxNotebook* parent);
		void initTab_PXO(wxNotebook* parent);

		void saveTab_Video();
		void saveTab_Audio();
		void saveTab_Joystick();
		void saveTab_Speed();
		void saveTab_Network();
		void saveTab_PXO();

		void save_settings();

		wxDECLARE_EVENT_TABLE();

	protected:
		enum {
			ID_CB_MSAA = 1000
		};

		wxComboBox* m_Video_Renderer;
		wxCheckBox* m_Video_Fullscreen;
		wxCheckBox* m_Video_MSAA;
		wxChoice* m_Video_MSAASamples;
		wxCheckBox* m_Video_ShowFPS;

		wxComboBox* m_Audio_PlaybackDevice;
		wxComboBox* m_Audio_CaptureDevice;
		wxCheckBox* m_Audio_EFX;
		wxCheckBox* m_Audio_LauncherSounds;

		wxComboBox* m_Joystick_Device;
		wxCheckBox* m_Joystick_FF;
		wxCheckBox* m_Joystick_Directional;

		wxComboBox* m_Speed_DefaultDetail;

		wxRadioBox* m_Network_Connection;
		wxRadioBox* m_Network_Speed;
		wxTextCtrl* m_PXO_Login;
		wxTextCtrl* m_PXO_Password;
		wxCheckBox* m_SkipVerify;
		wxCheckBox* m_PXOBanners;
		wxTextCtrl* m_Network_Port;
		unsigned short m_port_validate;

		void onOk(wxCommandEvent& event);
		void onToggleMSAA(wxCommandEvent& event);

	public:

		LauncherSetup( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Setup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~LauncherSetup();

};



#endif // LAUNCHERSETUP_H
