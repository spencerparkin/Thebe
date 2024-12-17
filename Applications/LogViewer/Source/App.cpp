#include "App.h"
#include "Frame.h"
#include <wx/msgdlg.h>

wxIMPLEMENT_APP(LogViewerApp);

LogViewerApp::LogViewerApp()
{
	this->frame = nullptr;
}

/*virtual*/ LogViewerApp::~LogViewerApp()
{
}

/*virtual*/ bool LogViewerApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	Thebe::NetworkAddress address;
	address.SetIPAddress("192.168.0.5");
	address.SetPort(8908);		// TODO: Maybe get this from command-line?
	this->logCollector.SetReceptionAddress(address);
	if (!this->logCollector.Setup())
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
	this->logCollector.Shutdown();

	return 0;
}

Thebe::DatagramLogCollector* LogViewerApp::GetLogCollector()
{
	return &this->logCollector;
}