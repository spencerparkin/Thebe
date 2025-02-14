#pragma once

#include <wx/app.h>
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

class GraphicsToolFrame;

class GraphicsToolApp : public wxApp
{
public:
	GraphicsToolApp();
	virtual ~GraphicsToolApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;

	Thebe::GraphicsEngine* GetGraphicsEngine();
	Thebe::XBoxController* GetController();

private:
	GraphicsToolFrame* frame;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::Log> log;
	Thebe::Reference<Thebe::XBoxController> controller;
};

wxDECLARE_APP(GraphicsToolApp);