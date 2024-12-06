#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"

using namespace Thebe;

TestApplication::TestApplication()
{
}

/*virtual*/ TestApplication::~TestApplication()
{
}

/*virtual*/ bool TestApplication::PrepareForWindowShow()
{
	this->graphicsEngine.Set(new Thebe::GraphicsEngine());

	if (!this->graphicsEngine->AddAssetFolder("Engine/Assets"))
		return false;

	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	Reference<Scene> scene;
	if (!graphicsEngine->LoadEnginePartFromFile(R"(Scenes\Test.scene)", scene))
		return false;

	graphicsEngine->SetInputToAllRenderPasses(scene);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 10.0);

	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	graphicsEngine->SetCameraForMainRenderPass(this->camera);

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