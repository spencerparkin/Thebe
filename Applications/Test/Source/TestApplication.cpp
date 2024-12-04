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

	if (!this->graphicsEngine->SetAssetFolder("Applications/Test/Assets"))
		return false;

	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

#if 0
	std::vector<Vector3> pointArray;
	pointArray.push_back(Vector3(-1.0, -1.0, -1.0));
	pointArray.push_back(Vector3(-1.0, -1.0, 1.0));
	pointArray.push_back(Vector3(-1.0, 1.0, -1.0));
	pointArray.push_back(Vector3(-1.0, 1.0, 1.0));
	pointArray.push_back(Vector3(1.0, -1.0, -1.0));
	pointArray.push_back(Vector3(1.0, -1.0, 1.0));
	pointArray.push_back(Vector3(1.0, 1.0, -1.0));
	pointArray.push_back(Vector3(1.0, 1.0, 1.0));
	Reference<IndexBuffer> indexBuffer;
	Reference<VertexBuffer> vertexBuffer;
	Buffer::GenerateIndexAndVertexBuffersForConvexHull(pointArray, this->graphicsEngine.Get(), indexBuffer, vertexBuffer);
	this->graphicsEngine->DumpEnginePartToFile(R"(E:\ENG_DEV\Thebe\Applications\Test\Assets\Cube\Cube.index_buffer)", indexBuffer.Get(), THEBE_DUMP_FLAG_CAN_OVERWRITE);
	this->graphicsEngine->DumpEnginePartToFile(R"(E:\ENG_DEV\Thebe\Applications\Test\Assets\Cube\Cube.vertex_buffer)", vertexBuffer.Get(), THEBE_DUMP_FLAG_CAN_OVERWRITE);
#endif

	Reference<Scene> scene;
	if (!graphicsEngine->LoadEnginePartFromFile(R"(E:\ENG_DEV\Thebe\Applications\Test\Assets\Scenes\Test.scene)", scene))
		return false;

	graphicsEngine->SetInputToAllRenderPasses(scene);

	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 0.0, 10.0);

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