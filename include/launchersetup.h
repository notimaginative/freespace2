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

	protected:
		/*
		wxNotebook* m_notebook4;
		wxPanel* m_P_Video;
		wxComboBox* m_comboBox1;
		wxCheckBox* m_checkBox2;
		wxCheckBox* m_checkBox1;
		wxPanel* m_P_Audio;
		wxComboBox* m_comboBox41;
		wxComboBox* m_comboBox51;
		wxCheckBox* m_checkBox51;
		wxCheckBox* m_checkBox6;
		wxPanel* m_P_Joystick;
		wxComboBox* m_comboBox5;
		wxCheckBox* m_checkBox4;
		wxCheckBox* m_checkBox5;
		wxPanel* m_P_Speed;
		wxComboBox* m_comboBox4;
		wxPanel* m_P_Network;
		wxRadioBox* m_radioBox3;
		wxRadioBox* m_radioBox4;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_textCtrl5;
		wxPanel* m_P_PXO;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_textCtrl3;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_textCtrl4;
		wxCheckBox* m_checkBox7;
		wxCheckBox* m_checkBox8;
		*/
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

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
		wxTextCtrl* m_Network_Port;
		unsigned short m_port_validate;

		wxTextCtrl* m_PXO_Username;
		wxTextCtrl* m_PXO_Password;
		wxCheckBox* m_PXO_SkipVerify;
		wxCheckBox* m_PXO_Banners;

	public:

		LauncherSetup( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Setup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~LauncherSetup();

};



#endif // LAUNCHERSETUP_H
