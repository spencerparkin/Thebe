#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>

namespace Thebe
{
	class TextureBuffer;
	class Shader;

	/**
	 *
	 */
	class THEBE_API Material : public EnginePart
	{
	public:
		Material();
		virtual ~Material();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;

		void SetShaderPath(const std::filesystem::path& shaderPath);
		void SetTexturePath(const std::string& textureUsage, const std::filesystem::path& texturePath);
		void ClearAllTexturePaths();
		Shader* GetShader();
		D3D12_BLEND_DESC& GetBlendDesc();
		UINT GetNumTextures();
		TextureBuffer* GetTextureForRegister(UINT i);

	private:
		Reference<Shader> shader;
		std::map<std::string, Reference<TextureBuffer>> textureMap;
		std::map<std::string, std::filesystem::path> textureFileMap;
		D3D12_BLEND_DESC blendDesc;
		std::filesystem::path shaderPath;
	};
}