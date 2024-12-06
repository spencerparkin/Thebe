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
	this->config = nullptr;
	this->graphicsEngine.Set(new Thebe::GraphicsEngine());
}

/*virtual*/ GraphicsToolApp::~GraphicsToolApp()
{
	delete this->config;
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

	this->config = new wxConfig("ThebeGraphicsToolConfig");

	wxInitAllImageHandlers();

	this->frame = new GraphicsToolFrame(wxPoint(10, 10), wxSize(1200, 800));

	int i = 0;
	while (true)
	{
		wxString assetFolder = this->config->Read(wxString::Format("AssetFolder%d", i++));
		if (assetFolder.length() == 0)
			break;
		
		std::filesystem::path assetFolderPath((const char*)assetFolder.c_str());
		if (!this->graphicsEngine->AddAssetFolder(assetFolderPath))
			wxMessageBox("Failed to configure asset folder: " + assetFolder, "Error!", wxICON_ERROR | wxOK);
	}

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
	int i = 0;
	for (const std::filesystem::path& assetFolderPath : this->graphicsEngine->GetAssetFolderList())
	{
		wxString assetFolder(assetFolderPath.string().c_str());
		this->config->Write(wxString::Format("AssetFolder%d", i++), assetFolder);
	}

	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;
	return 0;
}