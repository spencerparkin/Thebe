#include "App.h"
#include "Frame.h"
#include "Canvas.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/EngineParts/Camera.h"
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

	HWND windowHandle = this->frame->GetCanvas()->GetHWND();
	if (!this->graphicsEngine->Setup(windowHandle))
		return false;

	Thebe::Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 10.0);

	Thebe::Reference<Thebe::Camera> camera;
	camera.Set(new Thebe::PerspectiveCamera());
	camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCameraForMainRenderPass(camera);

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