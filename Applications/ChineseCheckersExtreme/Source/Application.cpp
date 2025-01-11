#include "Application.h"
#include "Frame.h"
#include "Canvas.h"
#include "Thebe/NetLog.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/DirectionalLight.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>

wxIMPLEMENT_APP(ChineseCheckersApp);

using namespace Thebe;

ChineseCheckersApp::ChineseCheckersApp()
{
	this->gameClient = nullptr;
	this->gameServer = nullptr;
	this->frame = nullptr;
	this->graphicsEngine.Set(new GraphicsEngine());
}

/*virtual*/ ChineseCheckersApp::~ChineseCheckersApp()
{
}

Thebe::FreeCam* ChineseCheckersApp::GetFreeCam()
{
	return &this->freeCam;
}

Thebe::DynamicLineRenderer* ChineseCheckersApp::GetLineRenderer()
{
	return this->lineRenderer.Get();
}

/*virtual*/ bool ChineseCheckersApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	this->frame = new ChineseCheckersFrame(wxPoint(10, 10), wxSize(1200, 800));

	wxFileName loggerPath(wxStandardPaths::Get().GetExecutablePath());
	loggerPath.SetName("ThebeLogViewer");
	loggerPath.SetExt("exe");
	wxString fullPath = loggerPath.GetFullPath();
	if (loggerPath.FileExists())
	{
		wxString command = loggerPath.GetFullPath() + " --port=12345 --addr=127.0.0.1";
		long loggerPID = wxExecute(command, wxEXEC_ASYNC);
		if (loggerPID != 0)
		{
			this->log.Set(new Thebe::Log());
			Thebe::Reference<Thebe::NetLogSink> logSink(new Thebe::NetLogSink());
			Thebe::NetworkAddress address;
			address.SetIPAddress("127.0.0.1");
			address.SetPort(12345);
			logSink->SetConnectAddress(address);
			this->log->AddSink(logSink);
			Thebe::Log::Set(this->log);
		}
	}

	HWND windowHandle = this->frame->GetCanvas()->GetHWND();
	if (!this->graphicsEngine->Setup(windowHandle))
		return false;

	if (!this->graphicsEngine->AddAssetFolder("Engine/Assets"))
		return false;

	if (!this->graphicsEngine->AddAssetFolder("Applications/ChineseCheckersExtreme/Assets"))
		return false;

	this->lineRenderer.Set(new DynamicLineRenderer());
	this->lineRenderer->SetGraphicsEngine(this->graphicsEngine);
	this->lineRenderer->SetLineMaxCount(1024);
	if (!this->lineRenderer->Setup())
		return false;

	Reference<Scene> scene;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Scenes\OceanScene.scene)", scene))
		return false;
	this->graphicsEngine->SetRenderObject(scene);
	scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());

	Reference<DirectionalLight> light(new DirectionalLight());
	light->Setup();
	Transform lightToWorld;
	lightToWorld.LookAt(Vector3(50.0, 100.0, 50.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
	light->SetLightToWorldTransform(lightToWorld);
	this->graphicsEngine->SetLight(light);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 50.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);
	this->freeCam.SetCamera(this->camera);

	this->frame->Show();

	return true;
}

/*virtual*/ int ChineseCheckersApp::OnExit(void)
{
	this->graphicsEngine->WaitForGPUIdle();

	this->lineRenderer->Shutdown();
	this->lineRenderer = nullptr;

	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;

	THEBE_LOG("CloseLogViewer");

	return 0;
}

GraphicsEngine* ChineseCheckersApp::GetGraphicsEngine()
{
	return this->graphicsEngine.Get();
}

ChineseCheckersClient* ChineseCheckersApp::GetGameClient()
{
	return this->gameClient;
}

ChineseCheckersServer* ChineseCheckersApp::GetGameServer()
{
	return this->gameServer;
}

void ChineseCheckersApp::SetGameClient(ChineseCheckersClient* gameClient)
{
	this->gameClient = gameClient;
}

void ChineseCheckersApp::SetGameServer(ChineseCheckersServer* gameServer)
{
	this->gameServer = gameServer;
}