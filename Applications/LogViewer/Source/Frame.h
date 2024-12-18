#pragma once

#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/cmdline.h>

class LogViewerFrame : public wxFrame
{
public:
	LogViewerFrame(const wxPoint& pos, const wxSize& size);
	virtual ~LogViewerFrame();

	enum
	{
		ID_Exit = wxID_HIGHEST,
		ID_Clear,
		ID_Timer,
		ID_About
	};

private:
	void OnClear(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnUpdateUI(wxUpdateUIEvent& event);

	wxTextCtrl* textCtrl;
	wxTimer timer;
};