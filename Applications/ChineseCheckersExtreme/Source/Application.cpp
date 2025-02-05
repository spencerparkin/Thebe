#include "Application.h"
#include "Frame.h"
#include "Canvas.h"
#include "Thebe/NetLog.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/DirectionalLight.h"
#include "Thebe/EngineParts/Text.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Material.h"
#include "Test.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>

wxIMPLEMENT_APP(ChineseCheckersApp);

using namespace Thebe;

ChineseCheckersApp::ChineseCheckersApp()
{
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

ChineseCheckersFrame* ChineseCheckersApp::GetFrame()
{
	return this->frame;
}

/*virtual*/ bool ChineseCheckersApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	ChineseCheckers::TwoPlayerGameTest test;
	test.Perform();

	this->frame = new ChineseCheckersFrame(wxPoint(10, 10), wxSize(1200, 800));

#if defined THEBE_LOGGING
	this->log.Set(new Log());
	Log::Set(this->log);
	this->log->AddSink(new LogConsoleSink());
#endif //THEBE_LOGGING

	HWND windowHandle = this->frame->GetCanvas()->GetHWND();
	if (!this->graphicsEngine->Setup(windowHandle))
		return false;

	if (!this->graphicsEngine->AddAssetFolder("Engine/Assets"))
		return false;

	if (!this->graphicsEngine->AddAssetFolder("Applications/ChineseCheckersExtreme/Assets"))
		return false;

	Reference<CubeMapBuffer> envMap;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Textures/OceanCubeMap/OceanCubeMap.cube_map)", envMap))
		return false;
	this->graphicsEngine->SetEnvMap(envMap);

	AxisAlignedBoundingBox worldBox;
	worldBox.minCorner.SetComponents(-1000.0, -1000.0, -1000.0);
	worldBox.maxCorner.SetComponents(1000.0, 1000.0, 1000.0);
	this->graphicsEngine->GetCollisionSystem()->SetWorldBox(worldBox);

#if defined _DEBUG
	this->lineRenderer.Set(new DynamicLineRenderer());
	this->lineRenderer->SetGraphicsEngine(this->graphicsEngine);
	this->lineRenderer->SetLineMaxCount(32 * 1024);
	if (!this->lineRenderer->Setup())
		return false;
#endif //_DEBUG

	Reference<FramerateText> framerateText;
	framerateText.Set(new FramerateText());
	framerateText->SetGraphicsEngine(this->graphicsEngine);
	framerateText->SetFlags(THEBE_RENDER_OBJECT_FLAG_VISIBLE);
	if (!framerateText->Setup())
		return false;

	Reference<Scene> scene;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Scenes\OceanScene.scene)", scene))
		return false;

	this->graphicsEngine->SetRenderObject(scene);
	scene->GetRootSpace()->AddSubSpace(framerateText);
	if (this->lineRenderer.Get())
		scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());

	Reference<DirectionalLight> light(new DirectionalLight());
	light->Setup();
	Transform lightToWorld;
	lightToWorld.LookAt(Vector3(50.0, 100.0, 50.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
	light->SetLightToWorldTransform(lightToWorld);
	this->graphicsEngine->SetLight(light);

	Transform cameraToWorld;
	cameraToWorld.LookAt(Vector3(0.0, 50.0, 100.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
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

	if (this->lineRenderer.Get())
	{
		this->lineRenderer->Shutdown();
		this->lineRenderer = nullptr;
	}

	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;

	THEBE_LOG("CloseLogViewer");

	return 0;
}

GraphicsEngine* ChineseCheckersApp::GetGraphicsEngine()
{
	return this->graphicsEngine.Get();
}