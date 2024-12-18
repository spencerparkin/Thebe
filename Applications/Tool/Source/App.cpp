#include "App.h"
#include "Frame.h"
#include "Canvas.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DirectionalLight.h"
#include "Thebe/NetLog.h"
#include <wx/image.h>
#include <wx/msgdlg.h>

wxIMPLEMENT_APP(GraphicsToolApp);

GraphicsToolApp::GraphicsToolApp()
{
	this->frame = nullptr;
	this->graphicsEngine.Set(new Thebe::GraphicsEngine());
}

/*virtual*/ GraphicsToolApp::~GraphicsToolApp()
{
}

Thebe::GraphicsEngine* GraphicsToolApp::GetGraphicsEngine()
{
	return this->graphicsEngine.Get();
}

Thebe::FreeCam* GraphicsToolApp::GetFreeCam()
{
	return &this->freeCam;
}

/*virtual*/ bool GraphicsToolApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	wxInitAllImageHandlers();

	this->frame = new GraphicsToolFrame(wxPoint(10, 10), wxSize(1200, 800));

	this->log.Set(new Thebe::Log());
	Thebe::Reference<Thebe::NetClientLogSink> logSink(new Thebe::NetClientLogSink());
	Thebe::NetworkAddress address;
	address.SetIPAddress("127.0.0.1");
	address.SetPort(12345);
	logSink->SetConnectAddress(address);
	this->log->AddSink(logSink);
	Thebe::Log::Set(this->log);

	HWND windowHandle = this->frame->GetCanvas()->GetHWND();
	if (!this->graphicsEngine->Setup(windowHandle))
		return false;

	Thebe::Reference<Thebe::DirectionalLight> light(new Thebe::DirectionalLight());
	light->Setup();
	Thebe::Transform lightToWorld;
	lightToWorld.LookAt(Thebe::Vector3(50.0, 100.0, 50.0), Thebe::Vector3(0.0, 0.0, 0.0), Thebe::Vector3(0.0, 1.0, 0.0));
	light->SetLightToWorldTransform(lightToWorld);
	this->graphicsEngine->SetLight(light);

	Thebe::Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 10.0);
	Thebe::Reference<Thebe::Camera> camera;
	camera.Set(new Thebe::PerspectiveCamera());
	camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(camera);

	this->frame->Show();

	this->freeCam.SetCamera(camera);

	return true;
}

/*virtual*/ int GraphicsToolApp::OnExit(void)
{
	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;
	return 0;
}