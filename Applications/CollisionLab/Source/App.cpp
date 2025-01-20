#include "App.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/RigidBody.h"

using namespace Thebe;

CollisionLabApp::CollisionLabApp()
{
}

/*virtual*/ CollisionLabApp::~CollisionLabApp()
{
}

/*virtual*/ bool CollisionLabApp::PrepareForWindowShow()
{
	this->graphicsEngine.Set(new GraphicsEngine());
	this->graphicsEngine->AddAssetFolder("Engine/Assets");
	this->graphicsEngine->AddAssetFolder("Applications/CollisionLab/Assets");
	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	Reference<Font> font;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Fonts\Roboto_Regular.font)", font))
		return false;

	this->text.Set(new Text());
	this->text->SetGraphicsEngine(this->graphicsEngine);
	this->text->SetFont(font);
	this->text->SetText("Hello!");
	this->text->SetRenderSpace(Text::CAMERA);
	this->text->SetTextColor(Vector3(1.0, 1.0, 1.0));
	if (!this->text->Setup())
		return false;

	Transform childToParent;
	childToParent.SetIdentity();
	childToParent.translation.SetComponents(-1.0, 0.5, -2.0);
	this->text->SetChildToParentTransform(childToParent);

	this->lineRenderer.Set(new DynamicLineRenderer());
	this->lineRenderer->SetGraphicsEngine(this->graphicsEngine);
	this->lineRenderer->SetLineMaxCount(1024);
	if (!this->lineRenderer->Setup())
		return false;

	Reference<Scene> scene(new Scene());
	scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());
	scene->SetRootSpace(this->text.Get());
	this->graphicsEngine->SetRenderObject(scene.Get());

	AxisAlignedBoundingBox worldBox;
	worldBox.minCorner.SetComponents(-1000.0, -1000.0, -1000.0);
	worldBox.maxCorner.SetComponents(1000.0, 1000.0, 1000.0);
	this->graphicsEngine->GetCollisionSystem()->SetWorldBox(worldBox);

	if (!this->graphicsEngine->LoadEnginePartFromFile("CollisionObjects/Icosahedron.collision_object", this->shapeA, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("CollisionObjects/Cube.collision_object", this->shapeB, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		return false;

	Transform objectToWorld = this->shapeB->GetObjectToWorld();
	objectToWorld.translation.x += 7.0;
	objectToWorld.translation.y += 0.0;
	objectToWorld.translation.z += 0.0;
	this->shapeB->SetObjectToWorld(objectToWorld);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 20.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);
	this->moverCam.SetCamera(this->camera);

	this->moverCam.AddMoveObject(this->shapeA);
	this->moverCam.AddMoveObject(this->shapeB);

	return true;
}

/*virtual*/ void CollisionLabApp::Shutdown(HINSTANCE instance)
{
	if (this->graphicsEngine.Get())
	{
		this->graphicsEngine->Shutdown();
		this->graphicsEngine = nullptr;
	}

	Application::Shutdown(instance);
}

/*virtual*/ LRESULT CollisionLabApp::OnPaint(WPARAM wParam, LPARAM lParam)
{
	this->lineRenderer->ResetLines();

	Vector3 origin(0.0, 0.0, 0.0);
	Vector3 xAxis(1.0, 0.0, 0.0), yAxis(0.0, 1.0, 0.0), zAxis(0.0, 0.0, 1.0);
	this->lineRenderer->AddLine(origin, xAxis, &xAxis, &xAxis);
	this->lineRenderer->AddLine(origin, yAxis, &yAxis, &yAxis);
	this->lineRenderer->AddLine(origin, zAxis, &zAxis, &zAxis);

	this->text->SetText("No collision.");
	this->text->SetTextColor(Vector3(1.0, 0.0, 0.0));
	std::vector<Reference<CollisionSystem::Collision>> collisionArray;
	this->graphicsEngine->GetCollisionSystem()->FindAllCollisions(this->shapeA.Get(), collisionArray);
	for (auto& collision : collisionArray)
	{
		if (collision->objectA.Get() == this->shapeB.Get() || collision->objectB.Get() == this->shapeB.Get())
		{
			this->text->SetText("Yes collision!");
			this->text->SetTextColor(Vector3(0.0, 1.0, 0.0));
			this->separationDelta = collision->separationDelta;
			this->RenderContacts(collision.Get(), this->lineRenderer.Get());
			this->RenderSeparationDelta(this->lineRenderer.Get());
			break;
		}
	}

	this->graphicsEngine->GetCollisionSystem()->DebugDraw(this->lineRenderer.Get());

	this->graphicsEngine->Render();

	this->moverCam.Update(this->graphicsEngine->GetDeltaTime());

	XBoxController* controller = this->moverCam.GetController();
	
	if (controller->WasButtonPressed(XINPUT_GAMEPAD_A))
	{
		Transform objectToWorld = this->shapeA->GetObjectToWorld();
		objectToWorld.translation += this->separationDelta;
		this->shapeA->SetObjectToWorld(objectToWorld);
		this->separationDelta = Vector3::Zero();
	}

	if (controller->WasButtonPressed(XINPUT_GAMEPAD_B))
	{
		Transform objectToWorld = this->shapeB->GetObjectToWorld();
		objectToWorld.translation -= this->separationDelta;
		this->shapeB->SetObjectToWorld(objectToWorld);
		this->separationDelta = Vector3::Zero();
	}

	return 0;
}

void CollisionLabApp::RenderSeparationDelta(Thebe::DynamicLineRenderer* lineRenderer)
{
	Vector3 color(1.0, 1.0, 1.0);
	lineRenderer->AddLine(Vector3::Zero(), this->separationDelta, &color, &color);
}

void CollisionLabApp::RenderContacts(Thebe::CollisionSystem::Collision* collision, Thebe::DynamicLineRenderer* lineRenderer)
{
	Thebe::Reference<Thebe::RigidBody> bodyA, bodyB;

	bodyA.Set(new Thebe::RigidBody());
	bodyB.Set(new Thebe::RigidBody());

	bodyA->SetCollisionObject(collision->objectA);
	bodyB->SetCollisionObject(collision->objectB);

	std::list<PhysicsSystem::Contact> contactList;
	Thebe::PhysicsSystem::ContactCalculator<GJKConvexHull, GJKConvexHull> contactCalculator;
	contactCalculator.CalculateContacts(bodyA, bodyB, contactList);

	Vector3 color(0.0, 1.0, 0.0);

	for (auto& contact : contactList)
	{
		Vector3 pointA = contact.surfacePoint;
		Vector3 pointB = pointA + contact.unitNormal;
		lineRenderer->AddLine(pointA, pointB, &color, &color);
	}
}

/*virtual*/ LRESULT CollisionLabApp::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}

/*virtual*/ const char* CollisionLabApp::GetWindowTitle()
{
	return "Collision Lab";
}