#include "Application.h"
#include "Frame.h"
#include "Canvas.h"
#include "Thebe/NetLog.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>

wxIMPLEMENT_APP(ChineseCheckersApp);

using namespace Thebe;

ChineseCheckersApp::ChineseCheckersApp()
{
	this->frame = nullptr;
	this->graphicsEngine.Set(new GraphicsEngine());
}

/*virtual*/ ChineseCheckersApp::~ChineseCheckersApp()
{
}

/*virtual*/ bool ChineseCheckersApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	this->frame = new ChineseCheckersFrame(wxPoint(10, 10), wxSize(1200, 800));

	wxFileName loggerPath(wxStandardPaths::Get().GetExecutablePath());
	loggerPath.SetName("ThebeLogViewer");
	loggerPath.SetExt("exe");
	wxString fullPath = loggerPath.GetFullPath();
	if (loggerPath.FileExists())
	{
		wxString command = loggerPath.GetFullPath() + " --port=12345 --addr=127.0.0.1";
		long loggerPID = wxExecute(command, wxEXEC_ASYNC);
		if (loggerPID != 0)
		{
			this->log.Set(new Thebe::Log());
			Thebe::Reference<Thebe::NetClientLogSink> logSink(new Thebe::NetClientLogSink());
			Thebe::NetworkAddress address;
			address.SetIPAddress("127.0.0.1");
			address.SetPort(12345);
			logSink->SetConnectAddress(address);
			this->log->AddSink(logSink);
			Thebe::Log::Set(this->log);
		}
	}

	HWND windowHandle = this->frame->GetCanvas()->GetHWND();
	if (!this->graphicsEngine->Setup(windowHandle))
		return false;

	this->frame->Show();

	return true;
}

/*virtual*/ int ChineseCheckersApp::OnExit(void)
{
	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;

	THEBE_LOG("CloseLogViewer");

	return 0;
}

GraphicsEngine* ChineseCheckersApp::GetGraphicsEngine()
{
	return this->graphicsEngine.Get();
}