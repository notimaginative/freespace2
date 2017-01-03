/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pofview.h"

#include "res/pofview_ico.h"


AboutBox::AboutBox( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
	: wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_bitmap2 = new wxStaticBitmap( this, wxID_ANY, wxBITMAP_PNG_FROM_DATA(pofview_ico), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_bitmap2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 15 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	bSizer4->SetMinSize( wxSize( 200,-1 ) );

	bSizer4->Add( 0, 10, 0, wxEXPAND, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, wxT("PofView Version 1.0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer4->Add( m_staticText3, 0, wxALL, 5 );


	bSizer4->Add( 0, 5, 0, 0, 5 );

	m_staticText4 = new wxStaticText( this, wxID_ANY, wxT("Copyright © 1996"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer4->Add( m_staticText4, 0, wxALL, 5 );


	bSizer4->Add( 0, 30, 0, 0, 5 );


	bSizer3->Add( bSizer4, 1, 0, 5 );

	m_button2 = new wxButton( this, wxID_OK, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_button2, 0, wxALL, 5 );


	this->SetSizer( bSizer3 );
	this->Layout();
	bSizer3->Fit( this );

	this->Centre( wxBOTH );
}

AboutBox::~AboutBox()
{
}

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( SubobjectsDialog, wxDialog )
	EVT_TREE_SEL_CHANGED( ID_OBJ_TREE, SubobjectsDialog::OnSelChanged )
END_EVENT_TABLE()

SubobjectsDialog::SubobjectsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
	: wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_treeCtrlSubobjects = new wxTreeCtrl( this, ID_OBJ_TREE, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_HIDE_ROOT );
	m_treeCtrlSubobjects->SetMinSize( wxSize( 200,-1 ) );

	bSizer4->Add( m_treeCtrlSubobjects, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 1, 15, 0 );
	fgSizer1->SetFlexibleDirection( wxVERTICAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxVERTICAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextName = new wxStaticText( this, wxID_ANY, wxT("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	fgSizer2->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlName = new wxTextCtrl( this, wxID_ANY, wxT("name"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( m_textCtrlName, 0, wxALL|wxEXPAND, 5 );

	m_staticTextBspGenVersion = new wxStaticText( this, wxID_ANY, wxT("BspGen Version:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBspGenVersion->Wrap( -1 );
	fgSizer2->Add( m_staticTextBspGenVersion, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlBspGenVersion = new wxTextCtrl( this, wxID_ANY, wxT("1.1"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( m_textCtrlBspGenVersion, 0, wxALL|wxEXPAND, 5 );

	m_staticTextPolys = new wxStaticText( this, wxID_ANY, wxT("Polys:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPolys->Wrap( -1 );
	fgSizer2->Add( m_staticTextPolys, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlNumPolys = new wxTextCtrl( this, wxID_ANY, wxT("1000"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( m_textCtrlNumPolys, 0, wxALL|wxEXPAND, 5 );

	m_staticTextVerts = new wxStaticText( this, wxID_ANY, wxT("Verts:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVerts->Wrap( -1 );
	fgSizer2->Add( m_staticTextVerts, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlNumVerts = new wxTextCtrl( this, wxID_ANY, wxT("2000"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( m_textCtrlNumVerts, 0, wxALL|wxEXPAND, 5 );

	m_staticTextMovementType = new wxStaticText( this, wxID_ANY, wxT("Movement Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMovementType->Wrap( -1 );
	fgSizer2->Add( m_staticTextMovementType, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlMovementType = new wxTextCtrl( this, wxID_ANY, wxT("mt"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( m_textCtrlMovementType, 0, wxALL|wxEXPAND, 5 );

	m_staticTextMovementAxis = new wxStaticText( this, wxID_ANY, wxT("Movement Axis:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMovementAxis->Wrap( -1 );
	fgSizer2->Add( m_staticTextMovementAxis, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlMovementAxis = new wxTextCtrl( this, wxID_ANY, wxT("ma"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( m_textCtrlMovementAxis, 0, wxALL|wxEXPAND, 5 );


	fgSizer1->Add( fgSizer2, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Lower Detail Versions") ), wxVERTICAL );

	m_staticTextDetail1 = new wxStaticText( this, wxID_ANY, wxT("detail1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDetail1->Wrap( -1 );
	sbSizer1->Add( m_staticTextDetail1, 0, wxLEFT, 10 );

	m_staticTextDetail2 = new wxStaticText( this, wxID_ANY, wxT("detail2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDetail2->Wrap( -1 );
	sbSizer1->Add( m_staticTextDetail2, 0, wxLEFT, 10 );

	m_staticTextDetail3 = new wxStaticText( this, wxID_ANY, wxT("detail3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDetail3->Wrap( -1 );
	sbSizer1->Add( m_staticTextDetail3, 0, wxLEFT, 10 );

	m_staticTextDetail4 = new wxStaticText( this, wxID_ANY, wxT("detail4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDetail4->Wrap( -1 );
	sbSizer1->Add( m_staticTextDetail4, 0, wxLEFT, 10 );

	m_staticTextDetail5 = new wxStaticText( this, wxID_ANY, wxT("detail5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDetail5->Wrap( -1 );
	sbSizer1->Add( m_staticTextDetail5, 0, wxLEFT, 10 );

	m_staticTextDetail6 = new wxStaticText( this, wxID_ANY, wxT("detail6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDetail6->Wrap( -1 );
	sbSizer1->Add( m_staticTextDetail6, 0, wxLEFT, 10 );


	fgSizer1->Add( sbSizer1, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("POF Info") ), wxVERTICAL );

	m_staticTextPOFInfo = new wxStaticText( this, wxID_ANY, wxT("pofinfo"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPOFInfo->Wrap( -1 );
	sbSizer2->Add( m_staticTextPOFInfo, 0, wxALL, 5 );


	fgSizer1->Add( sbSizer2, 1, wxEXPAND, 5 );


	bSizer4->Add( fgSizer1, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer4 );
	this->Layout();
	bSizer4->Fit( this );

	this->Centre( wxBOTH );

	m_pm = NULL;
}

SubobjectsDialog::~SubobjectsDialog()
{
}

class subobjectItemData : public wxTreeItemData
{
	private:
		int sm_id;

	public:
		subobjectItemData(int sm)
			: wxTreeItemData()
		{
			sm_id = sm;
		}

		int GetSubmodelId() { return sm_id; }
};

void SubobjectsDialog::OnSelChanged(wxTreeEvent &event)
{
	wxTreeItemId item = event.GetItem();

	if ( !item.IsOk() ) {
		return;
	}

	int model_num = ((PofViewFrame*)GetParent())->GetModelnum();

	int sm_id = ((subobjectItemData*)(m_treeCtrlSubobjects->GetItemData(item)))->GetSubmodelId();

	bsp_info *sm = &m_pm->submodel[sm_id];

	m_staticTextDetail1->SetLabelText("");
	m_staticTextDetail2->SetLabelText("");
	m_staticTextDetail3->SetLabelText("");
	m_staticTextDetail4->SetLabelText("");
	m_staticTextDetail5->SetLabelText("");
	m_staticTextDetail6->SetLabelText("");

	m_textCtrlName->SetValue(sm->name);
	m_textCtrlBspGenVersion->SetValue( wxString::Format("%d.%02d", m_pm->version / 100, m_pm->version % 100));
	m_textCtrlNumPolys->SetValue( wxString::Format("%d", submodel_get_num_polys(model_num, sm_id)) );
	m_textCtrlNumVerts->SetValue( wxString::Format("%d", submodel_get_num_verts(model_num, sm_id)) );

	switch (sm->movement_type) {
		case -1:
			m_textCtrlMovementType->SetValue("None");
			break;

		case 0:
			m_textCtrlMovementType->SetValue("Positional");
			break;

		case 1:
			m_textCtrlMovementType->SetValue("Rotational");
			break;

		default:
			m_textCtrlMovementType->SetValue("?Unknown?");
			break;
	}

	if (sm->movement_type == 1) {
		switch (sm->movement_axis) {
			case 0:
				m_textCtrlMovementAxis->SetValue("X (Pitch)");
				break;

			case 1:
				m_textCtrlMovementAxis->SetValue("Y (Bank)");
				break;

			case 2:
				m_textCtrlMovementAxis->SetValue("Z (Heading)");
				break;

			default:
				m_textCtrlMovementAxis->SetValue("?Unknown?");
				break;
		}
	} else {
		m_textCtrlMovementAxis->SetValue("");
	}

	switch (sm->num_details) {
		case 6:
			m_staticTextDetail6->SetLabelText(m_pm->submodel[sm->details[5]].name);
		case 5:
			m_staticTextDetail5->SetLabelText(m_pm->submodel[sm->details[4]].name);
		case 4:
			m_staticTextDetail4->SetLabelText(m_pm->submodel[sm->details[3]].name);
		case 3:
			m_staticTextDetail3->SetLabelText(m_pm->submodel[sm->details[2]].name);
		case 2:
			m_staticTextDetail2->SetLabelText(m_pm->submodel[sm->details[1]].name);
		case 1:
			m_staticTextDetail1->SetLabelText(m_pm->submodel[sm->details[0]].name);
	}

#ifndef NDEBUG
	if (sm->i_replace > -1) {
		m_staticTextPOFInfo->SetLabelText( wxString::Format("%s\n[I replace %s]",
															m_pm->debug_info,
															m_pm->submodel[sm->i_replace].name) );
	} else if (sm->my_replacement > -1) {
		m_staticTextPOFInfo->SetLabelText(wxString::Format("%s\n[My replacement %s]",
														   m_pm->debug_info,
														   m_pm->submodel[sm->my_replacement].name) );
	} else {
		m_staticTextPOFInfo->SetLabelText(m_pm->debug_info);
	}
#else
	m_staticTextPOFInfo->SetLabelText("");
#endif
}

void SubobjectsDialog::AddModel(int sm, wxTreeItemId parent)
{
	wxTreeItemId item;
	int i;

	subobjectItemData *data = new subobjectItemData(sm);

	// check for live debris
	if ( !SDL_strncasecmp("debris-", m_pm->submodel[sm].name, strlen("debris-")) ) {
		wxString debris_name;

		// traverse the tree and put live debris with correct submodel
		for (item = m_treeCtrlSubobjects->GetFirstVisibleItem(); item.IsOk(); item = m_treeCtrlSubobjects->GetNextVisible(item)) {
			debris_name = wxString::Format("debris-%s", m_treeCtrlSubobjects->GetItemText(item));

			int res = wxString(m_pm->submodel[sm].name).Find(debris_name);

			if (res != wxNOT_FOUND) {
				item = m_treeCtrlSubobjects->AppendItem(item, m_pm->submodel[sm].name);
				m_treeCtrlSubobjects->SetItemData(item, data);

				return;
			}
		}
	}

	item = m_treeCtrlSubobjects->AppendItem(parent, m_pm->submodel[sm].name);

	m_treeCtrlSubobjects->SetItemData(item, data);

	for (i = m_pm->submodel[sm].first_child; i > -1; i = m_pm->submodel[i].next_sibling) {
		AddModel(i, item);
	}
}

void SubobjectsDialog::ParseModel()
{
	int model_num = ((PofViewFrame*)GetParent())->GetModelnum();

	if (model_num < 0) {
		return;
	}

	m_pm = model_get(model_num);
	wxASSERT( m_pm );

	SetTitle( wxString::Format("%s's subobjects", ((PofViewFrame*)GetParent())->GetFileName()) );

	int i;

	wxTreeItemId root = m_treeCtrlSubobjects->AddRoot( wxT("root") );

	// add all base submodels (ie, without parents) except live debris
	for (i = 0; i < m_pm->n_models; i++) {
		if (m_pm->submodel[i].parent < 0) {
			// add if *not* live debris
			if ( SDL_strncasecmp("debris-", m_pm->submodel[i].name, strlen("debris-")) ) {
				AddModel(i, root);
			}
		}
	}

	// Expand to make all tree visible for searching when adding live debris
	m_treeCtrlSubobjects->ExpandAll();

	// Now add any live debris
	for (i = 0; i < m_pm->n_models; i++) {
		if (m_pm->submodel[i].parent < 0) {
			// add if live debris
			if ( !SDL_strncasecmp("debris-", m_pm->submodel[i].name, strlen("debris-")) ) {
				AddModel(i, root);
			}
		}
	}

	m_treeCtrlSubobjects->CollapseAll();

	wxTreeItemId item = m_treeCtrlSubobjects->GetFirstVisibleItem();

	// deal with wxWidgets bug that shows hidden root as first visible
	if ( item == m_treeCtrlSubobjects->GetRootItem() ) {
		wxTreeItemIdValue cookie;

		item = m_treeCtrlSubobjects->GetFirstChild(item, cookie);
	}

	// force select first item in tree and resize dialog to fit
	m_treeCtrlSubobjects->SelectItem(item);

	Fit();
}
