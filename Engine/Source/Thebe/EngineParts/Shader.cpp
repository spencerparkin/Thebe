#include "Thebe/EngineParts/Shader.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <codecvt>

using namespace Thebe;

Shader::Shader()
{
}

/*virtual*/ Shader::~Shader()
{
}

/*virtual*/ bool Shader::Setup()
{
	if (this->vertesShaderBlob.Get() || this->pixelShaderBlob.Get() || this->rootSignature.Get())
	{
		THEBE_LOG("Shader already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!graphicsEngine->ResolvePath(this->vertesShaderBlobFile, GraphicsEngine::RELATIVE_TO_ASSET_FOLDER))
	{
		THEBE_LOG("Failed to resolve path: %s", this->vertesShaderBlobFile.string().c_str());
		return false;
	}

	if (!graphicsEngine->ResolvePath(this->pixelShaderBlobFile, GraphicsEngine::RELATIVE_TO_ASSET_FOLDER))
	{
		THEBE_LOG("Failed to resolve path: %s", this->pixelShaderBlobFile.string().c_str());
		return false;
	}

	std::wstring vsShaderObjFile = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->vertesShaderBlobFile.string());
	std::wstring psShaderObjFile = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->pixelShaderBlobFile.string());

	HRESULT result = D3DReadFileToBlob(vsShaderObjFile.c_str(), &this->vertesShaderBlob);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to read pixel shader blob file %s.  Error: 0x%08x", this->vertesShaderBlobFile.c_str(), result);
		return false;
	}

	result = D3DReadFileToBlob(vsShaderObjFile.c_str(), &this->pixelShaderBlob);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to read pixel shader blob file %s.  Error: 0x%08x", this->pixelShaderBlobFile.c_str(), result);
		return false;
	}

	std::vector<D3D12_DESCRIPTOR_RANGE1> descriptorRangeArray;
	descriptorRangeArray.resize(1);
	descriptorRangeArray[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorRangeArray[0].BaseShaderRegister = 0;
	descriptorRangeArray[0].NumDescriptors = 1;
	descriptorRangeArray[0].RegisterSpace = 0;
	descriptorRangeArray[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
	descriptorRangeArray[0].OffsetInDescriptorsFromTableStart = 0;

	std::vector<D3D12_ROOT_PARAMETER1> rootParameterArray;
	rootParameterArray.resize(1);

	rootParameterArray[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameterArray[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameterArray[0].DescriptorTable.pDescriptorRanges = descriptorRangeArray.data();
	rootParameterArray[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignatureDesc.Desc_1_1.NumParameters = (UINT)rootParameterArray.size();
	rootSignatureDesc.Desc_1_1.pParameters = rootParameterArray.data();
	rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	// TODO: Use texture associations to add SRV thingies to root signature?
	// TODO: Also need to add sampler description before we create the root signature.

	ComPtr<ID3DBlob> signatureBlob, errorBob;
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signatureBlob, &errorBob);
	if (FAILED(result))
	{
		// TODO: Get error message from blob?
		THEBE_LOG("Failed to serialize root signature description.  Error: 0x%08x", result);
		return false;
	}

	result = graphicsEngine->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&this->rootSignature));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create root signature.  Error: 0x%08x", result);
		return false;
	}

	return true;
}

/*virtual*/ void Shader::Shutdown()
{
	this->vertesShaderBlob = nullptr;
	this->pixelShaderBlob = nullptr;
	this->rootSignature = nullptr;
}

/*virtual*/ bool Shader::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{
	using namespace ParseParty;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root of JSON to be an object.");
		return false;
	}

	this->parameterArray.clear();
	auto constantsMapValue = dynamic_cast<const JsonObject*>(rootValue->GetValue("constants"));
	if (constantsMapValue)
	{
		for (auto pair : *constantsMapValue)
		{
			Parameter parameter;
			parameter.name = pair.first;
			
			auto parameterInfoValue = dynamic_cast<const JsonObject*>(pair.second);
			if (!parameterInfoValue)
			{
				THEBE_LOG("Expected parameter info to be an object.");
				return false;
			}

			auto offsetValue = dynamic_cast<const JsonInt*>(parameterInfoValue->GetValue("offset"));
			if (!offsetValue)
			{
				THEBE_LOG("Parameter \"%s\" has no offset value.", parameter.name.c_str());
				return false;
			}

			parameter.offset = (UINT32)offsetValue->GetValue();

			auto typeValue = dynamic_cast<const JsonString*>(parameterInfoValue->GetValue("type"));
			if (!typeValue)
			{
				THEBE_LOG("Parameter \"%s\" has no type value.", parameter.name.c_str());
				return false;
			}

			if (typeValue->GetValue() == "float")
				parameter.type = Parameter::FLOAT;
			else if (typeValue->GetValue() == "float2")
				parameter.type = Parameter::FLOAT2;
			else if (typeValue->GetValue() == "float3")
				parameter.type = Parameter::FLOAT3;
			else if (typeValue->GetValue() == "float2x2")
				parameter.type = Parameter::FLOAT2x2;
			else if (typeValue->GetValue() == "float3x3")
				parameter.type = Parameter::FLOAT3x3;
			else if (typeValue->GetValue() == "float4x4")
				parameter.type = Parameter::FLOAT4x4;
			else
			{
				THEBE_LOG("Could not decypher type \"%s\" for parameter \"%s\".", typeValue->GetValue().c_str(), parameter.name.c_str());
				return false;
			}

			this->parameterArray.push_back(parameter);
		}
	}

	auto vertexShaderBlobFileValue = dynamic_cast<const JsonString*>(rootValue->GetValue("vs_shader_object"));
	if (!vertexShaderBlobFileValue)
	{
		THEBE_LOG("No vertex shader object file given.");
		return false;
	}

	auto pixelShaderBlobFileValue = dynamic_cast<const JsonString*>(rootValue->GetValue("ps_shader_object"));
	if (!pixelShaderBlobFileValue)
	{
		THEBE_LOG("No pixel shader object file given.");
		return false;
	}

	this->vertesShaderBlobFile = vertexShaderBlobFileValue->GetValue();
	this->pixelShaderBlobFile = pixelShaderBlobFileValue->GetValue();

	// TODO: Read in texture associations.  These are needed to help create our root signature.

	return true;
}