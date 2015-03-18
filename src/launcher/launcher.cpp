/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "launcher.h"

#include "wx/filename.h"
#include "wx/stdpaths.h"

#include "pstypes.h"
#include "osregistry.h"
#include "cfile.h"

#ifndef MAKE_FS1
#include "res/fs2_background.xpm"
#include "res/fs2_btn_help.xpm"
#include "res/fs2_btn_help-hover.xpm"
#include "res/fs2_btn_help-click.xpm"
#include "res/fs2_btn_play.xpm"
#include "res/fs2_btn_play-hover.xpm"
#include "res/fs2_btn_play-click.xpm"
#include "res/fs2_btn_pxo.xpm"
#include "res/fs2_btn_pxo-hover.xpm"
#include "res/fs2_btn_pxo-click.xpm"
#include "res/fs2_btn_quit.xpm"
#include "res/fs2_btn_quit-hover.xpm"
#include "res/fs2_btn_quit-click.xpm"
#include "res/fs2_btn_readme.xpm"
#include "res/fs2_btn_readme-hover.xpm"
#include "res/fs2_btn_readme-click.xpm"
#include "res/fs2_btn_setup.xpm"
#include "res/fs2_btn_setup-hover.xpm"
#include "res/fs2_btn_setup-click.xpm"
#include "res/fs2_btn_uninstall.xpm"
#include "res/fs2_btn_uninstall-hover.xpm"
#include "res/fs2_btn_uninstall-click.xpm"
#include "res/fs2_btn_update.xpm"
#include "res/fs2_btn_update-hover.xpm"
#include "res/fs2_btn_update-click.xpm"
#include "res/fs2_btn_volition.xpm"
#include "res/fs2_btn_volition-hover.xpm"
#include "res/fs2_btn_volition-click.xpm"

#include "res/fs2_snd_hover_wav.inc"
#include "res/fs2_snd_click_wav.inc"
#else
#include "res/freespace_img.xpm"
#include "res/volition_img.xpm"
#endif



class LauncherApp: public wxApp
{
	public:
		virtual bool OnInit();
};


IMPLEMENT_APP(LauncherApp)

bool LauncherApp::OnInit()
{
	Launcher *frame = new Launcher(NULL);

	frame->Show();
	SetTopWindow(frame);

	return true;
}

bool wxBackgroundBitmap::ProcessEvent(wxEvent &Event)
{
	if (Event.GetEventType() == wxEVT_ERASE_BACKGROUND) {
		wxEraseEvent &EraseEvent = dynamic_cast<wxEraseEvent &>(Event);
		wxDC *DC = EraseEvent.GetDC();
		DC->DrawBitmap(Bitmap, 0, 0, false);

		return true;
	} else {
		return Inherited::ProcessEvent(Event);
	}
}


wxBEGIN_EVENT_TABLE(wxLauncherButton, wxStaticBitmap)
	EVT_LEFT_DOWN(wxLauncherButton::onMouseDown)
	EVT_LEFT_UP(wxLauncherButton::onMouseUp)
	EVT_MOTION(wxLauncherButton::onMouseEnter)
wxEND_EVENT_TABLE()


wxLauncherButton::wxLauncherButton(wxWindow *parent, wxWindowID id, const wxBitmap& label, const wxPoint& pos, const wxSize& size)
{
	this->Create(parent, id, label, pos, size);

	parent->Connect(wxEVT_MOTION, wxMouseEventHandler(wxLauncherButton::onMouseLeave), NULL, this);

	m_bitmap = label;

	m_in_hover = false;
}

wxLauncherButton::~wxLauncherButton()
{
}

void wxLauncherButton::onMouseEnter(wxMouseEvent& event)
{
	if ( !m_in_hover ) {
		this->SetBitmap(m_bitmap_hover);

		((Launcher*)GetGrandParent())->SndPlayHover();

		m_in_hover = true;

		// hack to toggle off certain buttons that don't get the onMouseLeave
		// call due to placement/overlap
		{
			wxMouseEvent mv;

			const wxWindowList wl = GetParent()->GetChildren();

			wxWindowList::compatibility_iterator node = wl.GetFirst();

			while (node) {
				wxLauncherButton *cur = (wxLauncherButton*)node->GetData();

				if ( cur->GetId() != event.GetId() ) {
					cur->onMouseLeave(mv);
				}

				node = node->GetNext();
			}
		}
	}

	event.Skip();
}

void wxLauncherButton::onMouseLeave(wxMouseEvent& event)
{
	if (m_in_hover) {
		this->SetBitmap(m_bitmap);

		m_in_hover = false;
	}

	event.Skip();
}

void wxLauncherButton::onMouseDown(wxMouseEvent& event)
{
	this->SetBitmap(m_bitmap_pressed);

	((Launcher*)GetGrandParent())->SndPlayPressed();

	event.Skip();
}

void wxLauncherButton::onMouseUp(wxMouseEvent& event)
{
	this->SetBitmap(m_bitmap_hover);

	wxCommandEvent ev(wxEVT_COMMAND_BUTTON_CLICKED, event.GetId());
	GetGrandParent()->GetEventHandler()->ProcessEvent(ev);

	event.Skip();
}


wxBEGIN_EVENT_TABLE(Launcher, wxDialog)
	EVT_CLOSE(Launcher::OnClose)
	EVT_BUTTON(ID_B_PLAY, Launcher::OnPlay)
	EVT_BUTTON(ID_B_SETUP, Launcher::OnSetup)
	EVT_BUTTON(ID_B_README, Launcher::OnReadme)
	EVT_BUTTON(ID_B_UPDATE, Launcher::OnUpdate)
	EVT_BUTTON(ID_B_HELP, Launcher::OnHelp)
	EVT_BUTTON(ID_B_UNINSTALL, Launcher::OnUninstall)
	EVT_BUTTON(ID_B_VOLITION, Launcher::OnVolition)
	EVT_BUTTON(ID_B_PXO, Launcher::OnPXO)
	EVT_BUTTON(ID_B_QUIT, Launcher::OnQuit)
wxEND_EVENT_TABLE()


Launcher::Launcher( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
	: wxDialog( parent, id, title, pos, size, style )
{
	this->SetBackgroundColour( wxColour( 0, 0, 0 ) );

	use_sound = false;

#ifndef MAKE_FS1
	this->SetClientSize(375, 440);
	this->SetTitle( wxT("FreeSpace 2 Launcher") );

	init_sound();


	p_background = new wxBackgroundBitmap(fs2_background_xpm);

	m_panel = new wxPanel(this);
	m_panel->PushEventHandler(p_background);


	m_btn_Play = new wxLauncherButton( m_panel, ID_B_PLAY, wxBitmap(fs2_btn_play_xpm), wxPoint(45, 102), wxSize(131, 58) );
	m_btn_Play->SetBitmapHover( wxBitmap(fs2_btn_play_hover_xpm) );
	m_btn_Play->SetBitmapPressed( wxBitmap(fs2_btn_play_click_xpm) );

	m_btn_Setup = new wxLauncherButton( m_panel, ID_B_SETUP, wxBitmap(fs2_btn_setup_xpm), wxPoint(199, 102), wxSize(131, 58) );
	m_btn_Setup->SetBitmapHover( wxBitmap(fs2_btn_setup_hover_xpm) );
	m_btn_Setup->SetBitmapPressed( wxBitmap(fs2_btn_setup_click_xpm) );

	m_btn_Readme = new wxLauncherButton( m_panel, ID_B_README, wxBitmap(fs2_btn_readme_xpm), wxPoint(45, 175), wxSize(131, 58) );
	m_btn_Readme->SetBitmapHover( wxBitmap(fs2_btn_readme_hover_xpm) );
	m_btn_Readme->SetBitmapPressed( wxBitmap(fs2_btn_readme_click_xpm) );

	m_btn_Update = new wxLauncherButton( m_panel, ID_B_UPDATE, wxBitmap(fs2_btn_update_xpm), wxPoint(199, 175), wxSize(131, 58) );
	m_btn_Update->SetBitmapHover( wxBitmap(fs2_btn_update_hover_xpm) );
	m_btn_Update->SetBitmapPressed( wxBitmap(fs2_btn_update_click_xpm) );

	m_btn_Help = new wxLauncherButton( m_panel, ID_B_HELP, wxBitmap(fs2_btn_help_xpm), wxPoint(45, 247), wxSize(131, 58) );
	m_btn_Help->SetBitmapHover( wxBitmap(fs2_btn_help_hover_xpm) );
	m_btn_Help->SetBitmapPressed( wxBitmap(fs2_btn_help_click_xpm) );

	m_btn_Uninstall = new wxLauncherButton( m_panel, ID_B_UNINSTALL, wxBitmap(fs2_btn_uninstall_xpm), wxPoint(199, 247), wxSize(131, 58) );
	m_btn_Uninstall->SetBitmapHover( wxBitmap(fs2_btn_uninstall_hover_xpm) );
	m_btn_Uninstall->SetBitmapPressed( wxBitmap(fs2_btn_uninstall_click_xpm) );

	m_btn_Volition = new wxLauncherButton( m_panel, ID_B_VOLITION, wxBitmap(fs2_btn_volition_xpm), wxPoint(15, 304), wxSize(90, 108) );
	m_btn_Volition->SetBitmapHover( wxBitmap(fs2_btn_volition_hover_xpm) );
	m_btn_Volition->SetBitmapPressed( wxBitmap(fs2_btn_volition_click_xpm) );

	m_btn_PXO = new wxLauncherButton( m_panel, ID_B_PXO, wxBitmap(fs2_btn_pxo_xpm), wxPoint(249, 305), wxSize(114, 113) );
	m_btn_PXO->SetBitmapHover( wxBitmap(fs2_btn_pxo_hover_xpm) );
	m_btn_PXO->SetBitmapPressed( wxBitmap(fs2_btn_pxo_click_xpm) );

	m_btn_Quit = new wxLauncherButton( m_panel, ID_B_QUIT, wxBitmap(fs2_btn_quit_xpm), wxPoint(116, 339), wxSize(131, 58) );
	m_btn_Quit->SetBitmapHover( wxBitmap(fs2_btn_quit_hover_xpm) );
	m_btn_Quit->SetBitmapPressed( wxBitmap(fs2_btn_quit_click_xpm) );
#else
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetTitle( wxT("FreeSpace Launcher") );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	wxStaticBitmap *m_bitmap1 = new wxStaticBitmap( this, wxID_ANY, wxBitmap( freespace_img_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_bitmap1, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticBitmap *m_bitmap2 = new wxStaticBitmap( this, wxID_ANY, wxBitmap( volition_img_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_bitmap2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_btn_Play = new wxButton( this, ID_B_PLAY, wxT("Play FreeSpace"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btn_Play->SetDefault();
	bSizer5->Add( m_btn_Play, 0, wxALL|wxEXPAND, 5 );

	m_btn_Setup = new wxButton( this, ID_B_SETUP, wxT("Setup"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btn_Setup, 0, wxALL|wxEXPAND, 5 );

	m_btn_Readme = new wxButton( this, ID_B_README, wxT("View README"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btn_Readme, 0, wxALL|wxEXPAND, 5 );

	m_btn_Update = new wxButton( this, ID_B_UPDATE, wxT("Update FreeSpace"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btn_Update, 0, wxALL|wxEXPAND, 5 );

	m_btn_Volition = new wxButton( this, ID_B_VOLITION, wxT("FreeSpace Webpage"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btn_Volition, 0, wxALL|wxEXPAND, 5 );

	m_btn_Uninstall = new wxButton( this, ID_B_UNINSTALL, wxT("Uninstall"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btn_Uninstall, 0, wxALL|wxEXPAND, 5 );

	m_btn_Quit = new wxButton( this, ID_B_QUIT, wxT("Quit"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btn_Quit, 0, wxALL|wxEXPAND, 5 );

	fgSizer3->Add( bSizer5, 1, wxALIGN_CENTER|wxALL, 10 );

	bSizer3->Add( fgSizer3, 1, wxALIGN_BOTTOM|wxALIGN_RIGHT|wxEXPAND, 5 );

	this->SetSizer( bSizer3 );
	this->Layout();
	bSizer3->Fit( this );
#endif

	this->Centre( wxBOTH );
}

Launcher::~Launcher()
{
	close_sound();
}

void Launcher::OnClose( wxCloseEvent& WXUNUSED(event) )
{
#ifndef MAKE_FS1
	m_panel->RemoveEventHandler(p_background);
	delete p_background;
#endif

	Destroy();
}

void Launcher::OnPlay( wxCommandEvent& WXUNUSED(event) )
{
	wxString epath = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath(true);

#ifndef MAKE_FS1
	epath.Append( wxT("freespace2") );
#else
	epath.Append( wxT("freespace") );
#endif

#ifdef FS2_DEMO
	epath.Append( wxT("_demo") );
#endif

#ifdef _WIN32
	epath.Append( wxT(".exe") );
#endif

	wxExecute(epath, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER | wxEXEC_HIDE_CONSOLE);

	this->Close();
}

void Launcher::OnSetup( wxCommandEvent& WXUNUSED(event) )
{

}

void Launcher::OnReadme( wxCommandEvent& WXUNUSED(event) )
{
	wxLaunchDefaultApplication("README.txt");
}

void Launcher::OnUpdate( wxCommandEvent& WXUNUSED(event) )
{
	wxMessageBox( wxT("Not implemented") );
}

void Launcher::OnHelp( wxCommandEvent& WXUNUSED(event) )
{

}

void Launcher::OnUninstall( wxCommandEvent& WXUNUSED(event) )
{
	wxMessageBox( wxT("Not implemented") );
}

void Launcher::OnVolition( wxCommandEvent& WXUNUSED(event) )
{
	wxLaunchDefaultBrowser( wxT("http://www.volition-inc.com") );
}

void Launcher::OnPXO( wxCommandEvent& WXUNUSED(event) )
{
	wxLaunchDefaultBrowser( wxT("http://www.pxo.net") );
}

void Launcher::OnQuit( wxCommandEvent& WXUNUSED(event) )
{
	this->Close();
}

void Launcher::SndPlayHover()
{
	if (use_sound) {
		alSourcePlay(m_snd_hover_source_id);
	}
}

void Launcher::SndPlayPressed()
{
	if (use_sound) {
		alSourcePlay(m_snd_click_source_id);
	}
}

void Launcher::init_sound()
{
#ifndef MAKE_FS1
	if (use_sound) {
		return;
	}

	if ( os_config_read_uint(NULL, "LauncherSoundEnabled", 1) == 0 ) {
		return;
	}

	al_device = alcOpenDevice(NULL);

	if (al_device == NULL) {
		return;
	}

	al_context = alcCreateContext(al_device, NULL);

	if (al_context == NULL) {
		alcCloseDevice(al_device);
		return;
	}

	alcMakeContextCurrent(al_context);

	// 'hover' sound
	alGenBuffers(1, &m_snd_hover_buf_id);
	alBufferData(m_snd_hover_buf_id, AL_FORMAT_MONO8, fs2_snd_hover_wav, sizeof(fs2_snd_hover_wav), 22050);

	alGenSources(1, &m_snd_hover_source_id);
	alSourcef(m_snd_hover_source_id, AL_GAIN, 1.0f);
	alSourcei(m_snd_hover_source_id, AL_BUFFER, m_snd_hover_buf_id);

	// 'click' sound
	alGenBuffers(1, &m_snd_click_buf_id);
	alBufferData(m_snd_click_buf_id, AL_FORMAT_MONO8, fs2_snd_click_wav, sizeof(fs2_snd_click_wav), 22050);

	alGenSources(1, &m_snd_click_source_id);
	alSourcef(m_snd_click_source_id, AL_GAIN, 1.0f);
	alSourcei(m_snd_click_source_id, AL_BUFFER, m_snd_click_buf_id);

	use_sound = true;
#endif
}

void Launcher::close_sound()
{
	if ( !use_sound ) {
		return;
	}

	alSourceStop(m_snd_click_source_id);

	alSourcei(m_snd_click_source_id, AL_BUFFER, 0);
	alDeleteSources(1, &m_snd_click_source_id);
	alDeleteBuffers(1, &m_snd_click_buf_id);

	alSourceStop(m_snd_hover_source_id);

	alSourcei(m_snd_hover_source_id, AL_BUFFER, 0);
	alDeleteSources(1, &m_snd_hover_source_id);
	alDeleteBuffers(1, &m_snd_hover_buf_id);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(al_context);
	alcCloseDevice(al_device);

	use_sound = false;
}
