#pragma once

#include <wx/frame.h>
#include <wx/timer.h>

class ChineseCheckersCanvas;

class ChineseCheckersFrame : public wxFrame
{
public:
	ChineseCheckersFrame(const wxPoint& pos, const wxSize& size);
	virtual ~ChineseCheckersFrame();

	enum
	{
		ID_About = wxID_HIGHEST,
		ID_Exit,
		ID_HostGame,
		ID_JoinGame,
		ID_Timer
	};

	ChineseCheckersCanvas* GetCanvas();

private:
	void OnHostGame(wxCommandEvent& event);
	void OnJoinGame(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnUpdateUI(wxUpdateUIEvent& event);
	void OnTimer(wxTimerEvent& event);

	ChineseCheckersCanvas* canvas;
	wxTimer timer;
};