/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include "wx/sound.h"

#include "al.h"
#include "alc.h"


class wxBackgroundBitmap : public wxEvtHandler
{
	private:
		typedef wxEvtHandler Inherited;

	protected:
		wxBitmap Bitmap;

	public:
		wxBackgroundBitmap(const wxBitmap &B) : wxEvtHandler(), Bitmap(B) { }
		virtual bool ProcessEvent(wxEvent &Event);
};

class wxLauncherButton : public wxStaticBitmap
{
	private:
		bool m_in_hover;

		wxBitmap m_bitmap;
		wxBitmap m_bitmap_hover;
		wxBitmap m_bitmap_pressed;

		wxDECLARE_EVENT_TABLE();

	protected:


	public:
		wxLauncherButton(wxWindow* parent, wxWindowID id, const wxBitmap& label, const wxPoint& pos, const wxSize& size);
		~wxLauncherButton();

		void onMouseDown(wxMouseEvent& event);
		void onMouseUp(wxMouseEvent& event);
		void onMouseEnter(wxMouseEvent& event);
		void onMouseLeave(wxMouseEvent& event);

		void SetBitmapHover(const wxBitmap& bitmap)
		{
			m_bitmap_hover = bitmap;
		}

		void SetBitmapPressed(const wxBitmap& bitmap)
		{
			m_bitmap_pressed = bitmap;
		}
};

class Launcher : public wxDialog
{
	private:
		bool use_sound;

		ALCdevice *al_device;
		ALCcontext *al_context;

		ALuint m_snd_hover_buf_id;
		ALuint m_snd_click_buf_id;

		ALuint m_snd_hover_source_id;
		ALuint m_snd_click_source_id;

		void init_sound();
		void close_sound();

		wxDECLARE_EVENT_TABLE();

	protected:

		enum {
			ID_B_PLAY = 1000,
			ID_B_SETUP,
			ID_B_README,
			ID_B_UPDATE,
			ID_B_HELP,
			ID_B_UNINSTALL,
			ID_B_VOLITION,
			ID_B_PXO,
			ID_B_QUIT
		};

		wxBackgroundBitmap *p_background;

		wxPanel* m_panel;

#ifndef MAKE_FS1
		wxLauncherButton* m_btn_Play;
		wxLauncherButton* m_btn_Setup;
		wxLauncherButton* m_btn_Readme;
		wxLauncherButton* m_btn_Update;
		wxLauncherButton* m_btn_Help;
		wxLauncherButton* m_btn_Uninstall;
		wxLauncherButton* m_btn_Volition;
		wxLauncherButton* m_btn_PXO;
		wxLauncherButton* m_btn_Quit;
#else
		wxButton* m_btn_Play;
		wxButton* m_btn_Setup;
		wxButton* m_btn_Readme;
		wxButton* m_btn_Update;
		wxButton* m_btn_Uninstall;
		wxButton* m_btn_Volition;
		wxButton* m_btn_Quit;
#endif

		void OnClose(wxCloseEvent& event);

		void OnPlay(wxCommandEvent& event);
		void OnSetup(wxCommandEvent& event);
		void OnReadme(wxCommandEvent& event);
		void OnUpdate(wxCommandEvent& event);
		void OnHelp(wxCommandEvent& event);
		void OnUninstall(wxCommandEvent& event);
		void OnVolition(wxCommandEvent& event);
		void OnPXO(wxCommandEvent& event);
		void OnQuit(wxCommandEvent& event);

	public:

		Launcher( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Launcher"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~Launcher();

		void JumpToSetup();

		void SndPlayHover();
		void SndPlayPressed();
		void SndEnable(bool enabled	= true);
};

#endif // LAUNCHER_H
