#pragma once

#include <wx/app.h>
#include "Thebe/NetLog.h"

class LogViewerFrame;

class LogViewerApp : public wxApp
{
public:
	LogViewerApp();
	virtual ~LogViewerApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;
	virtual void OnInitCmdLine(wxCmdLineParser& parser) override;
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

	Thebe::NetLogCollector* GetLogCollector();

private:
	LogViewerFrame* frame;
	Thebe::NetLogCollector* logCollector;
	Thebe::NetworkAddress address;
};

wxDECLARE_APP(LogViewerApp);