#include "Thebe/EngineParts/Material.h"

using namespace Thebe;

Material::Material()
{
}

/*virtual*/ Material::~Material()
{
}

/*virtual*/ bool Material::Setup()
{
	return true;
}

/*virtual*/ void Material::Shutdown()
{
}

void Material::Bind(ID3D12GraphicsCommandList* commandList)
{
	//commandList->SetPipelineState(...);
}