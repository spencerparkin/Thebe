#include "Frame.h"
#include "Canvas.h"
#include "Application.h"
#include "HostGameDialog.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>

ChineseCheckersFrame::ChineseCheckersFrame(const wxPoint& pos, const wxSize& size) : wxFrame(nullptr, wxID_ANY, "Chinese Checkers Extreme", pos, size), timer(this, ID_Timer)
{
	wxMenu* gameMenu = new wxMenu();
	gameMenu->Append(new wxMenuItem(gameMenu, ID_HostGame, "Host Game", "Start a new game that one and others on the network can join."));
	gameMenu->Append(new wxMenuItem(gameMenu, ID_JoinGame, "Join Game", "Join a game that someone is hosting on the network."));
	gameMenu->AppendSeparator();
	gameMenu->Append(new wxMenuItem(gameMenu, ID_LeaveGame, "Leave Game", "Leave the currently joined game.  If you're the host, everyone has to leave."));
	gameMenu->AppendSeparator();
	gameMenu->Append(new wxMenuItem(gameMenu, ID_Exit, "Exit", "Go ski."));

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append(new wxMenuItem(helpMenu, ID_About, "About", "Show the about-box."));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(gameMenu, "Game");
	menuBar->Append(helpMenu, "Help");
	this->SetMenuBar(menuBar);

	this->CreateStatusBar();

	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnHostGame, this, ID_HostGame);
	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnJoinGame, this, ID_JoinGame);
	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnAbout, this, ID_About);
	this->Bind(wxEVT_TIMER, &ChineseCheckersFrame::OnTimer, this, ID_Timer);
	this->Bind(wxEVT_UPDATE_UI, &ChineseCheckersFrame::OnUpdateUI, this, ID_HostGame);
	this->Bind(wxEVT_UPDATE_UI, &ChineseCheckersFrame::OnUpdateUI, this, ID_JoinGame);
	this->Bind(wxEVT_UPDATE_UI, &ChineseCheckersFrame::OnUpdateUI, this, ID_LeaveGame);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	this->canvas = new ChineseCheckersCanvas(this);
	sizer->Add(this->canvas, 1, wxGROW);
	this->SetSizer(sizer);

	this->timer.Start(0);
}

/*virtual*/ ChineseCheckersFrame::~ChineseCheckersFrame()
{
}

ChineseCheckersCanvas* ChineseCheckersFrame::GetCanvas()
{
	return this->canvas;
}

void ChineseCheckersFrame::OnHostGame(wxCommandEvent& event)
{
	HostGameDialog hostGameDialog(this);
	if (hostGameDialog.ShowModal() == wxID_OK)
	{
		const HostGameDialog::Data& data = hostGameDialog.GetData();
	}
}

void ChineseCheckersFrame::OnJoinGame(wxCommandEvent& event)
{
}

void ChineseCheckersFrame::OnLeaveGame(wxCommandEvent& event)
{

}

void ChineseCheckersFrame::OnExit(wxCommandEvent& event)
{
	this->Close(true);
}

void ChineseCheckersFrame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutDialogInfo;

	aboutDialogInfo.SetName("Chinese Checkers Extreme");
	aboutDialogInfo.SetDescription("This is a variation of the traditional game of Chinese Checkers.");
	
	wxArrayString developerArray;
	developerArray.Add("Spencer T. Parkin (spencer.parkin@proton.me)");
	aboutDialogInfo.SetDevelopers(developerArray);

	aboutDialogInfo.SetCopyright("Copyright (c) 2025, All Rights Reserved");

	aboutDialogInfo.SetVersion("0.1");

	wxAboutBox(aboutDialogInfo);
}

void ChineseCheckersFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
	switch (event.GetId())
	{
		case ID_HostGame:
		case ID_JoinGame:
		{
			event.Enable(wxGetApp().GetGameClient() == nullptr && wxGetApp().GetGameServer() == nullptr);
			break;
		}
		case ID_LeaveGame:
		{
			event.Enable(wxGetApp().GetGameClient() != nullptr || wxGetApp().GetGameServer() != nullptr);
			break;
		}
	}
}

void ChineseCheckersFrame::OnTimer(wxTimerEvent& event)
{
	this->canvas->Refresh(false);
	double deltaTimeSeconds = wxGetApp().GetGraphicsEngine()->GetDeltaTime();
	wxGetApp().GetFreeCam()->Update(deltaTimeSeconds);
}