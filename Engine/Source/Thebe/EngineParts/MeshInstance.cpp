#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/Log.h"

using namespace Thebe;

MeshInstance::MeshInstance()
{
}

/*virtual*/ MeshInstance::~MeshInstance()
{
}

/*virtual*/ bool MeshInstance::Setup()
{
	if (this->constantsBuffer.Get())
	{
		THEBE_LOG("Mesh instance already setup.");
		return false;
	}

	if (!this->mesh.Get())
	{
		THEBE_LOG("No mesh set for instantiation.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	this->constantsBuffer.Set(new ConstantsBuffer());
	this->constantsBuffer->SetGraphicsEngine(graphicsEngine);
	this->constantsBuffer->SetShader(this->mesh->GetMaterial()->GetShader());
	if (!this->constantsBuffer->Setup())
	{
		THEBE_LOG("Failed to setup constants buffer for mesh instance.");
		return false;
	}

	// TODO: Create PSO.

	return true;
}

/*virtual*/ void MeshInstance::Shutdown()
{
	if (this->constantsBuffer.Get())
	{
		this->constantsBuffer->Shutdown();
		this->constantsBuffer = nullptr;
	}
}

/*virtual*/ bool MeshInstance::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	//commandList->SetPipelineState(...);

	//commandList->DrawIndexed(...);

	return true;
}

void MeshInstance::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
}

Mesh* MeshInstance::GetMesh()
{
	return this->mesh;
}