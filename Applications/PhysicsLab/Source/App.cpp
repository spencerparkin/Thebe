#include "App.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Scene.h"

using namespace Thebe;

PhysicsLabApp::PhysicsLabApp()
{
}

/*virtual*/ PhysicsLabApp::~PhysicsLabApp()
{
}

/*virtual*/ bool PhysicsLabApp::PrepareForWindowShow()
{
	this->graphicsEngine.Set(new GraphicsEngine());
	this->graphicsEngine->AddAssetFolder("Engine/Assets");
	this->graphicsEngine->AddAssetFolder("Applications/CollisionLab/Assets");
	this->graphicsEngine->AddAssetFolder("Applications/PhysicsLab/Assets");
	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	this->lineRenderer.Set(new DynamicLineRenderer());
	this->lineRenderer->SetGraphicsEngine(this->graphicsEngine);
	this->lineRenderer->SetLineMaxCount(1024);
	if (!this->lineRenderer->Setup())
		return false;

	Reference<Scene> scene(new Scene());
	scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());
	this->graphicsEngine->SetRenderObject(scene.Get());

	AxisAlignedBoundingBox worldBox;
	worldBox.minCorner.SetComponents(-1000.0, -1000.0, -1000.0);
	worldBox.maxCorner.SetComponents(1000.0, 1000.0, 1000.0);
	this->graphicsEngine->GetCollisionSystem()->SetWorldBox(worldBox);

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Cube.rigid_body", this->objectA))
		return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Icosahedron.rigid_body", this->objectB))
		return false;

	Transform objectToWorld = this->objectB->GetObjectToWorld();
	objectToWorld.translation.x += 1.0;
	objectToWorld.translation.y += 1.0;
	objectToWorld.translation.z += 1.0;
	this->objectB->SetObjectToWorld(objectToWorld);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 20.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);
	this->jediCam.SetCamera(this->camera);

	this->jediCam.AddObject(this->objectA);
	this->jediCam.AddObject(this->objectB);

	return true;
}

/*virtual*/ void PhysicsLabApp::Shutdown(HINSTANCE instance)
{
	if (this->graphicsEngine.Get())
	{
		this->graphicsEngine->Shutdown();
		this->graphicsEngine = nullptr;
	}

	Application::Shutdown(instance);
}

/*virtual*/ LRESULT PhysicsLabApp::OnPaint(WPARAM wParam, LPARAM lParam)
{
	UINT lineOffset = 0;
	this->graphicsEngine->GetCollisionSystem()->DebugDraw(this->lineRenderer.Get(), lineOffset);
	this->lineRenderer->SetLineRenderCount(lineOffset);

	double deltaTimeSeconds = this->graphicsEngine->GetDeltaTime();
	this->graphicsEngine->GetPhysicsSystem()->StepSimulation(deltaTimeSeconds);

	this->graphicsEngine->Render();

	this->jediCam.Update(deltaTimeSeconds);

	return 0;
}

/*virtual*/ LRESULT PhysicsLabApp::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}