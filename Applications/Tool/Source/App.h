#pragma once

#include <wx/app.h>
#include <wx/config.h>
#include "Thebe/GraphicsEngine.h"
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
	wxConfig* GetConfig();

private:
	GraphicsToolFrame* frame;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::FreeCam freeCam;
	wxConfig* config;
};

wxDECLARE_APP(GraphicsToolApp);