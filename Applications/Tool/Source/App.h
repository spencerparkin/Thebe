#pragma once

#include <wx/app.h>
#include "Thebe/GraphicsEngine.h"

class GraphicsToolFrame;

class GraphicsToolApp : public wxApp
{
public:
	GraphicsToolApp();
	virtual ~GraphicsToolApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;

	Thebe::GraphicsEngine* GetGraphicsEngine();

private:
	GraphicsToolFrame* frame;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
};

wxDECLARE_APP(GraphicsToolApp);