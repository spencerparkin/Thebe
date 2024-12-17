#pragma once

#include <wx/app.h>
#include "Thebe/DatagramLog.h"

class LogViewerFrame;

class LogViewerApp : public wxApp
{
public:
	LogViewerApp();
	virtual ~LogViewerApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;

	Thebe::DatagramLogCollector* GetLogCollector();

private:
	LogViewerFrame* frame;
	Thebe::DatagramLogCollector logCollector;
};

wxDECLARE_APP(LogViewerApp);