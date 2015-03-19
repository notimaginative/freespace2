
#include "SDL.h"

#include "launcher.h"
#include "launchersetup.h"
#include "osregistry.h"

#include "wx/valnum.h"


LauncherSetup::LauncherSetup( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize(370, -1), wxDefaultSize );


	wxNotebook *nbook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	initTab_Video(nbook);
	initTab_Audio(nbook);
	initTab_Joystick(nbook);
	initTab_Speed(nbook);
	initTab_Network(nbook);
	initTab_PXO(nbook);

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );
	bSizer->Add( nbook, 0, wxALL|wxEXPAND, 5 );


	wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
	sdbSizer->AddButton( new wxButton(this, wxID_OK) );
	sdbSizer->AddButton( new wxButton(this, wxID_CANCEL) );
	sdbSizer->Realize();

	bSizer->Add( sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizer );
	this->Layout();
	bSizer->Fit( this );


	this->Centre( wxBOTH );
}

LauncherSetup::~LauncherSetup()
{
}

void LauncherSetup::initTab_Video(wxNotebook *parent)
{
	unsigned int checked = 0;

	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );


	// 'renderer'
	wxStaticBoxSizer* sbSizer = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("Renderer") ), wxVERTICAL );

	m_Video_Renderer = new wxComboBox( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	sbSizer->Add( m_Video_Renderer, 1, wxALL|wxEXPAND, 5 );

	bSizer->Add( sbSizer, 0, wxALL|wxEXPAND, 5 );

	m_Video_Renderer->Append( wxT("OpenGL") );
	m_Video_Renderer->SetSelection( 0 );

	// 'fullscreen'
	m_Video_Fullscreen = new wxCheckBox( panel, wxID_ANY, wxT("Fullscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_Video_Fullscreen, 0, wxALL, 5 );

	checked = os_config_read_uint("Video", "Fullscreen", 1);

	m_Video_Fullscreen->SetValue( (checked == 1) );

	// 'show fps'
	m_Video_ShowFPS = new wxCheckBox( panel, wxID_ANY, wxT("Show FPS"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_Video_ShowFPS, 0, wxALL, 5 );

	checked = os_config_read_uint("Video", "ShowFPS", 0);

	m_Video_ShowFPS->SetValue( (checked == 1) );

	// 'anti-alias'
	wxBoxSizer* bSizer_aa = new wxBoxSizer( wxHORIZONTAL );

	m_Video_MSAA = new wxCheckBox( panel, wxID_ANY, wxT("Anti-Alias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer_aa->Add( m_Video_MSAA, 0, wxALIGN_CENTER_VERTICAL, 5 );

	const wxString msaa_samples[] = { wxT("2x"), wxT("4x"), wxT("8x"), wxT("16x") };
	const int num_msaa_samples = sizeof( msaa_samples ) / sizeof( wxString );
	m_Video_MSAASamples = new wxChoice( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, num_msaa_samples, msaa_samples, 0 );
	bSizer_aa->Add( m_Video_MSAASamples, 0, wxLEFT, 15 );

	bSizer->Add( bSizer_aa, 0, wxALL, 5 );

	checked = os_config_read_uint("Video", "AntiAlias", 0);

	if (checked) {
		m_Video_MSAA->SetValue(true);

		if (checked < 4) {
			m_Video_MSAASamples->SetSelection(0);
		} else if (checked < 8) {
			m_Video_MSAASamples->SetSelection(1);
		} else if (checked < 16) {
			m_Video_MSAASamples->SetSelection(2);
		} else {
			m_Video_MSAASamples->SetSelection(3);
		}
	} else {
		m_Video_MSAA->SetValue(false);
		m_Video_MSAASamples->Disable();
	}


	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("Video"), true );
}

void LauncherSetup::initTab_Audio(wxNotebook *parent)
{
	unsigned int checked = 0;
	ALint ver_major = 0, ver_minor = 0;
	const ALCchar *ptr = NULL;
	const char *conf_ptr = NULL;
	bool audio_enabled = false;

	alcGetIntegerv(NULL, ALC_MAJOR_VERSION, 1, &ver_major);
	alcGetIntegerv(NULL, ALC_MINOR_VERSION, 1, &ver_minor);

	if ( (ver_major >= 1) && (ver_minor >= 1) ) {
		audio_enabled = true;
	}

	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );


	// 'playback device'
	wxStaticBoxSizer* sbSizer_playback = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("Playback Device") ), wxVERTICAL );

	m_Audio_PlaybackDevice = new wxComboBox( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	sbSizer_playback->Add( m_Audio_PlaybackDevice, 0, wxALL|wxEXPAND, 5 );

	bSizer->Add( sbSizer_playback, 0, wxALL|wxEXPAND, 5 );

	m_Audio_PlaybackDevice->Append("<Default>");
	m_Audio_PlaybackDevice->SetSelection(0);

	conf_ptr = os_config_read_string("Audio", "PlaybackDevice", NULL);

	if (audio_enabled) {
		if ( alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE ) {
			ptr = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
		} else {
			ptr = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
		}

		if (ptr) {
			while (*ptr) {
				m_Audio_PlaybackDevice->Append(ptr);

				if ( conf_ptr && !SDL_strcasecmp(conf_ptr, ptr) ) {
					unsigned int sel = m_Audio_PlaybackDevice->GetCount() - 1;

					m_Audio_PlaybackDevice->SetSelection(sel);

					conf_ptr = NULL;
				}

				ptr += strlen(ptr) + 1;
			}
		}
	}

	// 'capture device'
	wxStaticBoxSizer* sbSizer_capture = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("Capture Device") ), wxVERTICAL );

	m_Audio_CaptureDevice = new wxComboBox( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	sbSizer_capture->Add( m_Audio_CaptureDevice, 0, wxALL|wxEXPAND, 5 );

	bSizer->Add( sbSizer_capture, 0, wxALL|wxEXPAND, 5 );

	m_Audio_CaptureDevice->Append("<Default>");
	m_Audio_CaptureDevice->SetSelection(0);

	conf_ptr = os_config_read_string("Audio", "CaptureDevice", NULL);

	if (audio_enabled) {
		ptr = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);

		if (ptr) {
			while (*ptr) {
				m_Audio_CaptureDevice->Append(ptr);

				if ( conf_ptr && !SDL_strcasecmp(conf_ptr, ptr) ) {
					unsigned int sel = m_Audio_CaptureDevice->GetCount() - 1;

					m_Audio_CaptureDevice->SetSelection(sel);

					conf_ptr = NULL;
				}

				ptr += strlen(ptr) + 1;
			}
		}
	}

	// 'efx'
	m_Audio_EFX = new wxCheckBox( panel, wxID_ANY, wxT("EFX / EAX Reverb"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_Audio_EFX, 0, wxALL, 5 );

	checked = os_config_read_uint("Audio", "EFX", 0);

	m_Audio_EFX->SetValue( (checked == 1) );

	// 'launcher sounds'
#ifndef MAKE_FS1
	m_Audio_LauncherSounds = new wxCheckBox( panel, wxID_ANY, wxT("Enable Launcher Sounds"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_Audio_LauncherSounds, 0, wxALL, 5 );

	checked = os_config_read_uint("Audio", "LauncherSoundEnabled", 1);

	m_Audio_LauncherSounds->SetValue( (checked == 1) );
#endif


//	bSizer->Add( fgSizer, 1, wxEXPAND, 5 );

	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("Audio"), false );
}

void LauncherSetup::initTab_Joystick(wxNotebook *parent)
{
	unsigned int checked = 0;
	const char *conf_ptr = NULL;
	bool joystick_enabled = false;

	if ( !SDL_InitSubSystem(SDL_INIT_JOYSTICK) ) {
		joystick_enabled = true;
	}

	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("Joystick") ), wxVERTICAL );


	// 'current joystick'
	m_Joystick_Device = new wxComboBox( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	sbSizer->Add( m_Joystick_Device, 0, wxALL|wxEXPAND, 5 );

	m_Joystick_Device->Append( wxT("<Default>") );
	m_Joystick_Device->SetSelection( 0 );

	conf_ptr = os_config_read_string("Controls", "CurrentJoystick", NULL);

	if (joystick_enabled) {
		int num_sticks = SDL_NumJoysticks();

		for (int i = 0; i < num_sticks; i++) {
			const char *jname = SDL_JoystickNameForIndex(i);

			if (jname) {
				m_Joystick_Device->Append(jname);

				if ( conf_ptr && !SDL_strcasecmp(conf_ptr, jname) ) {
					unsigned int sel = m_Joystick_Device->GetCount() - 1;

					m_Joystick_Device->SetSelection(sel);

					conf_ptr = NULL;
				}
			}
		}

		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}

	bSizer->Add( sbSizer, 0, wxALL|wxEXPAND, 5 );

	// 'force feedback'
	m_Joystick_FF = new wxCheckBox( panel, wxID_ANY, wxT("Enable Force Feedback"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_Joystick_FF, 0, wxALL|wxEXPAND, 5 );

	checked = os_config_read_uint("Controls", "EnableJoystickFF", 0);

	m_Joystick_FF->SetValue( (checked == 1) );

	// 'directional hit'
	m_Joystick_Directional = new wxCheckBox( panel, wxID_ANY, wxT("Enable directional hit effect (Force Feedback)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_Joystick_Directional, 0, wxALL|wxEXPAND, 5 );

	checked	= os_config_read_uint("Controls", "EnableHitEffect", 0);

	m_Joystick_Directional->SetValue( (checked == 1) );


	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("Joystick"), false );
}

void LauncherSetup::initTab_Speed(wxNotebook *parent)
{
	unsigned int detail_lvl = 0;

	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	// 'computer speed'
	wxStaticBoxSizer* sbSizer = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("Default Detail Level") ), wxVERTICAL );

	m_Speed_DefaultDetail = new wxComboBox( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN );
	sbSizer->Add( m_Speed_DefaultDetail, 0, wxALL|wxEXPAND, 5 );

	m_Speed_DefaultDetail->Append( wxT("Low") );
	m_Speed_DefaultDetail->Append( wxT("Medium") );
	m_Speed_DefaultDetail->Append( wxT("High") );
	m_Speed_DefaultDetail->Append( wxT("Very High") );

	detail_lvl = os_config_read_uint(NULL, "ComputerSpeed", 2);

	if (detail_lvl > 3) {
		detail_lvl = 0;
	}

	m_Speed_DefaultDetail->SetSelection(detail_lvl);


	bSizer->Add( sbSizer, 0, wxALL|wxEXPAND, 5 );

	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("Speed"), false );
}

void LauncherSetup::initTab_Network(wxNotebook* parent)
{
	const char *conf_ptr = NULL;
	unsigned int val = 0;

	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	// 'connection type'
	const wxString conn_types[] = { wxT("None"), wxT("Dialup Networking"), wxT("LAN/Direct Connection") };
	const int num_conn_types = sizeof( conn_types ) / sizeof( wxString );
	m_Network_Connection = new wxRadioBox( panel, wxID_ANY, wxT("Internet Connection"), wxDefaultPosition, wxDefaultSize, num_conn_types, conn_types, 1, wxRA_SPECIFY_COLS|wxSTATIC_BORDER );
	bSizer->Add( m_Network_Connection, 0, wxALL|wxEXPAND, 5 );

	conf_ptr = os_config_read_string("Network", "NetworkConnection", NULL);
	val = 0;

	if (conf_ptr) {
		if ( !SDL_strcasecmp(conf_ptr, "dialup") ) {
			val = 1;
		} else if ( !SDL_strcasecmp(conf_ptr, "lan") ) {
			val = 2;
		}
	}

	m_Network_Connection->SetSelection(val);

	// 'connection speed'
	const wxString speed_types[] = { wxT("None Specified"), wxT("Slower than 56K Modem"), wxT("56K Modem"), wxT("Single Channel ISDN"), wxT("Dual Channel ISDN, Cable Modems"), wxT("T1, ADSL, T3, etc.") };
	const int num_speed_types = sizeof( speed_types ) / sizeof( wxString );
	m_Network_Speed = new wxRadioBox( panel, wxID_ANY, wxT("Connection Speed"), wxDefaultPosition, wxDefaultSize, num_speed_types, speed_types, 1, wxRA_SPECIFY_COLS );
	bSizer->Add( m_Network_Speed, 0, wxALL|wxEXPAND, 5 );

	conf_ptr = os_config_read_string("Network", "ConnectionSpeed", NULL);
	val = 0;

	if (conf_ptr) {
		if ( !SDL_strcasecmp(conf_ptr, "Slow") ) {
			val = 1;
		} else if ( !SDL_strcasecmp(conf_ptr, "56K") ) {
			val = 2;
		} else if ( !SDL_strcasecmp(conf_ptr, "ISDN") ) {
			val = 3;
		} else if ( !SDL_strcasecmp(conf_ptr, "Cable") ) {
			val = 4;
		} else if ( !SDL_strcasecmp(conf_ptr, "Fast") ) {
			val = 5;
		}
	}

	m_Network_Speed->SetSelection(val);


	wxStaticBoxSizer* sbSizer = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("Misc") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer_port = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer_port->SetFlexibleDirection( wxBOTH );
	fgSizer_port->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	// 'port'
	wxStaticText* txt_port = new wxStaticText( panel, wxID_ANY, wxT("Force local port"), wxDefaultPosition, wxDefaultSize, 0 );
	txt_port->Wrap( -1 );
	fgSizer_port->Add( txt_port, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_Network_Port = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer_port->Add( m_Network_Port, 0, wxALL|wxEXPAND, 5 );

	wxIntegerValidator<unsigned short> vald( &m_port_validate );

	m_Network_Port->SetValidator(vald);

	sbSizer->Add( fgSizer_port, 1, wxEXPAND, 5 );

	val = os_config_read_uint("Network", "ForcePort", 0);

	if (val) {
		wxString sval = wxString::Format( wxT("%u"), val );

		m_Network_Port->SetValue(sval);
	}


	bSizer->Add( sbSizer, 0, wxALL|wxEXPAND, 5 );

	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("Network"), false );
}

void LauncherSetup::initTab_PXO(wxNotebook* parent)
{
	const char *conf_ptr = NULL;
	unsigned int checked = 0;

	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer = new wxStaticBoxSizer( new wxStaticBox( panel, wxID_ANY, wxT("PXO Account") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	// 'pxo login'
	wxStaticText* pxo_login = new wxStaticText( panel, wxID_ANY, wxT("Login"), wxDefaultPosition, wxDefaultSize, 0 );
	pxo_login->Wrap( -1 );
	fgSizer->Add( pxo_login, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_PXO_Username = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_PXO_Username, 0, wxALL|wxEXPAND, 5 );

	m_PXO_Username->SetMaxLength(32);

	conf_ptr = os_config_read_string("PXO", "Login", NULL);

	if (conf_ptr) {
		m_PXO_Username->SetValue(conf_ptr);
	}

	// 'pxo password'
	wxStaticText* pxo_pass = new wxStaticText( panel, wxID_ANY, wxT("Password"), wxDefaultPosition, wxDefaultSize, 0 );
	pxo_pass->Wrap( -1 );
	fgSizer->Add( pxo_pass, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_PXO_Password = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
	fgSizer->Add( m_PXO_Password, 0, wxALL|wxEXPAND, 5 );

	m_PXO_Password->SetMaxLength(32);

	conf_ptr = os_config_read_string("PXO", "Password", NULL);

	if (conf_ptr) {
		m_PXO_Password->SetValue(conf_ptr);
	}

	sbSizer->Add( fgSizer, 1, wxEXPAND, 5 );

	bSizer->Add( sbSizer, 0, wxALL|wxEXPAND, 5 );

	// 'multi version check'
	m_PXO_SkipVerify = new wxCheckBox( panel, wxID_ANY, wxT("Skip version check in PXO"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_PXO_SkipVerify, 0, wxALL, 5 );

	checked = os_config_read_uint("PXO", "SkipVerify", 0);

	m_PXO_SkipVerify->SetValue( (checked == 1) );

	// 'banners'
	m_PXO_Banners = new wxCheckBox( panel, wxID_ANY, wxT("PXO Banners"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_PXO_Banners, 0, wxALL, 5 );

	checked = os_config_read_uint("PXO", "Banners", 1);

	m_PXO_Banners->SetValue( (checked == 1) );


	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("PXO"), false );
}
