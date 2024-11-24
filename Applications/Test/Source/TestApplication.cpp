#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"

TestApplication::TestApplication()
{
}

/*virtual*/ TestApplication::~TestApplication()
{
}

/*virtual*/ bool TestApplication::PrepareForWindowShow()
{
	this->graphicsEngine.Set(new Thebe::GraphicsEngine());

	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

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
		indexBuffer->Shutdown();
		vertexBuffer->Shutdown();
	}

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