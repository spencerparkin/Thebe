#pragma once

#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/stattext.h>
#include <wx/tglbtn.h>

class ChineseCheckersCanvas;

class ChineseCheckersFrame : public wxFrame
{
public:
	ChineseCheckersFrame(const wxPoint& pos, const wxSize& size);
	virtual ~ChineseCheckersFrame();

	enum
	{
		ID_About = wxID_HIGHEST,
		ID_Help,
		ID_Exit,
		ID_HostGame,
		ID_JoinGame,
		ID_LeaveGame,
#if defined THEBE_USE_IMGUI
		ID_ToggleProfilerWindow,
#endif //THEBE_USE_IMGUI
		ID_Timer
	};

	ChineseCheckersCanvas* GetCanvas();

	void SetInfoText(const wxString& infoText);
	void SetStatusText(const wxString& statusText);

private:
	void OnHostGame(wxCommandEvent& event);
	void OnJoinGame(wxCommandEvent& event);
	void OnLeaveGame(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnHelp(wxCommandEvent& event);
#if defined THEBE_USE_IMGUI
	void OnToggleProfilerWindow(wxCommandEvent& event);
#endif //THEBE_USE_IMGUI
	void OnUpdateUI(wxUpdateUIEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnCloseWindow(wxCloseEvent& event);
	void OnToggleLifeCountsButtonPressed(wxCommandEvent& event);
	void OnToggleProfileStatsButtonPressed(wxCommandEvent& event);

	wxStaticText* infoText;
	wxToggleButton* lifeToggleButton;
	wxToggleButton* profileStatsToggleButton;
	ChineseCheckersCanvas* canvas;
	wxTimer timer;
};