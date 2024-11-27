#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include <d3dx12.h>

using namespace Thebe;

Material::Material()
{
	::ZeroMemory(&this->blendDesc, sizeof(this->blendDesc));
}

/*virtual*/ Material::~Material()
{
}

/*virtual*/ bool Material::Setup()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!graphicsEngine->LoadEnginePartFromFile(this->shaderPath, this->shader))
	{
		THEBE_LOG("Failed to load shader file: %s", this->shaderPath.string().c_str());
		return false;
	}

	// TODO: Load textures too, if any configured.

	return true;
}

/*virtual*/ void Material::Shutdown()
{
}

/*virtual*/ bool Material::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{
	using namespace ParseParty;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root JSON value to be an object.");
		return false;
	}

	auto shaderValue = dynamic_cast<const JsonString*>(rootValue->GetValue("shader"));
	if (!shaderValue)
	{
		THEBE_LOG("No shader given.");
		return false;
	}

	this->shaderPath = shaderValue->GetValue();

	this->blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	auto alphaBlendingValue = dynamic_cast<const JsonBool*>(rootValue->GetValue("alpha_blending"));
	if (alphaBlendingValue && alphaBlendingValue->GetValue())
	{
		this->blendDesc.RenderTarget[0].BlendEnable = TRUE;
		this->blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		this->blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		this->blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	}

	// TODO: See what textures are configured.

	return true;
}

Shader* Material::GetShader()
{
	return this->shader;
}

D3D12_BLEND_DESC& Material::GetBlendDesc()
{
	return this->blendDesc;
}