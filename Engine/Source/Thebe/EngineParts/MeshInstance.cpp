#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Mesh.h"
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
	if (!this->mesh.Get())
	{
		THEBE_LOG("No mesh set for instantiation.");
		return false;
	}

	// TODO: Create PSO.
	// TODO: Create constants buffer.
	// TODO: Allocate CBVs and SRVs from their respective heaps.

	return true;
}

/*virtual*/ void MeshInstance::Shutdown()
{
	// TODO: Deallocate CBVs and SRVs from their respective heaps.  Make stack allocator for this?
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