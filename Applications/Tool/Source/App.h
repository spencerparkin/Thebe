#pragma once

#include <wx/app.h>
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include "Thebe/FreeCam.h"

class GraphicsToolFrame;

class GraphicsToolApp : public wxApp
{
public:
	GraphicsToolApp();
	virtual ~GraphicsToolApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;

	Thebe::GraphicsEngine* GetGraphicsEngine();
	Thebe::FreeCam* GetFreeCam();

private:
	GraphicsToolFrame* frame;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::Log> log;
	Thebe::FreeCam freeCam;
};

wxDECLARE_APP(GraphicsToolApp);