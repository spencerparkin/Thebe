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

Thebe::NetLogCollector* LogViewerApp::GetLogCollector()
{
	return &this->logCollector;
}

/*virtual*/ void LogViewerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	static const wxCmdLineEntryDesc cmdLineDesc[] =
	{
		 { wxCMD_LINE_OPTION, "p", "port", "Specify which port to use for socket communication.", wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
		 { wxCMD_LINE_OPTION, "a", "addr", "Specify which IP address to use for socket communication.", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
		 { wxCMD_LINE_NONE }
	};

	parser.SetDesc(cmdLineDesc);
}

/*virtual*/ bool LogViewerApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	Thebe::NetworkAddress address;

	long port = 0;
	if (parser.Found("port", &port))
		address.SetPort((uint32_t)port);

	wxString addr;
	if (parser.Found("addr", &addr))
		address.SetIPAddress((const char*)addr.c_str());

	this->logCollector.SetAddress(address);
	return true;
}