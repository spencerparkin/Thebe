#pragma once

#include <wx/frame.h>
#include <wx/timer.h>

class GraphicsToolCanvas;

class GraphicsToolFrame : public wxFrame
{
public:
	GraphicsToolFrame(const wxPoint& pos, const wxSize& size);
	virtual ~GraphicsToolFrame();

	enum
	{
		ID_BuildScene = wxID_HIGHEST,
		ID_PreviewScene,
		ID_About,
		ID_Exit,
		ID_Timer
	};

	GraphicsToolCanvas* GetCanvas();

private:
	void OnBuildScene(wxCommandEvent& event);
	void OnPreviewScene(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnUpdateUI(wxUpdateUIEvent& event);
	void OnCloseWindow(wxCloseEvent& event);

	GraphicsToolCanvas* canvas;
	wxTimer timer;
};