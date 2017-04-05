/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on the
 * source.
 *
*/

#include "stand_gui.h"
#include "osregistry.h"

#include "wx/statline.h"
#include "wx/filename.h"
#include "wx/stdpaths.h"
#include "wx/utils.h"
#include "wx/msgdlg.h"
#include "wx/process.h"
#include "wx/textfile.h"
#include "wx/socket.h"
#include "wx/cmdline.h"

#include "SDL.h"


// taken from psnet.h and psnet2.h
#ifdef MAKE_FS1
	#define DEFAULT_GAME_PORT 4000
#elif defined(FS2_DEMO)
	#define DEFAULT_GAME_PORT 7802
#else
	#define DEFAULT_GAME_PORT 7808
#endif

// taken from osregistry.cpp
const char *Osreg_company_name = "Volition";
#if defined(FS1_DEMO)
const char *Osreg_app_name = "FreeSpaceDemo";
#define PROFILE_NAME "FreeSpaceDemo.ini"
#elif defined(FS2_DEMO)
const char *Osreg_app_name = "FreeSpace2Demo";
#define PROFILE_NAME "FreeSpace2Demo.ini"
#elif defined(OEM_BUILD)
const char *Osreg_app_name = "FreeSpace2OEM";
#define PROFILE_NAME "FreeSpace2OEM.ini"
#elif defined(MAKE_FS1)
const char *Osreg_app_name = "FreeSpace";
#define PROFILE_NAME "FreeSpace.ini"
#else
const char *Osreg_app_name = "FreeSpace2";
#define PROFILE_NAME "FreeSpace2.ini"
#endif


IMPLEMENT_APP(StandaloneApp)

bool StandaloneApp::OnInit()
{
	if ( !wxApp::OnInit() ) {
		return false;
	}

	std_client = new Standalone(NULL);

	try {
		if ( !std_client->startFreeSpace(argc, argv) ) {
			throw "Unable to start FreeSpace";
		}

		wxMilliSleep(500);

		if ( !std_client->wsInitialize() ) {
			throw "Unable to initialize WebSocket";
		}
	} catch (const char *err) {
		wxMessageBox(err, "Error!", wxOK|wxICON_ERROR|wxCENTRE|wxSTAY_ON_TOP);

		return false;
	}

	std_client->Show(true);
	SetTopWindow(std_client);

	return true;
}

void StandaloneApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.SetCmdLine( wxT("") );
}

bool StandaloneApp::OnCmdLineParsed(wxCmdLineParser& WXUNUSED(parser))
{
	return true;
}

StandaloneTimer::StandaloneTimer(Standalone *stand)
{
	m_stand = stand;
}

StandaloneTimer::~StandaloneTimer()
{
}

void StandaloneTimer::Notify()
{
	m_stand->wsDoFrame();
}

StandPopup::StandPopup( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 300,100 ), wxDefaultSize );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->AddGrowableRow(0, 1);

	m_Label1 = new wxStaticText( this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	m_Label1->Wrap( -1 );
	fgSizer3->Add( m_Label1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_Label2 = new wxStaticText( this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	m_Label2->Wrap( -1 );
	fgSizer3->Add( m_Label2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer2->Add( fgSizer3, 1, wxALL|wxEXPAND, 10 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );

	this->Centre( wxBOTH );
}

StandPopup::~StandPopup()
{
}

wxBEGIN_EVENT_TABLE(Standalone, wxDialog)
	EVT_CLOSE(Standalone::OnClose)
	EVT_BUTTON(ID_B_SHUTDOWN, Standalone::OnShutdown)
	EVT_BUTTON(ID_B_KICK, Standalone::OnKick)
	EVT_BUTTON(ID_B_MREFRESH, Standalone::OnMissionRefresh)
	EVT_BUTTON(ID_B_RESET_ALL, Standalone::OnResetAll)
	EVT_SLIDER(ID_FPS_SLIDER, Standalone::OnFPSSel)
	EVT_TEXT_ENTER(ID_T_MSG, Standalone::OnServerMsg)
	EVT_TEXT_ENTER(ID_T_SERVER_NAME, Standalone::OnServerNameChange)
	EVT_TEXT_ENTER(ID_T_HOST_PASS, Standalone::OnHostPassChange)
	EVT_CHOICE(ID_C_P_PLAYERS, Standalone::OnPinfoPlayer)
wxEND_EVENT_TABLE()

Standalone::Standalone( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	fspid = 0;
	fsport = DEFAULT_GAME_PORT;
	stand_context = NULL;
	wsi_standalone = NULL;
	m_rate_limit = 0;

	m_timer = new StandaloneTimer(this);
	m_timer->Start(1000/30);

	m_popup = new StandPopup(this);
	m_popup->Show(false);

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxNotebook* nbook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	createTab_Server(nbook);
	createTab_Multi(nbook);
	createTab_Player(nbook);
	createTab_GodStuff(nbook);
	createTab_Debug(nbook);

	bSizer1->Add( nbook, 1, wxEXPAND | wxALL, 5 );

	wxButton* bshutdown = new wxButton( this, ID_B_SHUTDOWN, wxT("Shutdown"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( bshutdown, 0, wxALIGN_CENTER|wxALL, 5 );


	this->SetSizer( bSizer1 );
	bSizer1->Fit(this);
	bSizer1->SetSizeHints(this);

	this->Centre( wxBOTH );
}

Standalone::~Standalone()
{
	delete m_timer;
}

void Standalone::OnClose( wxCloseEvent& WXUNUSED(event) )
{
	Shutdown();
}

void Standalone::OnShutdown( wxCommandEvent& WXUNUSED(event) )
{
	Shutdown();
}

void Standalone::Shutdown()
{
	m_timer->Stop();

	if (stand_context) {
		std::string msg("shutdown");
		wsSend(msg);

		lws_service(stand_context, 50);

		lws_context_destroy(stand_context);
	}

	if ( wxProcess::Exists(fspid) ) {
		// wait a little bit, and if process still exists then kill it
		wxSleep(1);

		if ( wxProcess::Exists(fspid) ) {
			wxProcess::Kill(fspid, wxSIGTERM);
		}
	}

	Destroy();
}

void Standalone::OnServerNameChange(wxCommandEvent& WXUNUSED(event) )
{
	std::string msg("S:name ");

	msg.append( m_S_ServerName->GetValue().c_str() );

	wsSend(msg);
}

void Standalone::OnHostPassChange(wxCommandEvent& WXUNUSED(event) )
{
	std::string msg("S:pass ");

	msg.append( m_S_HostPass->GetValue().c_str() );

	wsSend(msg);
}

void Standalone::OnKick( wxCommandEvent& WXUNUSED(event) )
{
	std::string msg("S:kick ");

	long col, row;

	if ( !m_S_Connections->PositionToXY(m_S_Connections->GetInsertionPoint(), &col, &row) ) {
		return;
	}

	wxString line = m_S_Connections->GetLineText(row);

	if ( line.IsEmpty() ) {
		return;
	}

	wxArrayString ipaddr = wxSplit(line, ',');

	msg.append( ipaddr.Item(0).c_str() );

	wsSend(msg);
}

void Standalone::OnMissionRefresh( wxCommandEvent& WXUNUSED(event) )
{
	std::string msg("G:mrefresh");

	wsSend(msg);
}

void Standalone::OnResetAll( wxCommandEvent& WXUNUSED(event) )
{
	std::string msg("reset");

	wsSend(msg);
}

void Standalone::OnFPSSel( wxCommandEvent& WXUNUSED(event) )
{
	wxString fps = wxString::Format("%d", m_M_sliderFPS->GetValue());

	m_M_FPS->SetLabel(fps);

	std::string msg("M:fps ");
	msg.append(fps);

	wsSend(msg);
}

void Standalone::OnServerMsg( wxCommandEvent& WXUNUSED(event) )
{
	if ( m_GS_msg->GetValue().IsEmpty() ) {
		return;
	}

	std::string msg("G:smsg ");

	msg.append( m_GS_msg->GetValue().c_str() );

	// strip off return char which GetValue() has
	size_t pos = msg.find_last_not_of("\r\n");

	if (pos != std::string::npos) {
		msg.erase(pos+1);
	}

	wsSend(msg);

	m_GS_msg->Clear();
}

void Standalone::OnPinfoPlayer(wxCommandEvent& WXUNUSED(event) )
{
	int idx	= m_P_Players->GetCurrentSelection();

	if (idx == wxNOT_FOUND) {
		return;
	}

	std::string msg("P:info ");

	msg.append( m_P_Players->GetString(idx).c_str() );

	wsSend(msg);
}

void Standalone::createTab_Server(wxNotebook* parent)
{
	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer *panelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->AddGrowableCol(1, 1);

	wxStaticText* m_staticText6 = new wxStaticText( panel, wxID_ANY, wxT("Server Name"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_staticText6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_S_ServerName = new wxTextCtrl( panel, ID_T_SERVER_NAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_S_ServerName->SetMaxLength( 31 );
	fgSizer5->Add( m_S_ServerName, 0, wxALL|wxEXPAND, 5 );

	wxStaticText* m_staticText7 = new wxStaticText( panel, wxID_ANY, wxT("Host Password"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_staticText7, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_S_HostPass = new wxTextCtrl( panel, ID_T_HOST_PASS, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_S_HostPass->SetMaxLength( 16 );
	fgSizer5->Add( m_S_HostPass, 0, wxALL|wxEXPAND, 5 );

	bSizer->Add( fgSizer5, 0, wxEXPAND, 0 );

	wxStaticLine* m_staticline1 = new wxStaticLine( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizer7 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText9 = new wxStaticText( panel, wxID_ANY, wxT("# Connections :"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_staticText9, 0, wxALL, 5 );

	m_S_NumConn = new wxStaticText( panel, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_S_NumConn, 0, wxALL, 5 );

	bSizer->Add( fgSizer7, 0, wxEXPAND, 5 );

	wxStaticText* m_staticText8 = new wxStaticText( panel, wxID_ANY, wxT("Address and Ping"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_staticText8, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_S_Connections = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,300 ), wxTE_MULTILINE|wxTE_READONLY|wxTE_NO_VSCROLL );
	fgSizer3->Add( m_S_Connections, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bbSizer = new wxBoxSizer( wxVERTICAL );

	m_S_btnKick = new wxButton( panel, ID_B_KICK, wxT("Kick"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bbSizer->Add( m_S_btnKick, 1, wxALL, 5 );

	m_S_btnMRefresh = new wxButton( panel, ID_B_MREFRESH, wxT(" Refresh\nMissions"), wxDefaultPosition, wxDefaultSize, 0 );
	bbSizer->Add( m_S_btnMRefresh, 2, wxALL, 5 );

	m_S_btnResetAll = new wxButton( panel, ID_B_RESET_ALL, wxT("Reset All"), wxDefaultPosition, wxDefaultSize, 0 );
	bbSizer->Add( m_S_btnResetAll, 3, wxALL, 5 );

	fgSizer3->Add( bbSizer, 1, wxEXPAND, 5 );

	bSizer->Add( fgSizer3, 0, wxEXPAND, 5 );

	panelSizer->Add( bSizer, 1, wxALL|wxEXPAND, 5 );

	panel->SetSizer( panelSizer );
	panelSizer->Fit( panel );
	panelSizer->SetSizeHints( panel );

	parent->AddPage( panel, wxT("Server"), true );
}

void Standalone::createTab_Multi(wxNotebook* parent)
{
	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);

	m_M_sliderFPS = new wxSlider( panel, ID_FPS_SLIDER, 30, 15, 60, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer->Add( m_M_sliderFPS, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer8->AddGrowableCol(2, 1);

	wxStaticText* m_staticText71 = new wxStaticText( panel, wxID_ANY, wxT("Frame Cap : "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText71->Wrap( -1 );
	fgSizer8->Add( m_staticText71, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxPanel* fpsPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
	wxBoxSizer* fpsSizer = new wxBoxSizer(wxHORIZONTAL);
	m_M_FPS = new wxStaticText( fpsPanel, wxID_ANY, "30", wxDefaultPosition, wxSize(60, -1), 0 );
	m_M_FPS->Wrap( -1 );
	fpsSizer->Add( m_M_FPS, 0, wxALL, 2 );
	fpsPanel->SetSizer(fpsSizer);
	fpsSizer->Fit(fpsPanel);
	fgSizer8->Add(fpsPanel, 0, wxALL, 3);


	fgSizer8->Add( 0, 0, 1, wxEXPAND );

	wxStaticText* m_staticText81 = new wxStaticText( panel, wxID_ANY, wxT("Realized FPS : "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	fgSizer8->Add( m_staticText81, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxPanel*fpscapPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
	wxBoxSizer* fpscapSizer = new wxBoxSizer(wxHORIZONTAL);
	m_M_FPSRel = new wxStaticText( fpscapPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(60, -1), 0 );
	m_M_FPSRel->Wrap( -1 );
	fpscapSizer->Add( m_M_FPSRel, 0, wxALL, 2 );
	fpscapPanel->SetSizer(fpscapSizer);
	fpscapSizer->Fit(fpscapPanel);
	fgSizer8->Add(fpscapPanel, 0, wxALL, 3);


	bSizer->Add( fgSizer8, 0, wxEXPAND, 5 );

	wxStaticLine* m_staticline2 = new wxStaticLine( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );


	bSizer->Add( 0, 0, 0, wxALL, 20 );


	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText91 = new wxStaticText( panel, wxID_ANY, wxT("Mission Name : "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText91->Wrap( -1 );
	fgSizer9->Add( m_staticText91, 0, wxALL, 5 );

	wxPanel* mnPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
	wxBoxSizer* mnSizer = new wxBoxSizer(wxHORIZONTAL);
	m_M_MissionName = new wxStaticText( mnPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0 );
	m_M_MissionName->Wrap( -1 );
	mnSizer->Add( m_M_MissionName, 0, wxALL, 2 );
	mnPanel->SetSizer(mnSizer);
	mnSizer->Fit(mnPanel);
	fgSizer9->Add(mnPanel, 0, wxALL, 3);

	wxStaticText* m_staticText101 = new wxStaticText( panel, wxID_ANY, wxT("Mission Time : "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText101->Wrap( -1 );
	fgSizer9->Add( m_staticText101, 0, wxALL, 5 );

	wxPanel* mtPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
	wxBoxSizer* mtSizer = new wxBoxSizer(wxHORIZONTAL);
	m_M_MissionTime = new wxStaticText( mtPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0 );
	m_M_MissionTime->Wrap( -1 );
	mtSizer->Add( m_M_MissionTime, 0, wxALL, 2 );
	mtPanel->SetSizer(mtSizer);
	mtSizer->Fit(mtPanel);
	fgSizer9->Add(mtPanel, 0, wxALL, 3);


	bSizer->Add( fgSizer9, 0, wxEXPAND, 5 );


	bSizer->Add( 0, 0, 0, wxALL, 20 );

	wxBoxSizer* bSizer3 = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* bSizer4 = new wxBoxSizer(wxVERTICAL);

	wxStaticText* m_staticText11 = new wxStaticText( panel, wxID_ANY, wxT("Mission Goals"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	bSizer4->Add( m_staticText11, 0, wxALL, 5 );

	m_M_Goals = new wxTreeCtrl( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT|wxTR_NO_BUTTONS );

	wxImageList *goal_imgs = new wxImageList(16, 16);

	goal_imgs->Add( wxBitmap(goal_ord_xpm), wxColour(0xff, 0x0, 0xff) );
	goal_imgs->Add( wxBitmap(goal_none_xpm), wxColour(0xff, 0xff, 0xff) );
	goal_imgs->Add( wxBitmap(goal_inc_xpm), wxColour(0xff, 0xff, 0xff) );
	goal_imgs->Add( wxBitmap(goal_com_xpm), wxColour(0xff, 0xff, 0xff) );
	goal_imgs->Add( wxBitmap(goal_fail_xpm), wxColour(0xff, 0xff, 0xff) );

	m_M_Goals->AssignImageList(goal_imgs);

	wxTreeItemId root = m_M_Goals->AddRoot( wxT("Goals") );

	m_M_GoalItems[0] = m_M_Goals->AppendItem(root, wxT("Primary Objectives"), 0);
	m_M_GoalItems[1] = m_M_Goals->AppendItem(root, wxT("Secondary Objectives"), 0);
	m_M_GoalItems[2] = m_M_Goals->AppendItem(root, wxT("Bonus Objectives"), 0);

	bSizer4->Add( m_M_Goals, 1, wxALL|wxEXPAND, 5 );

	bSizer3->Add(bSizer4, 1, wxEXPAND);

	wxBoxSizer* bSizer5 = new wxBoxSizer(wxVERTICAL);

	wxStaticText* m_staticText12 = new wxStaticText( panel, wxID_ANY, wxT("Netgame Information"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer5->Add( m_staticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 2, 0, 15 );
	fgSizer11->AddGrowableRow(0, 1);
	fgSizer11->AddGrowableRow(1, 1);
	fgSizer11->AddGrowableRow(2, 1);
	fgSizer11->AddGrowableRow(3, 1);

	wxStaticText* m_staticText13 = new wxStaticText( panel, wxID_ANY, wxT("Max Players"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer11->Add( m_staticText13, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_M_ngMaxPlayers = new wxStaticText( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_M_ngMaxPlayers->Wrap( -1 );
	fgSizer11->Add( m_M_ngMaxPlayers, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* m_staticText15 = new wxStaticText( panel, wxID_ANY, wxT("Max Observers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer11->Add( m_staticText15, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_M_ngMaxObservers = new wxStaticText( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_M_ngMaxObservers->Wrap( -1 );
	fgSizer11->Add( m_M_ngMaxObservers, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* m_staticText17 = new wxStaticText( panel, wxID_ANY, wxT("Security"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizer11->Add( m_staticText17, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_M_ngSecurity = new wxStaticText( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_M_ngSecurity->Wrap( -1 );
	fgSizer11->Add( m_M_ngSecurity, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* m_staticText19 = new wxStaticText( panel, wxID_ANY, wxT("Respawns"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizer11->Add( m_staticText19, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_M_ngRespawns = new wxStaticText( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_M_ngRespawns->Wrap( -1 );
	fgSizer11->Add( m_M_ngRespawns, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	bSizer5->Add( fgSizer11, 1, wxEXPAND );

	bSizer3->Add(bSizer5, 1, wxEXPAND);

	bSizer->Add( bSizer3, 1, wxEXPAND, 5 );

	panelSizer->Add(bSizer, 1, wxALL|wxEXPAND, 5);

	panel->SetSizer( panelSizer );
	panelSizer->Fit( panel );
	panelSizer->SetSizeHints( panel );

	parent->AddPage( panel, wxT("Multi-Player") );
}

void Standalone::createTab_Player(wxNotebook* parent)
{
	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText211 = new wxStaticText( panel, wxID_ANY, wxT("Player"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	bSizer21->Add( m_staticText211, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_choice11Choices;
	m_P_Players = new wxChoice( panel, ID_C_P_PLAYERS, wxDefaultPosition, wxSize(200, -1), m_choice11Choices, 0 );
	m_P_Players->SetSelection( wxNOT_FOUND );
	bSizer21->Add( m_P_Players, 0, wxALL, 5 );

	bSizer->Add( bSizer21, 0, wxEXPAND );


	wxStaticLine* m_staticline4 = new wxStaticLine( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer->Add( m_staticline4, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizer14;
	fgSizer14 = new wxFlexGridSizer( 0, 2, 0, 0 );

	wxStaticText* m_staticText26 = new wxStaticText( panel, wxID_ANY, wxT("Ship Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer14->Add( m_staticText26, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxPanel* stPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
	wxBoxSizer* stSizer = new wxBoxSizer(wxHORIZONTAL);
	m_P_ShipType = new wxStaticText( stPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(150, -1), 0 );
	m_P_ShipType->Wrap( -1 );
	stSizer->Add(m_P_ShipType, 0, wxALL, 2);
	stPanel->SetSizer(stSizer);
	stSizer->Fit(stPanel);
	fgSizer14->Add( stPanel, 0, wxALL, 5 );

	wxStaticText* m_staticText28 = new wxStaticText( panel, wxID_ANY, wxT("Avg Ping"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgSizer14->Add( m_staticText28, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxPanel* apPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
	wxBoxSizer* apSizer = new wxBoxSizer(wxHORIZONTAL);
	m_P_AvgPing = new wxStaticText( apPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(150, -1), 0 );
	m_P_AvgPing->Wrap( -1 );
	apSizer->Add(m_P_AvgPing, 0, wxALL, 2);
	apPanel->SetSizer(apSizer);
	apSizer->Fit(apPanel);
	fgSizer14->Add( apPanel, 0, wxALL, 5 );

	bSizer->Add( fgSizer14, 0, wxEXPAND );

	bSizer->Add( 0, 0, 0, wxALL, 10);

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	//
	// All-Time Stats
	//

	wxBoxSizer* atsboxSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* atsText = new wxStaticText( panel, wxID_ANY, wxT("All Time Stats") );
	atsboxSizer->Add( atsText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	wxFlexGridSizer* atsgSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	atsgSizer->AddGrowableCol(1, 1);

	// primary shots
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary Shots"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsPriShots = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsPriShots->Wrap( -1 );
		p1sizer->Add( m_P_atsPriShots, 1, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsPriHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsPriHits->Wrap( -1 );
		p1sizer->Add( m_P_atsPriHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary BH hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary BH Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsPriBHHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsPriBHHits->Wrap( -1 );
		p1sizer->Add( m_P_atsPriBHHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsPriHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsPriHitPer->Wrap( -1 );
		p1sizer->Add( m_P_atsPriHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary BH hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary BH Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsPriBHHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsPriBHHitPer->Wrap( -1 );
		p1sizer->Add( m_P_atsPriBHHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary shots
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary Shots"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsSecShots = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsSecShots->Wrap( -1 );
		p1sizer->Add( m_P_atsSecShots, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsSecHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsSecHits->Wrap( -1 );
		p1sizer->Add( m_P_atsSecHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary BH hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary BH Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsSecBHHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsSecBHHits->Wrap( -1 );
		p1sizer->Add( m_P_atsSecBHHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsSecHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsSecHitPer->Wrap( -1 );
		p1sizer->Add( m_P_atsSecHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary BH hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary BH Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsSecBHHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsSecBHHitPer->Wrap( -1 );
		p1sizer->Add( m_P_atsSecBHHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// assists
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Assists"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		atsgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_atsAssists = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_atsAssists->Wrap( -1 );
		p1sizer->Add( m_P_atsAssists, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		atsgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	atsboxSizer->Add( atsgSizer, 1, wxEXPAND );

	bSizer6->Add( atsboxSizer, 1, wxEXPAND, 5 );

	wxStaticLine* m_staticline3 = new wxStaticLine( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer6->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );

	//
	// Mission Stats
	//

	wxBoxSizer* msboxSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* msText = new wxStaticText( panel, wxID_ANY, wxT("Mission Stats") );
	msboxSizer->Add( msText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	wxFlexGridSizer* msgSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	msgSizer->AddGrowableCol(1, 1);

	// primary shots
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary Shots"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msPriShots = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msPriShots->Wrap( -1 );
		p1sizer->Add( m_P_msPriShots, 1, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msPriHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msPriHits->Wrap( -1 );
		p1sizer->Add( m_P_msPriHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary BH hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary BH Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msPriBHHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msPriBHHits->Wrap( -1 );
		p1sizer->Add( m_P_msPriBHHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msPriHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msPriHitPer->Wrap( -1 );
		p1sizer->Add( m_P_msPriHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// primary BH hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Primary BH Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msPriBHHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msPriBHHitPer->Wrap( -1 );
		p1sizer->Add( m_P_msPriBHHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary shots
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary Shots"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msSecShots = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msSecShots->Wrap( -1 );
		p1sizer->Add( m_P_msSecShots, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msSecHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msSecHits->Wrap( -1 );
		p1sizer->Add( m_P_msSecHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary BH hits
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary BH Hits"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msSecBHHits = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msSecBHHits->Wrap( -1 );
		p1sizer->Add( m_P_msSecBHHits, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msSecHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msSecHitPer->Wrap( -1 );
		p1sizer->Add( m_P_msSecHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// secondary BH hit %
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Secondary BH Hit %"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msSecBHHitPer = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msSecBHHitPer->Wrap( -1 );
		p1sizer->Add( m_P_msSecBHHitPer, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	// assists
	{
		wxStaticText* itxt = new wxStaticText( panel, wxID_ANY, wxT("Assists"), wxDefaultPosition, wxDefaultSize, 0 );
		itxt->Wrap( -1 );
		msgSizer->Add( itxt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
		wxPanel* p1 = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		wxBoxSizer* p1sizer = new wxBoxSizer( wxHORIZONTAL );
		m_P_msAssists = new wxStaticText( p1, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
		m_P_msAssists->Wrap( -1 );
		p1sizer->Add( m_P_msAssists, 0, wxALL|wxEXPAND, 1);
		p1->SetSizer( p1sizer );
		p1sizer->Fit( p1 );
		msgSizer->Add( p1, 0, wxALL|wxEXPAND, 1 );
	}

	msboxSizer->Add( msgSizer, 1, wxEXPAND );

	bSizer6->Add( msboxSizer, 1, wxEXPAND );

	bSizer->Add( bSizer6, 1, wxEXPAND );

	panelSizer->Add( bSizer, 1, wxALL|wxEXPAND, 5 );

	panel->SetSizer( panelSizer );
	panelSizer->Fit( panel );
	panelSizer->SetSizeHints(panel);

	parent->AddPage( panel, wxT("Player Info") );
}

void Standalone::createTab_GodStuff(wxNotebook* parent)
{
	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText21 = new wxStaticText( panel, wxID_ANY, wxT("Player"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_staticText21, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choice1Choices;
	m_GS_Players = new wxChoice( panel, wxID_ANY, wxDefaultPosition, wxSize(200, -1), m_choice1Choices, 0 );
	m_GS_Players->SetSelection( wxNOT_FOUND );
	bSizer2->Add( m_GS_Players, 0, wxALL|wxEXPAND, 5 );

	bSizer->Add( bSizer2, 0, wxEXPAND, 5 );

	bSizer->Add( 0, 0, 0, wxALL, 10 );

	wxStaticText* m_staticText22 = new wxStaticText( panel, wxID_ANY, wxT("Server Message"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer->Add( m_staticText22, 0, wxALL, 5 );

	m_GS_msg = new wxTextCtrl( panel, ID_T_MSG, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_GS_msg->SetMaxLength(150);
	bSizer->Add( m_GS_msg, 0, wxALL|wxEXPAND, 5 );

	bSizer->Add( 0, 0, 0, wxALL, 5 );

	m_GS_Messages = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	bSizer->Add( m_GS_Messages, 1, wxALL|wxEXPAND, 5 );

	panelSizer->Add( bSizer, 1, wxALL|wxEXPAND, 5 );

	panel->SetSizer( panelSizer );
	panelSizer->Fit( panel );
	panelSizer->SetSizeHints( panel );

	parent->AddPage( panel, wxT("God Stuff") );
}

void Standalone::createTab_Debug(wxNotebook* parent)
{
	wxPanel* panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* hbSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText1 = new wxStaticText( panel, wxID_ANY, wxT("Standalone State :"), wxDefaultPosition, wxDefaultSize, 0 );
	hbSizer->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxPanel* p1 = new wxPanel( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER );

	wxBoxSizer* p1Sizer = new wxBoxSizer( wxHORIZONTAL );

	m_D_State = new wxStaticText( p1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	p1Sizer->Add( m_D_State, 1, wxALL|wxEXPAND, 2);

	p1->SetSizer( p1Sizer );
	p1Sizer->Fit( p1 );

	hbSizer->Add( p1, 1, wxALL|wxEXPAND, 5 );

	bSizer->Add( hbSizer, 0, wxALL|wxEXPAND, 5 );

	panel->SetSizer( bSizer );
	panel->Layout();

	bSizer->Fit( panel );

	parent->AddPage( panel, wxT("Debug") );
}

bool Standalone::startFreeSpace(int argc, wxCmdLineArgsArray &argv)
{
	wxString epath = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath(true);
	bool cmd_port = false;

	epath.Append( wxT("fs") );

#ifndef MAKE_FS1
	epath.Append( wxT("2") );
#endif

#if defined(FS1_DEMO) || defined(FS2_DEMO)
	epath.Append( wxT("demo") );
#endif

#ifdef _WIN32
	epath.Append( wxT(".exe") );
#endif

	epath.Append( wxT(" -standalone") );

	for (int i = 1; i < argc; i++) {
		wxString arg( argv[i] );

		// check if -port argument and set var
		if ( (argc > i+1) && (arg.Contains( wxT("-port") ) || arg.IsSameAs( wxT("-o") )) ) {
			fsport = wxAtoi(argv[i+1]);
			cmd_port = true;
		}

		epath.Append( wxT(" ") );
		epath.Append(arg);
	}

	// if port isn't specified on cmdline, check ini file for "ForcePort" value
	if ( !cmd_port ) {
		char *u_path = SDL_GetPrefPath(Osreg_company_name, Osreg_app_name);

		if (u_path) {
			wxFileName ini_name(u_path, PROFILE_NAME);
			wxTextFile fs_ini( ini_name.GetFullPath() );

			if ( fs_ini.Open() ) {
				wxString line;

				for (line = fs_ini.GetLastLine(); fs_ini.GetCurrentLine() > 0; line = fs_ini.GetPrevLine()) {
					wxArrayString chk = wxSplit(line, '=');

					if (chk.GetCount() == 2) {
						if (chk.Item(0) == "ForcePort") {
							int port = wxAtoi( chk.Item(1) );

							if (port > 0) {
								fsport = port;
							}

							break;
						}
					}
				}

				fs_ini.Close();
			}

			SDL_free(u_path);
		}
	}

	// test if socket is in use (in case exising instance is running on same port)
	wxDatagramSocket *sock;
	wxIPV4address addr;

	addr.AnyAddress();
	addr.Service( (unsigned short)fsport );

	sock = new wxDatagramSocket(addr);

	bool isok = sock->IsOk();

	sock->Destroy();

	if ( !isok ) {
		throw "Unable to start FreeSpace\n\nAn instance is already running";
	}

	// start game executable
	fspid = wxExecute(epath, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER | wxEXEC_HIDE_CONSOLE);

	return (fspid > 0);
}

void Standalone::ResetAll()
{
	m_popup->Show(false);

	m_S_ServerName->Clear();
	m_S_HostPass->Clear();
	m_S_NumConn->SetLabel( wxT("0") );
	m_S_Connections->Clear();

	m_M_sliderFPS->SetValue(30);
	m_M_FPS->SetLabel("30");
	m_M_FPSRel->SetLabel( wxT("0.0") );
	m_M_MissionName->SetLabel("");
	m_M_MissionTime->SetLabel("");
	m_M_ngMaxPlayers->SetLabel("");
	m_M_ngMaxObservers->SetLabel("");
	m_M_ngSecurity->SetLabel("");
	m_M_ngRespawns->SetLabel("");

	for (int idx = 0; idx < 3; idx++) {
		m_M_Goals->DeleteChildren( m_M_GoalItems[idx] );
	}

	m_P_Players->Clear();
	m_P_ShipType->SetLabel("");
	m_P_AvgPing->SetLabel("");
	m_P_atsPriShots->SetLabel("");
	m_P_atsPriHits->SetLabel("");
	m_P_atsPriBHHits->SetLabel("");
	m_P_atsPriHitPer->SetLabel("");
	m_P_atsPriBHHitPer->SetLabel("");
	m_P_atsSecShots->SetLabel("");
	m_P_atsSecHits->SetLabel("");
	m_P_atsSecBHHits->SetLabel("");
	m_P_atsSecHitPer->SetLabel("");
	m_P_atsSecBHHitPer->SetLabel("");
	m_P_atsAssists->SetLabel("");
	m_P_msPriShots->SetLabel("");
	m_P_msPriHits->SetLabel("");
	m_P_msPriBHHits->SetLabel("");
	m_P_msPriHitPer->SetLabel("");
	m_P_msPriBHHitPer->SetLabel("");
	m_P_msSecShots->SetLabel("");
	m_P_msSecHits->SetLabel("");
	m_P_msSecBHHits->SetLabel("");
	m_P_msSecHitPer->SetLabel("");
	m_P_msSecBHHitPer->SetLabel("");
	m_P_msAssists->SetLabel("");

	m_GS_Players->Clear();
	m_GS_msg->Clear();
	m_GS_Messages->Clear();

	m_D_State->SetLabel("");
}

static int callback_standalone_client(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	#define MAX_BUF_SIZE	1050
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_BUF_SIZE + LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	int rval;
	int size;

	switch (reason) {
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			lws_callback_on_writable(wsi);
			break;

		case LWS_CALLBACK_CLOSED:
			wxGetApp().Client().wsDisconnect();
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			wxGetApp().Client().wsMessage( (const char *)in, len );
			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE: {
			while ( !wxGetApp().Client().wsGetSendBuffer().empty() ) {
				std::string msg = wxGetApp().Client().wsGetSendBuffer().front();

				size = wxStrlcpy((char *)p, msg.c_str(), MAX_BUF_SIZE);

				rval = lws_write(wsi, p, size, LWS_WRITE_TEXT);

				if (rval < size) {
					lwsl_err("ERROR sending buffer!\n");
					return -1;
				}

				wxGetApp().Client().wsGetSendBuffer().pop_front();

				if ( lws_send_pipe_choked(wsi) ) {
					lws_callback_on_writable(wsi);

					break;
				}
			}

			break;
		}

		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			wxGetApp().Client().wsDisconnect();
			break;

		default:
			break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
	{
		"standalone",
		callback_standalone_client,
		0,
		0
	},
	{ NULL, NULL, 0, 0 }
};

bool Standalone::wsInitialize()
{
	struct lws_context_creation_info info;

	memset(&info, 0, sizeof(info));
	memset(&ccinfo, 0, sizeof(ccinfo));

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	stand_context = lws_create_context(&info);

	if (stand_context == NULL) {
		return false;
	}

	ccinfo.context = stand_context;
	ccinfo.address = "127.0.0.1";
	ccinfo.port = fsport;
	ccinfo.path = "/";
	ccinfo.host = ccinfo.address;
	ccinfo.origin = ccinfo.address;
	ccinfo.ietf_version_or_minus_one = -1;
	ccinfo.protocol = protocols[0].name;

	// async call, actual connection to occur later
	wsi_standalone = lws_client_connect_via_info(&ccinfo);

	lws_service(stand_context, 0);

	return true;
}

void Standalone::wsDoFrame()
{
	if ( !stand_context ) {
		return;
	}

	// if connection is lost attempt reconnect every 2 seconds
	if ( !wsi_standalone && !(++m_rate_limit % 60) ) {
		// restart game process if it terminated
		if ( !wxProcess::Exists(fspid) ) {
			wxCmdLineArgsArray empty;

			try {
				startFreeSpace(0, empty);
			} catch (const char *err) {
				wxMessageBox(err, "Error!", wxOK|wxICON_ERROR|wxCENTRE|wxSTAY_ON_TOP);

				Shutdown();
			}

			wxMilliSleep(500);
		}

		wsi_standalone = lws_client_connect_via_info(&ccinfo);
		m_rate_limit = 0;
	}

	lws_service(stand_context, 0);
}

void Standalone::wsDisconnect()
{
	wsi_standalone = NULL;
}

void Standalone::wsMessage(const char *msg, size_t len)
{
	if (msg == NULL || len < 5) {
		return;
	}

	if ( !wxStrcmp(msg, "reset") ) {
		ResetAll();
		return;
	}

	if ( !wxStrncmp(msg, "popup ", 6) ) {
		if (len == 6) {
			m_popup->Show(false);
		} else {
			wxArrayString popmsg = wxSplit(msg+6, ';');

			m_popup->SetTitle( popmsg.Item(0) );
			m_popup->SetLabel1( popmsg.Item(1) );
			m_popup->SetLabel2( popmsg.Item(2) );

			m_popup->Layout(); // layout required to deal with label size changes
			m_popup->CenterOnParent();
			m_popup->Show(true);
		}

		return;
	}

	char mtype = msg[0];

	if (mtype == 'T') {
		SetTitle(msg+2);
	} else if (mtype == 'D') {
		m_D_State->SetLabel(msg+2);
	}
	// server tab
	else if (mtype == 'S') {
		wxString cmd(msg+2, 4);

		if (cmd == "name") {
			m_S_ServerName->SetValue(msg+7);
		} else if (cmd == "pass") {
			m_S_HostPass->SetValue(msg+7);
		} else if (cmd == "conn") {
			wxArrayString conns = wxSplit(msg+7, ';');

			size_t n_conn = conns.size();

			m_S_NumConn->SetLabel( wxString::Format("%u", (unsigned int)n_conn) );

			m_S_Connections->Clear();

			for (size_t idx = 0; idx < n_conn; idx++) {
				wxArrayString m_conn = wxSplit(conns.Item(idx), ',');

				m_P_Players->Append( m_conn.Item(0) );
				m_GS_Players->Append( m_conn.Item(0) );

				m_S_Connections->AppendText( m_conn.Item(1) + wxT(", \n") );
			}
		} else if (cmd == "ping") {
			wxArrayString ping_list = wxSplit(msg+7, ',');

			size_t n_pings = ping_list.size();

			size_t offset_pos = 0;
			long col = 0, row = 0;

			m_S_Connections->PositionToXY(m_S_Connections->GetInsertionPoint(), &col, &row);

			for (size_t idx = 0; idx < n_pings; idx++) {
				size_t from_pos = m_S_Connections->GetValue().find(", ", offset_pos);

				if (from_pos == wxString::npos) {
					break;
				} else {
					from_pos += 2;
				}

				size_t to_pos = m_S_Connections->GetValue().find_first_of("\n", from_pos);

				m_S_Connections->Replace(from_pos, to_pos, ping_list.Item(idx));

				offset_pos = to_pos;
			}

			// move intersion point to start of original line (avoids accidental kick)
			m_S_Connections->SetInsertionPoint( m_S_Connections->XYToPosition(0, row) );
		}
	}
	// multi-player tab
	else if (mtype == 'M') {
		wxString cmd(msg+2, 4);

		if (cmd == "name") {
			m_M_MissionName->SetLabel(msg+7);
		} else if (cmd == "time") {
			m_M_MissionTime->SetLabel(msg+7);
		} else if (cmd == "info") {
			wxArrayString ng_info = wxSplit(msg+7, ',');

			m_M_ngMaxPlayers->SetLabel( ng_info.Item(0) );
			m_M_ngMaxObservers->SetLabel( ng_info.Item(1) );
			m_M_ngSecurity->SetLabel( ng_info.Item(2) );
			m_M_ngRespawns->SetLabel( ng_info.Item(3) );
		} else if (cmd == "rfps") {
			m_M_FPSRel->SetLabel(msg+7);
		} else if (cmd == "goal") {
			wxArrayString objectives = wxSplit(msg+7, ';');

			size_t n_objectives = objectives.size();
			wxASSERT(n_objectives == 3);

			for (size_t idx = 0; idx < n_objectives; idx++) {
				wxArrayString goals = wxSplit( objectives.Item(idx), ',' );

				size_t n_goals = goals.size();

				m_M_Goals->DeleteChildren(m_M_GoalItems[idx]);

				for (size_t j = 0; j < n_goals; j++) {
					char status = goals.Item(j).GetChar(0);
					wxString goal = goals.Item(j).substr(2);
					int img = 1;

					switch (status) {
						case 'i': {
							if (goal == "none") {
								img = 1;
							} else {
								img = 2;
							}

							break;
						}

						case 'c':
							img = 3;
							break;

						case 'f':
							img = 4;
							break;

						default:
							break;
					}

					m_M_Goals->AppendItem(m_M_GoalItems[idx], goal, img);
				}
			}

			m_M_Goals->ExpandAll();
		}
	}
	// player tab
	else if (mtype == 'P') {
		wxString cmd(msg+2, 4);

		if (cmd == "info") {
			wxArrayString pinfo = wxSplit(msg+7, ';');
			wxArrayString stats;

			m_P_ShipType->SetLabel( pinfo.Item(0) );
			m_P_AvgPing->SetLabel( pinfo.Item(1) );

			stats = wxSplit(pinfo.Item(2), ',');

			m_P_atsPriShots->SetLabel( stats.Item(0) );
			m_P_atsPriHits->SetLabel( stats.Item(1) );
			m_P_atsPriBHHits->SetLabel( stats.Item(2) );
			m_P_atsPriHitPer->SetLabel( stats.Item(3) );
			m_P_atsPriBHHitPer->SetLabel( stats.Item(4) );
			m_P_atsSecShots->SetLabel( stats.Item(5) );
			m_P_atsSecHits->SetLabel( stats.Item(6) );
			m_P_atsSecBHHits->SetLabel( stats.Item(7) );
			m_P_atsSecHitPer->SetLabel( stats.Item(8) );
			m_P_atsSecBHHitPer->SetLabel( stats.Item(9) );
			m_P_atsAssists->SetLabel( stats.Item(10) );

			stats = wxSplit(pinfo.Item(3), ',');

			m_P_msPriShots->SetLabel( stats.Item(0) );
			m_P_msPriHits->SetLabel( stats.Item(1) );
			m_P_msPriBHHits->SetLabel( stats.Item(2) );
			m_P_msPriHitPer->SetLabel( stats.Item(3) );
			m_P_msPriBHHitPer->SetLabel( stats.Item(4) );
			m_P_msSecShots->SetLabel( stats.Item(5) );
			m_P_msSecHits->SetLabel( stats.Item(6) );
			m_P_msSecBHHits->SetLabel( stats.Item(7) );
			m_P_msSecHitPer->SetLabel( stats.Item(8) );
			m_P_msSecBHHitPer->SetLabel( stats.Item(9) );
			m_P_msAssists->SetLabel( stats.Item(10) );
		}
	}
	// god stuff tab
	else if (mtype == 'G') {
		wxString cmd(msg+2, 4);

		if (cmd == "mesg") {
			m_GS_Messages->AppendText(msg+7);
		}
	}
}

void Standalone::wsSend(std::string &msg)
{
	send_buf.push_back(msg);

	if (wsi_standalone) {
		lws_callback_on_writable(wsi_standalone);
	}
}
