#include "Thebe/EngineParts/MeshInstance.h"

using namespace Thebe;

MeshInstance::MeshInstance()
{
}

/*virtual*/ MeshInstance::~MeshInstance()
{
}

/*virtual*/ bool MeshInstance::Setup()
{
	return true;
}

/*virtual*/ void MeshInstance::Shutdown()
{
}

/*virtual*/ bool MeshInstance::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	//commandList->SetPipelineState(...);

	//commandList->DrawIndexed(...);

	return true;
}