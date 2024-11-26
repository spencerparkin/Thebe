#include "Thebe/EngineParts/Shader.h"

using namespace Thebe;

Shader::Shader()
{
}

/*virtual*/ Shader::~Shader()
{
}

/*virtual*/ bool Shader::Setup()
{
	return false;
}

/*virtual*/ void Shader::Shutdown()
{
}

/*virtual*/ bool Shader::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{

	D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc;
	std::vector<D3D12_DESCRIPTOR_RANGE1> discriptorRangeArray;
	std::vector<D3D12_ROOT_PARAMETER1> rootParameterArray;
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerArray;

	return false;
}