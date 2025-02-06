#include "Frame.h"
#include "Canvas.h"
#include "Application.h"
#include "HostGameDialog.h"
#include "JoinGameDialog.h"
#include "GameClient.h"
#include "GameServer.h"
#include "HumanClient.h"
#include "ComputerClient.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>
#include <wx/msgdlg.h>

using namespace Thebe;

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
	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnLeaveGame, this, ID_LeaveGame);
	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &ChineseCheckersFrame::OnAbout, this, ID_About);
	this->Bind(wxEVT_TIMER, &ChineseCheckersFrame::OnTimer, this, ID_Timer);
	this->Bind(wxEVT_UPDATE_UI, &ChineseCheckersFrame::OnUpdateUI, this, ID_HostGame);
	this->Bind(wxEVT_UPDATE_UI, &ChineseCheckersFrame::OnUpdateUI, this, ID_JoinGame);
	this->Bind(wxEVT_UPDATE_UI, &ChineseCheckersFrame::OnUpdateUI, this, ID_LeaveGame);
	this->Bind(wxEVT_CLOSE_WINDOW, &ChineseCheckersFrame::OnCloseWindow, this);

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
	if (hostGameDialog.ShowModal() != wxID_OK)
		return;
	
	const HostGameDialog::Data& data = hostGameDialog.GetData();

	std::unique_ptr<ChineseCheckersGameServer> gameServer(new ChineseCheckersGameServer());
	gameServer->SetAddress(data.hostAddress);
	gameServer->SetMaxConnections(data.numComputerPlayers + data.numHumanPlayers);
	gameServer->SetNumPlayers(data.numComputerPlayers + data.numHumanPlayers);
	if (!gameServer->Setup())
	{
		gameServer->Shutdown();
		wxMessageBox("Failed to start game server", "Error!", wxICON_ERROR | wxOK, this);
		return;
	}

	wxGetApp().SetGameServer(gameServer.release());

	if (data.numHumanPlayers > 0)
	{
		std::unique_ptr<HumanClient> humanClient(new HumanClient());
		humanClient->SetAddress(data.hostAddress);
		if (!humanClient->Setup())
		{
			humanClient->Shutdown();
			wxMessageBox("Failed to connect to game server!", "Error!", wxICON_ERROR | wxOK, this);
			return;
		}

		wxGetApp().GetGameClientArray().push_back(humanClient.release());
	}

	for (int i = 0; i < data.numComputerPlayers; i++)
	{
		std::unique_ptr<ComputerClient> computerClient(new ComputerClient());
		computerClient->SetAddress(data.hostAddress);
		if (!computerClient->Setup())
		{
			computerClient->Shutdown();
			wxMessageBox(wxString::Format("Failed to connect computer client %d to game server!", i), "Error!", wxICON_ERROR | wxOK, this);
			return;
		}

		wxGetApp().GetGameClientArray().push_back(computerClient.release());
	}
}

void ChineseCheckersFrame::OnJoinGame(wxCommandEvent& event)
{
	JoinGameDialog joinGameDialog(this);
	if (joinGameDialog.ShowModal() != wxID_OK)
		return;

	const JoinGameDialog::Data& data = joinGameDialog.GetData();

	std::unique_ptr<HumanClient> humanClient(new HumanClient());
	humanClient->SetAddress(data.hostAddress);
	if (!humanClient->Setup())
	{
		humanClient->Shutdown();
		wxMessageBox("Failed to connect to game server!", "Error!", wxICON_ERROR | wxOK, this);
		return;
	}

	wxGetApp().GetGameClientArray().push_back(humanClient.release());
}

void ChineseCheckersFrame::OnLeaveGame(wxCommandEvent& event)
{
	wxGetApp().ShutdownClientsAndServer();
}

void ChineseCheckersFrame::OnCloseWindow(wxCloseEvent& event)
{
	this->timer.Stop();

	wxFrame::OnCloseWindow(event);
}

void ChineseCheckersFrame::OnExit(wxCommandEvent& event)
{
	this->timer.Stop();

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
			event.Enable(wxGetApp().GetGameServer() == nullptr && wxGetApp().GetGameClientArray().size() == 0);
			break;
		}
		case ID_LeaveGame:
		{
			event.Enable(wxGetApp().GetGameClientArray().size() > 0);
			break;
		}
	}
}

void ChineseCheckersFrame::OnTimer(wxTimerEvent& event)
{
	this->canvas->Refresh(false);

	double deltaTimeSeconds = wxGetApp().GetGraphicsEngine()->GetDeltaTime();
	wxGetApp().GetFreeCam()->Update(deltaTimeSeconds);

	XBoxController* controller = wxGetApp().GetFreeCam()->GetController();
	if (controller->WasButtonPressed(XINPUT_GAMEPAD_BACK))
		this->canvas->SetDebugDraw(!this->canvas->GetDebugDraw());

	ChineseCheckersGameServer* gameServer = wxGetApp().GetGameServer();
	if (gameServer)
		gameServer->Serve();

	for (ChineseCheckersGameClient* gameClient : wxGetApp().GetGameClientArray())
		gameClient->Update(deltaTimeSeconds);
}