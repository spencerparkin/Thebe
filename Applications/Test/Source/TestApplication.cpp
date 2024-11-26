#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
//#include "Thebe/EngineParts/IndexBuffer.h"
//#include "Thebe/EngineParts/VertexBuffer.h"
//#include "Thebe/EngineParts/Shader.h"
//#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Mesh.h"

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

	/*
	std::vector<Thebe::Vector3> pointArray;
	pointArray.push_back(Thebe::Vector3(-1.0, -1.0, -1.0));
	pointArray.push_back(Thebe::Vector3(-1.0, -1.0, 1.0));
	pointArray.push_back(Thebe::Vector3(-1.0, 1.0, -1.0));
	pointArray.push_back(Thebe::Vector3(-1.0, 1.0, 1.0));
	pointArray.push_back(Thebe::Vector3(1.0, -1.0, -1.0));
	pointArray.push_back(Thebe::Vector3(1.0, -1.0, 1.0));
	pointArray.push_back(Thebe::Vector3(1.0, 1.0, -1.0));
	pointArray.push_back(Thebe::Vector3(1.0, 1.0, 1.0));

	Thebe::Reference<Thebe::IndexBuffer> indexBuffer;
	Thebe::Reference<Thebe::VertexBuffer> vertexBuffer;
	if (Thebe::Buffer::GenerateIndexAndVertexBuffersForConvexHull(pointArray, this->graphicsEngine.Get(), indexBuffer, vertexBuffer))
	{
		std::filesystem::path indexBufferPath = this->graphicsEngine->GetAssetFolder() / "Cube/Cube.index_buffer";
		this->graphicsEngine->DumpEnginePartToFile(indexBufferPath, indexBuffer, THEBE_DUMP_FLAG_CAN_OVERWRITE);

		std::filesystem::path vertexBufferPath = this->graphicsEngine->GetAssetFolder() / "Cube/Cube.vertex_buffer";
		this->graphicsEngine->DumpEnginePartToFile(vertexBufferPath, vertexBuffer, THEBE_DUMP_FLAG_CAN_OVERWRITE);
	}
	*/

	/*
	Thebe::Reference<Thebe::IndexBuffer> indexBuffer;
	Thebe::Reference<Thebe::VertexBuffer> vertexBuffer;
	Thebe::Reference<Thebe::EnginePart> enginePart;

	this->graphicsEngine->LoadEnginePartFromFile("Cube/Cube.index_buffer", enginePart);
	indexBuffer.SafeSet(enginePart.Get());

	this->graphicsEngine->LoadEnginePartFromFile("Cube/Cube.vertex_buffer", enginePart);
	vertexBuffer.SafeSet(enginePart.Get());
	*/

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