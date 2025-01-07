#include "App.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Text.h"
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

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Cube.rigid_body", this->objectC, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/GroundSlab.rigid_body", this->groundSlab))
		return false;

	Transform objectToWorld = this->objectA->GetObjectToWorld();
	objectToWorld.translation.x += 0.0;
	objectToWorld.translation.y += 5.0;
	objectToWorld.translation.z += 0.0;
	this->objectA->SetObjectToWorld(objectToWorld);

	objectToWorld = this->objectC->GetObjectToWorld();
	objectToWorld.translation.x += 0.0;
	objectToWorld.translation.y += 10.0;
	objectToWorld.translation.z += 0.0;
	this->objectC->SetObjectToWorld(objectToWorld);

	Thebe::Reference<FramerateText> framerateText;
	framerateText.Set(new FramerateText());
	framerateText->SetGraphicsEngine(this->graphicsEngine);
	framerateText->SetFlags(THEBE_RENDER_OBJECT_FLAG_VISIBLE);
	if (!framerateText->Setup())
		return false;
	scene->SetRootSpace(framerateText);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 40.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);
	this->jediCam.SetCamera(this->camera);

	this->jediCam.AddObject(this->objectA);
	this->jediCam.AddObject(this->objectB);

	this->graphicsEngine->GetEventSystem()->RegisterEventHandler("collision_object", [=](const Event* event) { this->HandleCollisionObjectEvent(event); });

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

void PhysicsLabApp::HandleCollisionObjectEvent(const Event* event)
{
	auto collisionObjectEvent = (CollisionObjectEvent*)event;
	if (collisionObjectEvent->what == CollisionObjectEvent::COLLISION_OBJECT_NOT_IN_COLLISION_WORLD)
	{
		Transform objectToWorld;
		objectToWorld.SetIdentity();
		objectToWorld.translation.SetComponents(0.0, this->random.InRange(4.0, 15.0), 0.0);
		collisionObjectEvent->collisionObject->SetObjectToWorld(objectToWorld);
		this->graphicsEngine->GetCollisionSystem()->TrackObject(collisionObjectEvent->collisionObject);
	}
}

/*virtual*/ LRESULT PhysicsLabApp::OnPaint(WPARAM wParam, LPARAM lParam)
{
	CollisionSystem* collisionSystem = this->graphicsEngine->GetCollisionSystem();
	PhysicsSystem* physicsSystem = this->graphicsEngine->GetPhysicsSystem();
	EventSystem* eventSystem = this->graphicsEngine->GetEventSystem();

	this->lineRenderer->ResetLines();

	Vector3 origin(0.0, 0.0, 0.0);
	Vector3 xAxis(1.0, 0.0, 0.0), yAxis(0.0, 1.0, 0.0), zAxis(0.0, 0.0, 1.0);
	this->lineRenderer->AddLine(origin, xAxis, &xAxis, &xAxis);
	this->lineRenderer->AddLine(origin, yAxis, &yAxis, &yAxis);
	this->lineRenderer->AddLine(origin, zAxis, &zAxis, &zAxis);

	collisionSystem->DebugDraw(this->lineRenderer.Get());

	double deltaTimeSeconds = this->graphicsEngine->GetDeltaTime();
	physicsSystem->StepSimulation(deltaTimeSeconds, collisionSystem);

	eventSystem->DispatchAllEvents();

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

/*virtual*/ const char* PhysicsLabApp::GetWindowTitle()
{
	return "Physics Lab";
}