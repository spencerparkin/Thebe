#include "App.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Font.h"

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

	if (!this->graphicsEngine->LoadEnginePartFromFile("CollisionObjects/Cube.collision_object", this->cubeA))
		return false;

	if (!this->graphicsEngine->LoadEnginePartFromFile("CollisionObjects/Cube.collision_object", this->cubeB, THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		return false;

	Transform objectToWorld = this->cubeB->GetObjectToWorld();
	objectToWorld.translation.x += 1.0;
	objectToWorld.translation.y += 1.0;
	objectToWorld.translation.z += 1.0;
	this->cubeB->SetObjectToWorld(objectToWorld);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 20.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);
	this->moverCam.SetCamera(this->camera);

	this->moverCam.AddMoveObject(this->cubeA);
	this->moverCam.AddMoveObject(this->cubeB);

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
	this->text->SetText("No collision.");
	this->text->SetTextColor(Vector3(1.0, 0.0, 0.0));
	std::vector<Reference<CollisionSystem::Collision>> collisionArray;
	this->graphicsEngine->GetCollisionSystem()->FindAllCollisions(this->cubeA.Get(), collisionArray);
	for (auto& collision : collisionArray)
	{
		if (collision->objectA.Get() == this->cubeB.Get() || collision->objectB.Get() == this->cubeB.Get())
		{
			this->text->SetText("Yes collision!");
			this->text->SetTextColor(Vector3(0.0, 1.0, 0.0));
			break;
		}
	}

	UINT lineOffset = 0;
	this->graphicsEngine->GetCollisionSystem()->DebugDraw(this->lineRenderer.Get(), lineOffset);
	this->lineRenderer->SetLineRenderCount(lineOffset);

	this->graphicsEngine->Render();

	this->moverCam.Update(this->graphicsEngine->GetDeltaTime());

	return 0;
}

/*virtual*/ LRESULT CollisionLabApp::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}