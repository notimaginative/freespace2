/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

// PofView.cpp : Defines the class behaviors for the application.
//

#include "pofview.h"

#include "wx/filename.h"

#include "pstypes.h"
#include "2d.h"
#include "bmpman.h"
#include "systemvars.h"
#include "model.h"
#include "cfile.h"
#include "timer.h"
#include "key.h"
#include "mouse.h"

#include "res/pofview_ico.xpm"
#include "res/tool_about.xpm"
#include "res/tool_damaged.xpm"
#include "res/tool_debris.xpm"
#include "res/tool_detail1.xpm"
#include "res/tool_detail2.xpm"
#include "res/tool_detail3.xpm"
#include "res/tool_detail4.xpm"
#include "res/tool_detail5.xpm"
#include "res/tool_detail6.xpm"
#include "res/tool_lights.xpm"
#include "res/tool_tree.xpm"


int Pofview_running = 1;

bool in_dialog = false;


class PofViewApp: public wxApp
{
	public:
		virtual bool OnInit();
};


IMPLEMENT_APP(PofViewApp)

bool PofViewApp::OnInit()
{
	PofViewFrame *frame = new PofViewFrame(NULL);
	frame->Show(true);
	SetTopWindow(frame);

	return true;
}

BEGIN_EVENT_TABLE( PofViewFrame, wxFrame )
	EVT_MENU( wxID_OPEN, PofViewFrame::OnFileOpen )
	EVT_MENU( wxID_CLOSE, PofViewFrame::OnFileClose )
	EVT_MENU( wxID_EXIT, PofViewFrame::OnExit )
	EVT_MENU( ID_M_VIEW_TOOLBAR, PofViewFrame::OnViewToolbar )
	EVT_MENU( ID_M_VIEW_STATUSBAR, PofViewFrame::OnViewStatusBar )
	EVT_MENU( wxID_ABOUT, PofViewFrame::OnHelpAbout )
	EVT_TOOL( ID_M_TOOLABOUT, PofViewFrame::OnHelpAbout )
	EVT_TOOL_RANGE( ID_M_TOOLDEBRIS, ID_M_TOOLDETAIL6, PofViewFrame::OnSetDetail )
	EVT_TOOL( ID_M_TOOLSHOWTREE, PofViewFrame::OnShowTree )
END_EVENT_TABLE()

PofViewFrame::PofViewFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
	: wxFrame( parent, id, title, pos, size, style )
{
	m_model_num = -1;
	m_glContext = NULL;
	m_canvas = NULL;

	pofview_initted = false;

	this->SetSizeHints( wxSize( 640,480 ), wxDefaultSize );

	this->SetIcon(wxICON(pofview_ico));

	MakeMenuBar();
	MakeToolBar();

	SetToolDefaults();

	this->Show(true);

	m_statusBar1 = this->CreateStatusBar( 1, wxSTB_DEFAULT_STYLE, wxID_ANY );

	this->Centre( wxBOTH );
}

PofViewFrame::~PofViewFrame()
{
	if (m_glContext) {
		delete m_glContext;
	}
}

void PofViewFrame::MakeMenuBar()
{
	m_menubar1 = new wxMenuBar( 0 );

	// 'File' menu
	m_menuFile = new wxMenu();

	m_menuFile->Append( wxID_OPEN );
	m_menuFile->Append( wxID_CLOSE );

	m_menuFile->AppendSeparator();

	m_menuFile->Append( wxID_EXIT );

	m_menubar1->Append( m_menuFile, wxT("File") );

	// 'View' menu
	m_menuView = new wxMenu();

	m_menuView->AppendCheckItem( ID_M_VIEW_TOOLBAR, "&Toolbar", "Show or hide the toolbar" );
	m_menuView->AppendCheckItem( ID_M_VIEW_STATUSBAR, "&Status Bar", "Show or hide the status bar" );

	m_menuView->Check( ID_M_VIEW_TOOLBAR, true );
	m_menuView->Check( ID_M_VIEW_STATUSBAR, true );

	m_menuView->AppendSeparator();

	m_menuView->AppendCheckItem( ID_M_VIEW_OUTLINE, "&Outline", "Toggles outline mode" );
	m_menuView->AppendCheckItem( ID_M_VIEW_LIGHTING, "&Lighting", "Toggle lighting" );
	m_menuView->AppendCheckItem( ID_M_VIEW_PIVTOS, "&Pivots/Bounding Boxes", "Toggle pivots and bounding boxes" );
	m_menuView->AppendCheckItem( ID_M_VIEW_PATHS, "P&aths", "Show all the paths" );
	m_menuView->AppendCheckItem( ID_M_VIEW_RADIUS, "&Radius", "Show the object's radius." );
	m_menuView->AppendCheckItem( ID_M_VIEW_THRUSTERS, "T&hrusters", "Toggles thrusters" );
	m_menuView->AppendCheckItem( ID_M_VIEW_TEXTURING, "Texturing" );
	m_menuView->AppendCheckItem( ID_M_VIEW_SMOOTHING, "Smoothing" );
	m_menuView->AppendCheckItem( ID_M_VIEW_SHIELDS, "Shields" );
	m_menuView->AppendCheckItem( ID_M_VIEW_INVISIBLE, "Invisible Faces" );
	m_menuView->AppendCheckItem( ID_M_VIEW_BAYPATHS, "Bay Paths" );
	m_menuView->AppendCheckItem( ID_M_VIEW_AUTOCENTER, "Autocenter" );

	m_menubar1->Append( m_menuView, wxT("View") );

	// 'Help' menu
	m_menuHelp = new wxMenu();

	m_menuHelp->Append( wxID_ABOUT, "&About PofView...");

	m_menubar1->Append( m_menuHelp, wxT("Help") );


	this->SetMenuBar( m_menubar1 );
}

void PofViewFrame::MakeToolBar()
{
	m_toolBar1 = this->CreateToolBar( wxTB_HORIZONTAL, wxID_ANY );

	m_toolBar1->AddTool( wxID_OPEN, wxEmptyString, wxArtProvider::GetBitmap( wxART_FILE_OPEN, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxT("Open an existing document"), wxEmptyString, NULL );

	m_toolBar1->AddSeparator();

	m_toolBar1->AddTool( ID_M_TOOLABOUT, wxEmptyString, wxBitmap( tool_about_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Display program information, version number and copyright"), wxEmptyString, NULL );

	m_toolBar1->AddSeparator();

	m_toolBar1->AddTool( ID_M_TOOLDEBRIS, wxEmptyString, wxBitmap( tool_debris_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Show debris pieces"), wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLDETAIL1, wxEmptyString, wxBitmap( tool_detail1_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Set to Detail Level 1"), wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLDETAIL2, wxEmptyString, wxBitmap( tool_detail2_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Set to Detail Level 2"), wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLDETAIL3, wxEmptyString, wxBitmap( tool_detail3_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Set to Detail Level 3"), wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLDETAIL4, wxEmptyString, wxBitmap( tool_detail4_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Set to Detail Level 4"), wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLDETAIL5, wxEmptyString, wxBitmap( tool_detail5_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Set to Detail Level 5"), wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLDETAIL6, wxEmptyString, wxBitmap( tool_detail6_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Set to Detail Level 6"), wxEmptyString, NULL );

	m_toolBar1->AddSeparator();

	m_toolBar1->AddTool( ID_M_TOOLSHOWTREE, wxEmptyString, wxBitmap( tool_tree_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );
	m_toolBar1->AddTool( ID_M_TOOLSHOWDAMAGED, wxEmptyString, wxBitmap( tool_damaged_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Toggles damaged subobjects on/off"), wxEmptyString, NULL );

	m_toolBar1->AddSeparator();

	m_toolBar1->AddTool( ID_M_TOOLTOGGLELIGHTS, wxEmptyString, wxBitmap( tool_lights_xpm ), wxNullBitmap, wxITEM_CHECK, wxT("Toggles lights"), wxEmptyString, NULL );

	m_toolBar1->Realize();
}

void PofViewFrame::SetToolDefaults()
{
	m_toolBar1->EnableTool( ID_M_TOOLDEBRIS, false );
	m_toolBar1->EnableTool( ID_M_TOOLDETAIL1, false );
	m_toolBar1->EnableTool( ID_M_TOOLDETAIL2, false );
	m_toolBar1->EnableTool( ID_M_TOOLDETAIL3, false );
	m_toolBar1->EnableTool( ID_M_TOOLDETAIL4, false );
	m_toolBar1->EnableTool( ID_M_TOOLDETAIL5, false );
	m_toolBar1->EnableTool( ID_M_TOOLDETAIL6, false );

	m_toolBar1->ToggleTool( ID_M_TOOLDEBRIS, false );
	m_toolBar1->ToggleTool( ID_M_TOOLDETAIL1, false );
	m_toolBar1->ToggleTool( ID_M_TOOLDETAIL2, false );
	m_toolBar1->ToggleTool( ID_M_TOOLDETAIL3, false );
	m_toolBar1->ToggleTool( ID_M_TOOLDETAIL4, false );
	m_toolBar1->ToggleTool( ID_M_TOOLDETAIL5, false );
	m_toolBar1->ToggleTool( ID_M_TOOLDETAIL6, false );

	m_toolBar1->EnableTool( ID_M_TOOLSHOWTREE, false );
	m_toolBar1->EnableTool( ID_M_TOOLSHOWDAMAGED, false );

	m_toolBar1->ToggleTool( ID_M_TOOLSHOWDAMAGED, false );

	m_toolBar1->EnableTool( ID_M_TOOLTOGGLELIGHTS, false );

	m_toolBar1->ToggleTool( ID_M_TOOLTOGGLELIGHTS, true );

	m_menuFile->Enable( wxID_CLOSE, false );

	m_menuView->Enable( ID_M_VIEW_OUTLINE, false );
	m_menuView->Enable( ID_M_VIEW_LIGHTING, false );
	m_menuView->Enable( ID_M_VIEW_PIVTOS, false );
	m_menuView->Enable( ID_M_VIEW_PATHS, false );
	m_menuView->Enable( ID_M_VIEW_RADIUS, false );
	m_menuView->Enable( ID_M_VIEW_THRUSTERS, false );

	m_menuView->Check( ID_M_VIEW_OUTLINE, false );
	m_menuView->Check( ID_M_VIEW_LIGHTING, false );
	m_menuView->Check( ID_M_VIEW_PIVTOS, false );
	m_menuView->Check( ID_M_VIEW_PATHS, false );
	m_menuView->Check( ID_M_VIEW_RADIUS, false );
	m_menuView->Check( ID_M_VIEW_THRUSTERS, false );

	m_menuView->Enable( ID_M_VIEW_TEXTURING, false );
	m_menuView->Enable( ID_M_VIEW_SMOOTHING, false );

	m_menuView->Check( ID_M_VIEW_TEXTURING, true );
	m_menuView->Check( ID_M_VIEW_SMOOTHING, true );

	m_menuView->Enable( ID_M_VIEW_SHIELDS, false );
	m_menuView->Enable( ID_M_VIEW_INVISIBLE, false );
	m_menuView->Enable( ID_M_VIEW_BAYPATHS, false );
	m_menuView->Enable( ID_M_VIEW_AUTOCENTER, false );

	m_menuView->Check( ID_M_VIEW_SHIELDS, false );
	m_menuView->Check( ID_M_VIEW_INVISIBLE, false );
	m_menuView->Check( ID_M_VIEW_BAYPATHS, false );
	m_menuView->Check( ID_M_VIEW_AUTOCENTER, false );
}

void PofViewFrame::SetTools()
{
	polymodel *pm = model_get(m_model_num);

	if (pm->num_debris_objects > 0) {
		m_toolBar1->EnableTool( ID_M_TOOLDEBRIS, true );
	}

	switch (pm->n_detail_levels) {
		case 6:
			m_toolBar1->EnableTool( ID_M_TOOLDETAIL6, true );
		case 5:
			m_toolBar1->EnableTool( ID_M_TOOLDETAIL5, true );
		case 4:
			m_toolBar1->EnableTool( ID_M_TOOLDETAIL4, true );
		case 3:
			m_toolBar1->EnableTool( ID_M_TOOLDETAIL3, true );
		case 2:
			m_toolBar1->EnableTool( ID_M_TOOLDETAIL2, true );
		case 1:
			m_toolBar1->EnableTool( ID_M_TOOLDETAIL1, true );
			break;

		default:
			break;
	}

	if (pm->n_detail_levels >= 1) {
		m_toolBar1->ToggleTool( ID_M_TOOLDETAIL1, true );
	}

	m_toolBar1->EnableTool( ID_M_TOOLSHOWTREE, true );
	m_toolBar1->EnableTool( ID_M_TOOLSHOWDAMAGED, true );

	m_toolBar1->EnableTool( ID_M_TOOLTOGGLELIGHTS, true );

	m_menuFile->Enable( wxID_CLOSE, true );

	m_menuView->Enable( ID_M_VIEW_OUTLINE, true );
	m_menuView->Enable( ID_M_VIEW_LIGHTING, true );
	m_menuView->Enable( ID_M_VIEW_PIVTOS, true );
	m_menuView->Enable( ID_M_VIEW_PATHS, true );
	m_menuView->Enable( ID_M_VIEW_RADIUS, true );
	m_menuView->Enable( ID_M_VIEW_THRUSTERS, true );

	m_menuView->Enable( ID_M_VIEW_TEXTURING, true );
	m_menuView->Enable( ID_M_VIEW_SMOOTHING, true );

	m_menuView->Enable( ID_M_VIEW_SHIELDS, true );
	m_menuView->Enable( ID_M_VIEW_INVISIBLE, true );
	m_menuView->Enable( ID_M_VIEW_BAYPATHS, true );
	m_menuView->Enable( ID_M_VIEW_AUTOCENTER, true );
}

void PofViewFrame::PofviewInit()
{
	int w, h;

	if (pofview_initted) {
		return;
	}

	outwnd_init();

	timer_init();

	cfile_init();

//	palette_load_table( "gamepalette1-01" );

	key_init();
	mouse_init();

	m_canvas->GetClientSize(&w, &h);

	gr_init();
	gr_set_viewport(w, h);
	gr_set_gamma(2.0f);

	gr_init_font( "font01.vf" );

	model_init();

	pofview_initted = true;
}

void PofViewFrame::OnFileOpen( wxCommandEvent& event )
{
	wxFileDialog openFileDialog(this, _("Open POF File"), wxEmptyString,
								wxEmptyString, _("POF Files (*.pof)|*.pof"),
								wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_OK) {
		// close existing model and reset, if anything is open already
		if (m_canvas) {
			m_canvas->Destroy();
			m_canvas = NULL;

			SetTitle("PofView");

			model_free_all();
			m_model_num = -1;

			SetToolDefaults();
		}

		// now setup for new model
		m_canvas = new PofViewCanvas(this, wxID_ANY, wxDefaultPosition, this->GetClientSize(), wxSUNKEN_BORDER);

		m_canvas->Show();

		// reuse the GL context so that we don't have to re-init the graphics
		// code between model loads
		if ( !m_glContext ) {
			m_glContext = new wxGLContext(m_canvas);
		}

		m_canvas->SetCurrent(*m_glContext);

		PofviewInit();

		m_model_num = model_load(openFileDialog.GetPath().ToAscii(), 0, NULL);
		m_current_detail_level = 1;

		if (m_model_num < 0) {
			m_canvas->Destroy();
			m_canvas = NULL;

			wxString failmsg;
			failmsg.Printf("Failed to open: '%s'",openFileDialog.GetPath());

			wxMessageDialog failDialog(this, failmsg, _("ERROR"), wxOK|wxCENTRE|wxICON_ERROR);
			failDialog.ShowModal();
		} else {
			SetDetailLevel(m_current_detail_level);
			SetTools();

			m_file_name = wxFileName(openFileDialog.GetPath()).GetName();

			SetTitle( wxString::Format("%s - PofView", m_file_name) );
		}
	}
}

void PofViewFrame::OnFileClose( wxCommandEvent& event )
{
	m_canvas->Destroy();
	m_canvas = NULL;

	SetTitle("PofView");

	model_free_all();
	m_model_num = -1;

	SetToolDefaults();
}

void PofViewFrame::OnExit(wxCommandEvent& WXUNUSED(event) )
{
	model_free_all();
	m_model_num = -1;

	Close(true);
}

void PofViewFrame::OnViewToolbar( wxCommandEvent& event )
{
	bool show_it = event.IsChecked();

	m_toolBar1->Show(show_it);
}

void PofViewFrame::OnViewStatusBar( wxCommandEvent& event )
{
	bool show_it = event.IsChecked();

	m_statusBar1->Show(show_it);
}

void PofViewFrame::OnHelpAbout( wxCommandEvent& WXUNUSED(event) )
{
	AboutBox about(this);

	in_dialog = true;

	about.ShowModal();

	in_dialog = false;
}

void PofViewFrame::OnSetDetail( wxCommandEvent& event )
{
	int detail_lvl = event.GetId() - ID_M_TOOLDEBRIS;

	polymodel *pm = model_get(m_model_num);

	if (pm == NULL) {
		return;
	}

	if (detail_lvl > pm->n_detail_levels) {
		Int3();
		return;
	}

	m_current_detail_level = detail_lvl;

	for (int i = ID_M_TOOLDEBRIS; i <= ID_M_TOOLDETAIL6; i++) {
		bool bval = false;

		if ( i == event.GetId() ) {
			bval = true;
		}

		m_toolBar1->ToggleTool(i, bval);
	}

	// update model
//	m_canvas->Render();
}

void PofViewFrame::OnShowTree( wxCommandEvent& WXUNUSED(event) )
{
	SubobjectsDialog tree(this);

	tree.ParseModel();

	in_dialog = true;

	tree.ShowModal();

	in_dialog = false;
}

void PofViewFrame::SetDetailLevel(int detail_lvl)
{
	if (m_model_num < 0) {
		return;
	}

	if (detail_lvl < 0) {
		return;
	}

	if (detail_lvl == m_current_detail_level) {
		return;
	}

	polymodel *pm = model_get(m_model_num);

	if (detail_lvl > pm->n_detail_levels) {
		return;
	}

	m_current_detail_level = detail_lvl;

	for (int i = ID_M_TOOLDEBRIS; i <= ID_M_TOOLDETAIL6; i++) {
		bool bval = false;

		if ( i == (ID_M_TOOLDEBRIS + m_current_detail_level) ) {
			bval = true;
		}

		m_toolBar1->ToggleTool(i, bval);
	}
}
