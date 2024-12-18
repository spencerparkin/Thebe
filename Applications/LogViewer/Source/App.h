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

	Thebe::NetLogCollector* GetLogCollector();

private:
	LogViewerFrame* frame;
	Thebe::NetLogCollector* logCollector;
};

wxDECLARE_APP(LogViewerApp);