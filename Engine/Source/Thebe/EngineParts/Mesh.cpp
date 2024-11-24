#include "Thebe/EngineParts/Mesh.h"

using namespace Thebe;

Mesh::Mesh()
{
}

/*virtual*/ Mesh::~Mesh()
{
}

/*virtual*/ bool Mesh::Setup()
{
	return true;
}

/*virtual*/ void Mesh::Shutdown()
{
}

/*virtual*/ bool Mesh::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	// TODO: Call commandList->SetPipelineState() with our material.

	return true;
}