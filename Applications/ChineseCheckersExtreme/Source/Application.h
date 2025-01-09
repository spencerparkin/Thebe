#pragma once

#include <wx/app.h>
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/FreeCam.h"
#include "Thebe/Log.h"

class ChineseCheckersFrame;

class ChineseCheckersApp : public wxApp
{
public:
	ChineseCheckersApp();
	virtual ~ChineseCheckersApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;

	Thebe::GraphicsEngine* GetGraphicsEngine();
	Thebe::FreeCam* GetFreeCam();
	Thebe::DynamicLineRenderer* GetLineRenderer();

private:
	ChineseCheckersFrame* frame;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::Log> log;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::Reference<Thebe::DynamicLineRenderer> lineRenderer;
	Thebe::FreeCam freeCam;
};

wxDECLARE_APP(ChineseCheckersApp);