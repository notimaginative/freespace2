/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

// PofView.h : main header file for the POFVIEW application
//

#ifndef _POFVIEW_H
#define _POFVIEW_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include "wx/artprov.h"
#include "wx/treectrl.h"
#include "wx/glcanvas.h"
#include "wx/aboutdlg.h"

#include "vecmat.h"
#include "physics.h"
#include "model.h"

///////////////////////////////////////////////////////////////////////////////
/// Class AboutBox
///////////////////////////////////////////////////////////////////////////////
class AboutBox : public wxDialog
{
	private:

	protected:
		wxStaticBitmap* m_bitmap2;
		wxStaticText* m_staticText3;
		wxStaticText* m_staticText4;
		wxButton* m_button2;

	public:

		AboutBox( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About PofView"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~AboutBox();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SubobjectsDialog
///////////////////////////////////////////////////////////////////////////////
class SubobjectsDialog : public wxDialog
{
	private:
		polymodel *m_pm;
		void AddModel(int sm, wxTreeItemId parent);

		wxDECLARE_EVENT_TABLE();

	protected:

		enum {
			ID_OBJ_TREE = 1000
		};

		wxTreeCtrl* m_treeCtrlSubobjects;
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCtrlName;
		wxStaticText* m_staticTextBspGenVersion;
		wxTextCtrl* m_textCtrlBspGenVersion;
		wxStaticText* m_staticTextPolys;
		wxTextCtrl* m_textCtrlNumPolys;
		wxStaticText* m_staticTextVerts;
		wxTextCtrl* m_textCtrlNumVerts;
		wxStaticText* m_staticTextMovementType;
		wxTextCtrl* m_textCtrlMovementType;
		wxStaticText* m_staticTextMovementAxis;
		wxTextCtrl* m_textCtrlMovementAxis;
		wxStaticText* m_staticTextDetail1;
		wxStaticText* m_staticTextDetail2;
		wxStaticText* m_staticTextDetail3;
		wxStaticText* m_staticTextDetail4;
		wxStaticText* m_staticTextDetail5;
		wxStaticText* m_staticTextDetail6;
		wxStaticText* m_staticTextPOFInfo;

		void OnSelChanged(wxTreeEvent& event);

	public:

		SubobjectsDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Subobjects"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~SubobjectsDialog();

		void ParseModel();
};

///////////////////////////////////////////////////////////////////////////////
/// Class PofViewTimer
///////////////////////////////////////////////////////////////////////////////

class PofViewCanvas;

class PofViewTimer : public wxTimer
{
	private:
		PofViewCanvas *m_canvas;
		int m_thrust_timer;

	public:
		PofViewTimer(PofViewCanvas *canvas);

		~PofViewTimer();

		void Notify();
};


///////////////////////////////////////////////////////////////////////////////
/// Class PofViewCanvas
///////////////////////////////////////////////////////////////////////////////
class PofViewCanvas : public wxGLCanvas
{
	private:
		bool first_frame;
		float m_ViewerZoom;
		vector m_ViewerPos;
		matrix m_ViewerOrient;
		matrix m_ObjectOrient;

		physics_info m_ViewerPhysics;
		control_info m_Viewer_ci;

		int m_mouse_inited;
		int m_mouse_x;
		int m_mouse_y;
		int m_mouse_dx;
		int m_mouse_dy;

		PofViewTimer *m_timer;

		bool thrust_anim_inited;

		float model_thrust;
		int model_afterburner;

		int shipp_thruster_bitmap;
		float shipp_thruster_frame;

		int shipp_thruster_glow_bitmap;
		float shipp_thruster_glow_frame;
		float shipp_thruster_glow_noise;

		void InitThrusters();

		wxDECLARE_EVENT_TABLE();

	protected:
		void OnPaint(wxPaintEvent& event);
		void OnSize(wxSizeEvent& event);
		void OnEraseBackground(wxEraseEvent& event);
		void OnMouse(wxMouseEvent& event);

	public:

		PofViewCanvas(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxT("TestGLCanvas"));

		virtual ~PofViewCanvas();

		void Render();
		void DoThrusterFrame(float frame_time);
		void MoveViewer(float frame_time);
};

///////////////////////////////////////////////////////////////////////////////
/// Class PofViewFrame
///////////////////////////////////////////////////////////////////////////////
class PofViewFrame : public wxFrame
{
	private:
		wxString m_file_name;
		int m_model_num;
		int m_current_detail_level;

		bool pofview_initted;

		wxGLContext* m_glContext;

		void MakeMenuBar();
		void MakeToolBar();

		void SetToolDefaults();
		void SetTools();

		void PofviewInit();

		wxDECLARE_EVENT_TABLE();

	protected:

		enum
		{
			ID_M_VIEW_TOOLBAR = 1000,
			ID_M_VIEW_STATUSBAR,
			ID_M_VIEW_OUTLINE,
			ID_M_VIEW_LIGHTING,
			ID_M_VIEW_PIVTOS,
			ID_M_VIEW_PATHS,
			ID_M_VIEW_RADIUS,
			ID_M_VIEW_THRUSTERS,
			ID_M_VIEW_TEXTURING,
			ID_M_VIEW_SMOOTHING,
			ID_M_VIEW_SHIELDS,
			ID_M_VIEW_INVISIBLE,
			ID_M_VIEW_BAYPATHS,
			ID_M_VIEW_AUTOCENTER,
			ID_M_WINDOW_NEW,
			ID_M_WINDOW_CASCADE,
			ID_M_WINDOW_TILE,
			ID_M_WINDOW_ARRANGE,
			ID_M_TOOLPRINT,
			ID_M_TOOLABOUT,
			// **************
			// do not alter order!!!
			ID_M_TOOLDEBRIS,
			ID_M_TOOLDETAIL1,
			ID_M_TOOLDETAIL2,
			ID_M_TOOLDETAIL3,
			ID_M_TOOLDETAIL4,
			ID_M_TOOLDETAIL5,
			ID_M_TOOLDETAIL6,
			// **************
			ID_M_TOOLSHOWTREE,
			ID_M_TOOLSHOWDAMAGED,
			ID_M_TOOLTOGGLELIGHTS,
			ID_M_TOOLTOGGLETURRETS
		};

		wxMenuBar* m_menubar1;
		wxMenu* m_menuFile;
		wxMenu* m_menuEdit;
		wxMenu* m_menuView;
		wxMenu* m_menuHelp;
		wxToolBar* m_toolBar1;
		wxPanel* m_panel1;
		wxStatusBar* m_statusBar1;
		PofViewCanvas* m_canvas;

		void OnFileOpen( wxCommandEvent& event );
		void OnFileClose( wxCommandEvent& event );
		void OnFilePrintSetup( wxCommandEvent& event );
		void OnExit( wxCommandEvent& event );
		void OnViewToolbar( wxCommandEvent& event );
		void OnViewStatusBar( wxCommandEvent& event );
		void OnHelpAbout( wxCommandEvent& event );
		void OnSetDetail( wxCommandEvent& event );
		void OnShowTree( wxCommandEvent& event );

	public:

		PofViewFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("PofView"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~PofViewFrame();

		void SetDetailLevel(int detail_lvl);

		const wxString GetFileName() { return m_file_name; }
		int GetModelnum() { return m_model_num; }
		int GetDetailLevel() { return m_current_detail_level; }

		bool ShowOutline() { return m_menuView->IsChecked(ID_M_VIEW_OUTLINE); }
		bool ShowShields() { return m_menuView->IsChecked(ID_M_VIEW_SHIELDS); }
		bool ShowInvisible() { return m_menuView->IsChecked(ID_M_VIEW_INVISIBLE); }
		bool ShowThrusters() { return m_menuView->IsChecked(ID_M_VIEW_THRUSTERS); }
		bool ShowPivots() { return m_menuView->IsChecked(ID_M_VIEW_PIVTOS); }
		bool ShowPaths() { return m_menuView->IsChecked(ID_M_VIEW_PATHS); }
		bool ShowRadius() { return m_menuView->IsChecked(ID_M_VIEW_RADIUS); }
		bool ShowBayPaths() { return m_menuView->IsChecked(ID_M_VIEW_BAYPATHS); }

		bool ShowDamaged() { return m_toolBar1->GetToolState(ID_M_TOOLSHOWDAMAGED); }

		bool UseSmoothing() { return m_menuView->IsChecked(ID_M_VIEW_SMOOTHING); }
		bool UseTexturing() { return m_menuView->IsChecked(ID_M_VIEW_TEXTURING); }
		bool UseAutocenter() { return m_menuView->IsChecked(ID_M_VIEW_AUTOCENTER); }
		bool UseLighting() { return m_menuView->IsChecked(ID_M_VIEW_LIGHTING); }

		bool LightingOn() { return m_toolBar1->GetToolState(ID_M_TOOLTOGGLELIGHTS); }
};

#endif
