#include "Thebe/EngineParts/Shader.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <codecvt>

using namespace Thebe;

Shader::Shader()
{
	this->shadowMapRegister = -1;
	this->structuredBufferRegister = -1;
	this->constantsBufferTableIndex = -1;
	this->shadowMapTableIndex = -1;
	this->textureMapsTableIndex = -1;
	this->structuredBufferTableIndex = -1;
}

/*virtual*/ Shader::~Shader()
{
}

/*virtual*/ bool Shader::Setup()
{
	if (this->vertexShaderBlob.Get() || this->pixelShaderBlob.Get() || this->rootSignature.Get())
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

	HRESULT result = D3DReadFileToBlob(vsShaderObjFile.c_str(), &this->vertexShaderBlob);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to read pixel shader blob file %s.  Error: 0x%08x", this->vertesShaderBlobFile.c_str(), result);
		return false;
	}

	result = D3DReadFileToBlob(psShaderObjFile.c_str(), &this->pixelShaderBlob);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to read pixel shader blob file %s.  Error: 0x%08x", this->pixelShaderBlobFile.c_str(), result);
		return false;
	}

	D3D12_DESCRIPTOR_RANGE1 constantsBufferDescriptorRange;
	constantsBufferDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	constantsBufferDescriptorRange.BaseShaderRegister = 0;
	constantsBufferDescriptorRange.NumDescriptors = 1;
	constantsBufferDescriptorRange.RegisterSpace = 0;
	constantsBufferDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	constantsBufferDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 shadowBufferDescriptorRange;
	shadowBufferDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	shadowBufferDescriptorRange.BaseShaderRegister = this->shadowMapRegister;
	shadowBufferDescriptorRange.NumDescriptors = 1;
	shadowBufferDescriptorRange.RegisterSpace = 0;
	shadowBufferDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	shadowBufferDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	std::vector<D3D12_DESCRIPTOR_RANGE1> textureDescriptorRangeArray;
	textureDescriptorRangeArray.resize(this->textureRegisterMap.size());
	D3D12_DESCRIPTOR_RANGE1* textureDescriptorRange = textureDescriptorRangeArray.data();
	for (const auto& pair : this->textureRegisterMap)
	{
		UINT registerNumber = pair.first;
		textureDescriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		textureDescriptorRange->BaseShaderRegister = registerNumber;
		textureDescriptorRange->NumDescriptors = 1;
		textureDescriptorRange->RegisterSpace = 0;
		textureDescriptorRange->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
		textureDescriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		textureDescriptorRange++;
	}

	D3D12_DESCRIPTOR_RANGE1 structuredBufferDescriptorRange;
	structuredBufferDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	structuredBufferDescriptorRange.BaseShaderRegister = this->structuredBufferRegister;
	structuredBufferDescriptorRange.NumDescriptors = 1;
	structuredBufferDescriptorRange.RegisterSpace = 0;
	structuredBufferDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	structuredBufferDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	std::vector<D3D12_ROOT_PARAMETER1> rootParameterArray;
	
	D3D12_ROOT_PARAMETER1 constantsBufferRootParam;
	constantsBufferRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	constantsBufferRootParam.DescriptorTable.NumDescriptorRanges = 1;
	constantsBufferRootParam.DescriptorTable.pDescriptorRanges = &constantsBufferDescriptorRange;
	constantsBufferRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	this->constantsBufferTableIndex = (UINT)rootParameterArray.size();
	rootParameterArray.push_back(constantsBufferRootParam);

	if (this->shadowMapRegister != -1)
	{
		D3D12_ROOT_PARAMETER1 shadowMapRootParam;
		shadowMapRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		shadowMapRootParam.DescriptorTable.NumDescriptorRanges = 1;
		shadowMapRootParam.DescriptorTable.pDescriptorRanges = &shadowBufferDescriptorRange;
		shadowMapRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		this->shadowMapTableIndex = (UINT)rootParameterArray.size();
		rootParameterArray.push_back(shadowMapRootParam);
	}

	if (textureDescriptorRangeArray.size() > 0)
	{
		D3D12_ROOT_PARAMETER1 textureMapsRootParam;
		textureMapsRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		textureMapsRootParam.DescriptorTable.NumDescriptorRanges = (UINT)textureDescriptorRangeArray.size();
		textureMapsRootParam.DescriptorTable.pDescriptorRanges = textureDescriptorRangeArray.data();
		textureMapsRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		this->textureMapsTableIndex = (UINT)rootParameterArray.size();
		rootParameterArray.push_back(textureMapsRootParam);
	}

	if (this->structuredBufferRegister != -1)
	{
		D3D12_ROOT_PARAMETER1 structuredBufferRootParam;
		structuredBufferRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		structuredBufferRootParam.DescriptorTable.NumDescriptorRanges = (this->structuredBufferRegister == -1) ? 0 : 1;
		structuredBufferRootParam.DescriptorTable.pDescriptorRanges = (this->structuredBufferRegister == -1) ? nullptr : &structuredBufferDescriptorRange;
		structuredBufferRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		this->structuredBufferTableIndex = (UINT)rootParameterArray.size();
		rootParameterArray.push_back(structuredBufferRootParam);
	}

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignatureDesc.Desc_1_1.NumParameters = (UINT)rootParameterArray.size();
	rootSignatureDesc.Desc_1_1.pParameters = rootParameterArray.data();
	rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
	rootSignatureDesc.Desc_1_1.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;

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
	this->vertexShaderBlob = nullptr;
	this->pixelShaderBlob = nullptr;
	this->rootSignature = nullptr;
}

void Shader::SetRootParameters(ID3D12GraphicsCommandList* commandList,
	DescriptorHeap::DescriptorSet* constantsSet,
	DescriptorHeap::DescriptorSet* texturesSet,
	DescriptorHeap::DescriptorSet* shadowMapSet,
	DescriptorHeap::DescriptorSet* structuredBufferSet)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;

	if (constantsSet && constantsSet->IsAllocated())
	{
		constantsSet->GetGpuHandle(0, handle);
		commandList->SetGraphicsRootDescriptorTable(this->constantsBufferTableIndex, handle);
	}

	if (this->shadowMapRegister != -1)
	{
		if (shadowMapSet && shadowMapSet->IsAllocated())
		{
			shadowMapSet->GetGpuHandle(0, handle);
			commandList->SetGraphicsRootDescriptorTable(this->shadowMapTableIndex, handle);
		}
	}

	if (texturesSet && texturesSet->IsAllocated())
	{
		texturesSet->GetGpuHandle(0, handle);
		commandList->SetGraphicsRootDescriptorTable(this->textureMapsTableIndex, handle);
	}

	if (this->structuredBufferRegister != -1)
	{
		if (structuredBufferSet && structuredBufferSet->IsAllocated())
		{
			structuredBufferSet->GetGpuHandle(0, handle);
			commandList->SetGraphicsRootDescriptorTable(this->structuredBufferTableIndex, handle);
		}
	}
}

std::string Shader::GetTextureUsageForRegister(UINT registerNumber)
{
	auto pair = this->textureRegisterMap.find(registerNumber);
	if (pair == this->textureRegisterMap.end())
		return "?";

	return pair->second;
}

UINT Shader::GetNumTextureRegisters() const
{
	return (UINT)this->textureRegisterMap.size();
}

/*virtual*/ bool Shader::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root of JSON to be an object.");
		return false;
	}

	auto shadowMapRegisterValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("shadow_map_register"));
	if (shadowMapRegisterValue)
		this->shadowMapRegister = (UINT)shadowMapRegisterValue->GetValue();

	auto structuredBufferRegisterValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("structured_buffer_register"));
	if (structuredBufferRegisterValue)
		this->structuredBufferRegister = (UINT)structuredBufferRegisterValue->GetValue();

	auto textureRegisterMapValue = dynamic_cast<const JsonObject*>(rootValue->GetValue("texture_register_map"));
	if (textureRegisterMapValue)
	{
		this->textureRegisterMap.clear();
		for (const auto& textureRegisterValue : *textureRegisterMapValue)
		{
			const std::string& textureUsage = textureRegisterValue.first;
			auto registerValue = dynamic_cast<const JsonInt*>(textureRegisterValue.second);
			if (!registerValue)
			{
				THEBE_LOG("Expected each texture register entry to be an integer.");
				return false;
			}

			this->textureRegisterMap.insert(std::pair((UINT)registerValue->GetValue(), textureUsage));
		}
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
			else if (typeValue->GetValue() == "float4")
				parameter.type = Parameter::FLOAT4;
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

	return true;
}

UINT32 Shader::Parameter::GetSize() const
{
	switch (this->type)
	{
		case Type::FLOAT: return sizeof(float);
		case Type::FLOAT2: return 2 * sizeof(float);
		case Type::FLOAT3: return 3 * sizeof(float);
		case Type::FLOAT4: return 4 * sizeof(float);
		case Type::FLOAT2x2: return 2 * 2 * sizeof(float);
		case Type::FLOAT3x3: return 3 * 3 * sizeof(float);
		case Type::FLOAT4x4: return 4 * 4 * sizeof(float);
	}

	return 0;
}

const std::vector<Shader::Parameter>& Shader::GetParameterArray() const
{
	return this->parameterArray;
}

const Shader::Parameter* Shader::FindParameter(const std::string& name) const
{
	for (const Parameter& parameter : this->parameterArray)
		if (parameter.name == name)
			return &parameter;

	return nullptr;
}

ID3D12RootSignature* Shader::GetRootSignature()
{
	return this->rootSignature.Get();
}

ID3DBlob* Shader::GetVertexShaderBlob()
{
	return this->vertexShaderBlob.Get();
}

ID3DBlob* Shader::GetPixelShaderBlob()
{
	return this->pixelShaderBlob.Get();
}