#pragma once

#include <wx/app.h>
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Log.h"

class ChineseCheckersFrame;
class ChineseCheckersGameServer;
class ChineseCheckersGameClient;
class ComputerClient;
class HumanClient;

class ChineseCheckersApp : public wxApp
{
public:
	ChineseCheckersApp();
	virtual ~ChineseCheckersApp();

	virtual bool OnInit(void) override;
	virtual int OnExit(void) override;

	Thebe::GraphicsEngine* GetGraphicsEngine();
	Thebe::DynamicLineRenderer* GetLineRenderer();

	ChineseCheckersFrame* GetFrame();
	ChineseCheckersGameServer* GetGameServer();
	void SetGameServer(ChineseCheckersGameServer* gameServer);
	std::vector<ChineseCheckersGameClient*>& GetGameClientArray();
	HumanClient* GetHumanClient();
	Thebe::XBoxController* GetController();

	void ShutdownClientsAndServer();

private:
	ChineseCheckersFrame* frame;
	ChineseCheckersGameServer* gameServer;
	std::vector<ChineseCheckersGameClient*> gameClientArray;
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::Log> log;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::Reference<Thebe::DynamicLineRenderer> lineRenderer;
	Thebe::Reference<Thebe::XBoxController> controller;
};

wxDECLARE_APP(ChineseCheckersApp);