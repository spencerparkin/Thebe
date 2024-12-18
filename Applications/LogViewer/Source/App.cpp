#include "App.h"
#include "Frame.h"
#include <wx/msgdlg.h>

wxIMPLEMENT_APP(LogViewerApp);

LogViewerApp::LogViewerApp()
{
	this->frame = nullptr;
	this->logCollector = nullptr;
}

/*virtual*/ LogViewerApp::~LogViewerApp()
{
}

/*virtual*/ bool LogViewerApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	auto server = new Thebe::NetServerLogCollector();
	Thebe::NetworkAddress address;
	address.SetIPAddress("127.0.0.1");
	address.SetPort(12345);
	server->SetListeningAddress(address);
	this->logCollector = server;
	if (!this->logCollector->Setup())
	{
		wxMessageBox("Failed to setup log collector!", "Error!", wxICON_ERROR | wxOK, nullptr);
		return false;
	}

	this->frame = new LogViewerFrame(wxPoint(100, 100), wxSize(800, 500));
	this->frame->Show();

	return true;
}

/*virtual*/ int LogViewerApp::OnExit(void)
{
	if (this->logCollector)
	{
		this->logCollector->Shutdown();
		delete this->logCollector;
		this->logCollector = nullptr;
	}

	return 0;
}

Thebe::NetLogCollector* LogViewerApp::GetLogCollector()
{
	return this->logCollector;
}