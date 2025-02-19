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
#include "GameClient.h"
#include "GameServer.h"
#include "HumanClient.h"
#include "ChineseCheckers/Test.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>

wxIMPLEMENT_APP(ChineseCheckersApp);

using namespace Thebe;

ChineseCheckersApp::ChineseCheckersApp()
{
	this->frame = nullptr;
	this->gameServer = nullptr;
	this->graphicsEngine.Set(new GraphicsEngine());
}

/*virtual*/ ChineseCheckersApp::~ChineseCheckersApp()
{
}

Thebe::DynamicLineRenderer* ChineseCheckersApp::GetLineRenderer()
{
	return this->lineRenderer.Get();
}

ChineseCheckersFrame* ChineseCheckersApp::GetFrame()
{
	return this->frame;
}

ChineseCheckersGameServer* ChineseCheckersApp::GetGameServer()
{
	return this->gameServer;
}

void ChineseCheckersApp::SetGameServer(ChineseCheckersGameServer* gameServer)
{
	this->gameServer = gameServer;
}

std::vector<ChineseCheckersGameClient*>& ChineseCheckersApp::GetGameClientArray()
{
	return this->gameClientArray;
}

HumanClient* ChineseCheckersApp::GetHumanClient()
{
	HumanClient* humanClient = nullptr;

	for (ChineseCheckersGameClient* gameClient : this->gameClientArray)
	{
		humanClient = dynamic_cast<HumanClient*>(gameClient);
		if (humanClient)
			break;
	}

	return humanClient;
}

Thebe::XBoxController* ChineseCheckersApp::GetController()
{
	return this->controller;
}

/*virtual*/ bool ChineseCheckersApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

#if false
	ChineseCheckers::TwoPlayerGameTest test;
	test.Perform();
#endif

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

	this->graphicsEngine->GetAudioSystem()->LoadAudioFromFolder("MidiSongs", this->graphicsEngine);

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

	Reference<Scene> scene;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Scenes\OceanScene.scene)", scene))
		return false;

	this->graphicsEngine->SetRenderObject(scene);
	if (this->lineRenderer.Get())
		scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());

#if defined THEBE_PROFILING
	Reference<ProfileTreeText> profileTreeText;
	profileTreeText.Set(new ProfileTreeText());
	profileTreeText->SetGraphicsEngine(this->graphicsEngine);
	profileTreeText->SetFlags(0);
	profileTreeText->SetName("ProfileText");
	if (!profileTreeText->Setup())
		return false;

	scene->GetRootSpace()->AddSubSpace(profileTreeText);
#else
	Reference<FramerateText> framerateText;
	framerateText.Set(new FramerateText());
	framerateText->SetGraphicsEngine(this->graphicsEngine);
	framerateText->SetFlags(0);
	framerateText->SetName("ProfileText");
	if (!framerateText->Setup())
		return false;

	scene->GetRootSpace()->AddSubSpace(framerateText);
#endif

	Reference<DirectionalLight> light(new DirectionalLight());
	light->Setup();
	Transform lightToWorld;
	lightToWorld.LookAt(Vector3(50.0, 100.0, 50.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
	light->SetLightToWorldTransform(lightToWorld);
	this->graphicsEngine->SetLight(light);

	Transform cameraToWorld;
	cameraToWorld.LookAt(Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);

	this->controller = new XBoxController(0);

	Reference<FreeCam> freeCam(new FreeCam());
	freeCam->SetXBoxController(this->controller);

	CameraSystem* cameraSystem = this->graphicsEngine->GetCameraSystem();
	cameraSystem->SetCamera(this->camera);
	cameraSystem->AddController("free_cam", freeCam);
	cameraSystem->SetActiveController("free_cam");

	Reference<Mesh> ringMesh;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Meshes\Ring.mesh)", ringMesh))
		return false;

	Reference<Material> mirrorMaterial;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Materials\PerfectMirror.material)", mirrorMaterial))
		return false;

	ringMesh->SetMaterial(mirrorMaterial);

	int numRingMeshInstances = 32;
	for (int i = 0; i < numRingMeshInstances; i++)
	{
		Reference<MeshInstance> ringMeshInstance(new MeshInstance());
		ringMeshInstance->SetGraphicsEngine(this->graphicsEngine);
		ringMeshInstance->SetMesh(ringMesh);
		ringMeshInstance->SetName(std::format("ring{}", i));
		ringMeshInstance->SetFlags(ringMeshInstance->GetFlags() & ~THEBE_RENDER_OBJECT_FLAG_VISIBLE);
		if (!ringMeshInstance->Setup())
			return false;

		scene->GetRootSpace()->AddSubSpace(ringMeshInstance);
	}

	this->frame->Show();

	return true;
}

/*virtual*/ int ChineseCheckersApp::OnExit(void)
{
	this->frame = nullptr;

	this->ShutdownClientsAndServer();

	this->graphicsEngine->WaitForGPUIdle();

	if (this->lineRenderer.Get())
	{
		this->lineRenderer->Shutdown();
		this->lineRenderer = nullptr;
	}

	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;

	this->controller = nullptr;

	THEBE_LOG("CloseLogViewer");

	return 0;
}

GraphicsEngine* ChineseCheckersApp::GetGraphicsEngine()
{
	return this->graphicsEngine.Get();
}

void ChineseCheckersApp::ShutdownClientsAndServer()
{
	for (ChineseCheckersGameClient* gameClient : this->gameClientArray)
	{
		gameClient->Shutdown();
		delete gameClient;
	}

	this->gameClientArray.clear();

	if (this->gameServer)
	{
		this->gameServer->Shutdown();
		delete this->gameServer;
		this->gameServer = nullptr;
	}
}