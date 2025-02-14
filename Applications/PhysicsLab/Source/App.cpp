#include "App.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Text.h"
#include "Thebe/EngineParts/Scene.h"
#include "JediCam.h"

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

	//if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Cube.floppy_body", this->objectA))
	//	return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Cube.rigid_body", this->objectA))
		return false;

	//if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Icosahedron.rigid_body", this->objectB))
	//	return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Cube.rigid_body", this->objectB, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/Cube.rigid_body", this->objectC, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		return false;

	if (this->objectA.Get())
	{
		Transform objectToWorld = this->objectA->GetObjectToWorld();
		objectToWorld.matrix.SetFromAxisAngle(Vector3::ZAxis(), THEBE_PI / 4.0);
		objectToWorld.translation.x += 0.0;
		objectToWorld.translation.y += 5.0;
		objectToWorld.translation.z += 0.0;
		this->objectA->SetObjectToWorld(objectToWorld);
	}

	if (this->objectC.Get())
	{
		Transform objectToWorld = this->objectC->GetObjectToWorld();
		objectToWorld.translation.x += 0.0;
		objectToWorld.translation.y += 10.0;
		objectToWorld.translation.z += 0.0;
		this->objectC->SetObjectToWorld(objectToWorld);
	}

	if (!this->graphicsEngine->LoadEnginePartFromFile("PhysicsObjects/GroundSlab.rigid_body", this->groundSlab))
		return false;

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

	this->controller = new XBoxController(0);

	Reference<JediCam> jediCam(new JediCam);
	jediCam->SetXBoxController(this->controller);

	Thebe::CameraSystem* cameraSystem = this->graphicsEngine->GetCameraSystem();
	cameraSystem->SetCamera(this->camera);
	cameraSystem->AddController("jedi_cam", jediCam);
	cameraSystem->SetActiveController("jedi_cam");
	
	if (this->objectA.Get())
		jediCam->AddObject(this->objectA);

	if (this->objectB.Get())
		jediCam->AddObject(this->objectB);

	if (this->objectC.Get())
		jediCam->AddObject(this->objectC);

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
		RefHandle handle = (RefHandle)collisionObjectEvent->collisionObject->GetPhysicsData();
		Reference<PhysicsObject> physicsObject;
		if (HandleManager::Get()->GetObjectFromHandle(handle, physicsObject))
		{
			Transform objectToWorld;
			objectToWorld.SetIdentity();
			objectToWorld.translation.SetComponents(0.0, this->random.InRange(4.0, 15.0), 0.0);
			physicsObject->SetObjectToWorld(objectToWorld);
			this->graphicsEngine->GetCollisionSystem()->TrackObject(collisionObjectEvent->collisionObject);
		}
	}
}

/*virtual*/ LRESULT PhysicsLabApp::OnPaint(WPARAM wParam, LPARAM lParam)
{
	this->controller->Update();

	CollisionSystem* collisionSystem = this->graphicsEngine->GetCollisionSystem();
	PhysicsSystem* physicsSystem = this->graphicsEngine->GetPhysicsSystem();
	EventSystem* eventSystem = this->graphicsEngine->GetEventSystem();
	CameraSystem* cameraSystem = this->graphicsEngine->GetCameraSystem();

	this->lineRenderer->ResetLines();

	Vector3 origin(0.0, 0.0, 0.0);
	Vector3 xAxis(1.0, 0.0, 0.0), yAxis(0.0, 1.0, 0.0), zAxis(0.0, 0.0, 1.0);
	this->lineRenderer->AddLine(origin, xAxis, &xAxis, &xAxis);
	this->lineRenderer->AddLine(origin, yAxis, &yAxis, &yAxis);
	this->lineRenderer->AddLine(origin, zAxis, &zAxis, &zAxis);

	collisionSystem->DebugDraw(this->lineRenderer.Get());
	physicsSystem->DebugDraw(this->lineRenderer.Get());

	double deltaTimeSeconds = this->graphicsEngine->GetDeltaTime();
	physicsSystem->StepSimulation(deltaTimeSeconds, collisionSystem);

	eventSystem->DispatchAllEvents();

	this->graphicsEngine->Render();

	cameraSystem->Update(deltaTimeSeconds);

	if (this->controller->WasButtonPressed(XINPUT_GAMEPAD_A))
	{
		if (this->objectA.Get())
		{
			Transform objectToWorld = this->objectA->GetObjectToWorld();
			objectToWorld.matrix.SetFromAxisAngle(Vector3::ZAxis(), THEBE_PI / 4.0);
			objectToWorld.translation.x = 0.0;
			objectToWorld.translation.y = -8.0;
			objectToWorld.translation.z = 0.0;
			this->objectA->SetObjectToWorld(objectToWorld);
			this->objectA->ZeroMomentum();
		}
	}

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