#pragma once

#include <wx/app.h>
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/FreeCam.h"
#include "Thebe/Log.h"
#include "Network/GameClient.h"
#include "Network/GameServer.h"

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

	ChineseCheckersClient* GetGameClient();
	ChineseCheckersServer* GetGameServer();

	void SetGameClient(ChineseCheckersClient* gameClient);
	void SetGameServer(ChineseCheckersServer* gameServer);

private:
	ChineseCheckersFrame* frame;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::Log> log;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::Reference<Thebe::DynamicLineRenderer> lineRenderer;
	Thebe::FreeCam freeCam;
	ChineseCheckersClient* gameClient;
	ChineseCheckersServer* gameServer;
};

wxDECLARE_APP(ChineseCheckersApp);