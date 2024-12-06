#include "Frame.h"
#include "Canvas.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>

GraphicsToolFrame::GraphicsToolFrame(const wxPoint& pos, const wxSize& size) : wxFrame(nullptr, wxID_ANY, "Thebe Graphics Tool", pos, size), timer(this, ID_Timer)
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_BuildScene, "Build Scene", "Build a scene for the Thebe graphics engine using a file exported from 3Ds Max, Maya or Blender."));
	fileMenu->Append(new wxMenuItem(fileMenu, ID_PreviewScene, "Preview Scene", "Load and render a Thebe graphics engine scene file."));
	fileMenu->AppendSeparator();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_Exit, "Exit", "Go ski."));

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append(new wxMenuItem(helpMenu, ID_About, "About", "Show the about-box."));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, "File");
	menuBar->Append(helpMenu, "Help");
	this->SetMenuBar(menuBar);

	this->CreateStatusBar();

	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnBuildScene, this, ID_BuildScene);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnPreviewScene, this, ID_PreviewScene);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnAbout, this, ID_About);
	this->Bind(wxEVT_CLOSE_WINDOW, &GraphicsToolFrame::OnCloseWindow, this);
	this->Bind(wxEVT_TIMER, &GraphicsToolFrame::OnTimer, this, ID_Timer);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	this->canvas = new GraphicsToolCanvas(this);
	sizer->Add(this->canvas, 1, wxGROW);
	this->SetSizer(sizer);

	this->timer.Start(0);
}

/*virtual*/ GraphicsToolFrame::~GraphicsToolFrame()
{
}

GraphicsToolCanvas* GraphicsToolFrame::GetCanvas()
{
	return this->canvas;
}

void GraphicsToolFrame::OnBuildScene(wxCommandEvent& event)
{
}

void GraphicsToolFrame::OnPreviewScene(wxCommandEvent& event)
{
}

void GraphicsToolFrame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutDialogInfo;

	aboutDialogInfo.SetName("Thebe Graphics Tool");
	aboutDialogInfo.SetDescription("This tool is used to create content for the Thebe Graphics Engine.");

	wxAboutBox(aboutDialogInfo);
}

void GraphicsToolFrame::OnExit(wxCommandEvent& event)
{
	this->Close(true);
}

void GraphicsToolFrame::OnTimer(wxTimerEvent& event)
{
	this->canvas->Refresh();
}

void GraphicsToolFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
}

void GraphicsToolFrame::OnCloseWindow(wxCloseEvent& event)
{
	wxFrame::OnCloseWindow(event);
}