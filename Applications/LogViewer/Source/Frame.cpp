#include "Frame.h"
#include "App.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>

LogViewerFrame::LogViewerFrame(const wxPoint& pos, const wxSize& size) : wxFrame(nullptr, wxID_ANY, "Thebe Log Viewer", pos, size), timer(this)
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_Clear, "Clear", "Clear all log messages."));
	fileMenu->AppendSeparator();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_Exit, "Exit", "Go ski."));

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append(new wxMenuItem(helpMenu, ID_About, "About", "Show the about-box."));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, "File");
	menuBar->Append(helpMenu, "Help");
	this->SetMenuBar(menuBar);

	this->Bind(wxEVT_MENU, &LogViewerFrame::OnClear, this, ID_Clear);
	this->Bind(wxEVT_MENU, &LogViewerFrame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &LogViewerFrame::OnAbout, this, ID_About);
	this->Bind(wxEVT_UPDATE_UI, &LogViewerFrame::OnUpdateUI, this, ID_Clear);

	this->CreateStatusBar();

	this->textCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(textCtrl, 1, wxGROW | wxALL, 2);
	this->SetSizer(sizer);

	Thebe::NetworkAddress address = wxGetApp().GetLogCollector()->GetReceptionAddress();
	this->SetTitle(wxString::Format("Thebe Log Viewer -- Listening @ %s", address.GetAddress().c_str()));

	this->timer.Start(60);
}

/*virtual*/ LogViewerFrame::~LogViewerFrame()
{
}

void LogViewerFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
	switch (event.GetId())
	{
		case ID_Clear:
		{
			event.Enable(this->textCtrl->GetValue().Length() > 0);
			break;
		}
	}
}

void LogViewerFrame::OnClear(wxCommandEvent& event)
{
	this->textCtrl->Clear();
}

void LogViewerFrame::OnExit(wxCommandEvent& event)
{
	this->Close(true);
}

void LogViewerFrame::OnTimer(wxTimerEvent& event)
{
	std::string logMessage;
	if (wxGetApp().GetLogCollector()->GrabLogMessage(logMessage))
	{
		wxString logMessageStr(logMessage.c_str());
		this->textCtrl->AppendText(logMessageStr);
	}
}

void LogViewerFrame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutDialogInfo;

	aboutDialogInfo.SetName("Thebe Log Viewer");
	aboutDialogInfo.SetDescription("This tool can be used to see logs from the Thebe graphics engine in real time.");

	wxAboutBox(aboutDialogInfo);
}