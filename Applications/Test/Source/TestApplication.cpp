#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/MeshInstance.h"

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

	Thebe::Reference<Thebe::Mesh> cubeMesh;
	this->graphicsEngine->LoadEnginePartFromFile("Cube/Cube.mesh", cubeMesh);

	Thebe::Reference<Thebe::MeshInstance> cubeMeshInstance(new Thebe::MeshInstance());
	cubeMeshInstance->SetGraphicsEngine(this->graphicsEngine);
	cubeMeshInstance->SetMesh(cubeMesh);
	cubeMeshInstance->Setup();

	cubeMeshInstance->Shutdown();

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