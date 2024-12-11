#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/DirectionalLight.h"

using namespace Thebe;

TestApplication::TestApplication()
{
}

/*virtual*/ TestApplication::~TestApplication()
{
}

/*virtual*/ bool TestApplication::PrepareForWindowShow()
{
	this->graphicsEngine.Set(new GraphicsEngine());

	// Tell the engine where it can find assets.  We're just using what assets the engine comes with by default.
	if (!this->graphicsEngine->AddAssetFolder("Engine/Assets"))
		return false;

	// Initialize the engine.
	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	// Load and configure the scene.
	Reference<Scene> scene;
	if (!graphicsEngine->LoadEnginePartFromFile(R"(Scenes\Silly.scene)", scene))
		return false;
	graphicsEngine->SetInputToAllRenderPasses(scene);

	// Load and configure a light source.
	Reference<DirectionalLight> light(new DirectionalLight());
	Transform lightToWorld;
	lightToWorld.LookAt(Vector3(50.0, 100.0, 50.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
	light->SetLightToWorldTransform(lightToWorld);
	graphicsEngine->SetLightForMainRenderPass(light);

	// Configure a camera.
	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 20.0, 50.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	graphicsEngine->SetCameraForMainRenderPass(this->camera);

	// Let the our free-cam control the camera.
	this->freeCam.SetCamera(this->camera);

	return true;
}

/*virtual*/ void TestApplication::Shutdown(HINSTANCE instance)
{
	if (this->graphicsEngine.Get())
	{
		this->graphicsEngine->Shutdown();
		this->graphicsEngine = nullptr;
	}

	Application::Shutdown(instance);
}

/*virtual*/ LRESULT TestApplication::OnPaint(WPARAM wParam, LPARAM lParam)
{
	this->graphicsEngine->Render();
	this->freeCam.Update(this->graphicsEngine->GetDeltaTime());
	return 0;
}

/*virtual*/ LRESULT TestApplication::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}