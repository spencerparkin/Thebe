#include "App.h"
#include "Thebe/EngineParts/Scene.h"

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

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 20.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);
	this->freeCam.SetCamera(this->camera);

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
	this->graphicsEngine->Render();

	this->freeCam.Update(this->graphicsEngine->GetDeltaTime());

	return 0;
}

/*virtual*/ LRESULT CollisionLabApp::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}