#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/Math/Transform.h"

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

	if (!this->graphicsEngine->SetAssetFolder("Applications/Test/Assets"))
		return false;

	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	Reference<Mesh> cubeMesh;
	if (!this->graphicsEngine->LoadEnginePartFromFile("Cube/Cube.mesh", cubeMesh))
		return false;

	Transform childToParent;
	childToParent.matrix.SetIdentity();
	childToParent.translation.SetComponents(0.0, 5.0, 0.0);

	Reference<MeshInstance> cubeMeshInstance(new MeshInstance());
	cubeMeshInstance->SetGraphicsEngine(this->graphicsEngine);
	cubeMeshInstance->SetMesh(cubeMesh);
	cubeMeshInstance->SetChildToParentTransform(childToParent);
	if (!cubeMeshInstance->Setup())
		return false;

	Reference<Space> worldSpace(new Space());
	worldSpace->AddSubSpace(cubeMeshInstance);

	Reference<Scene> scene(new Scene());
	scene->SetRootSpace(worldSpace);
	graphicsEngine->SetInputToAllRenderPasses(scene);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 5.0, 10.0);

	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	graphicsEngine->SetCameraForMainRenderPass(this->camera);

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
	return 0;
}

/*virtual*/ LRESULT TestApplication::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}