#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/CubeMapBuffer.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include <d3dx12.h>

using namespace Thebe;

Material::Material()
{
	this->castsShadows = true;
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

	this->textureMap.clear();
	for (auto pair : this->textureFileMap)
	{
		const std::string& textureUsage = pair.first;
		const std::filesystem::path& textureFilePath = pair.second;
		
		Reference<TextureBuffer> texture;
		if (!graphicsEngine->LoadEnginePartFromFile(textureFilePath, texture))
		{
			THEBE_LOG("Failed to load texture file: %s", textureFilePath.string().c_str());
			return false;
		}

		this->textureMap.insert(std::pair(textureUsage, texture));
	}

	if (this->cubeMapPath.string().length() > 0)
	{
		if (!graphicsEngine->LoadEnginePartFromFile(this->cubeMapPath, this->cubeMap))
		{
			THEBE_LOG("Failed to load cube-map file: %s", this->cubeMapPath.string().c_str());
			return false;
		}
	}

	return true;
}

/*virtual*/ void Material::Shutdown()
{
	// Note that we don't shutdown shaders or textures, because
	// some other material may be using them.  Shutdown happens
	// when the cache is purged when the engine goes down.
	this->shader = nullptr;
	this->textureMap.clear();
	this->textureFileMap.clear();
}

/*virtual*/ bool Material::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

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

	this->textureFileMap.clear();
	auto textureMapValue = dynamic_cast<const JsonObject*>(rootValue->GetValue("texture_map"));
	if (textureMapValue)
	{
		for (auto pair : *textureMapValue)
		{
			auto textureFilePathValue = dynamic_cast<const JsonString*>(pair.second);
			if (!textureFilePathValue)
			{
				THEBE_LOG("Expected texture entry to be a string.");
				return false;
			}

			const std::string& textureUsage = pair.first;
			const std::string& textureFilePath = textureFilePathValue->GetValue();
			this->textureFileMap.insert(std::pair(textureUsage, textureFilePath));
		}
	}

	auto cubeMapValue = dynamic_cast<const JsonString*>(rootValue->GetValue("cube_map"));
	if (cubeMapValue)
		this->cubeMapPath = cubeMapValue->GetValue();

	auto castsShadowsValue = dynamic_cast<const JsonBool*>(rootValue->GetValue("casts_shadows"));
	if (castsShadowsValue)
		this->castsShadows = castsShadowsValue->GetValue();

	return true;
}

/*virtual*/ bool Material::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!EnginePart::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("shader", new JsonString(this->shaderPath.string()));
	rootValue->SetValue("alpha_blending", new JsonBool(this->blendDesc.RenderTarget[0].BlendEnable == TRUE));

	auto textureMapValue = new JsonObject();
	rootValue->SetValue("texture_map", textureMapValue);

	for (auto pair : this->textureFileMap)
	{
		const std::string& textureUsage = pair.first;
		const std::filesystem::path& textureFilePath = pair.second;
		textureMapValue->SetValue(textureUsage, new JsonString(textureFilePath.string()));
	}

	if (this->cubeMapPath.string().length() > 0)
		rootValue->SetValue("cube_map", new JsonString(this->cubeMapPath.string()));

	rootValue->SetValue("casts_shadows", new JsonBool(this->castsShadows));

	return true;
}

void Material::SetShaderPath(const std::filesystem::path& shaderPath)
{
	this->shaderPath = shaderPath;
}

void Material::SetTexturePath(const std::string& textureUsage, const std::filesystem::path& texturePath)
{
	if (this->textureFileMap.find(textureUsage) != this->textureFileMap.end())
		this->textureFileMap.erase(textureUsage);

	this->textureFileMap.insert(std::pair(textureUsage, texturePath));
}

void Material::SetCubeMapPath(const std::filesystem::path& cubeMapPath)
{
	this->cubeMapPath = cubeMapPath;
}

void Material::ClearAllTexturePaths()
{
	this->textureFileMap.clear();
}

Shader* Material::GetShader()
{
	return this->shader;
}

D3D12_BLEND_DESC& Material::GetBlendDesc()
{
	return this->blendDesc;
}

UINT Material::GetNumTextures()
{
	return (UINT)this->textureMap.size();
}

Buffer* Material::GetTextureForRegister(UINT i)
{
	if (!this->shader.Get())
		return nullptr;

	std::string textureUsage = this->shader->GetTextureUsageForRegister(i);
	if (textureUsage == "cube_map")
		return this->cubeMap;

	if (textureUsage == "env_map")
	{
		Reference<GraphicsEngine> graphicsEngine;
		if (!this->GetGraphicsEngine(graphicsEngine))
			return nullptr;

		return graphicsEngine->GetEnvMap();
	}

	auto iter = this->textureMap.find(textureUsage);
	if (iter == this->textureMap.end())
		return nullptr;

	TextureBuffer* texture = iter->second.Get();
	return texture;
}

void Material::SetCastsShadows(bool castsShadows)
{
	this->castsShadows = castsShadows;
}

bool Material::GetCastsShadows() const
{
	return this->castsShadows;
}

CubeMapBuffer* Material::GetCubeMap()
{
	return this->cubeMap;
}