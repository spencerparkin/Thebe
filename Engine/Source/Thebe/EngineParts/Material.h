#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>

namespace Thebe
{
	class TextureBuffer;
	class CubeMapBuffer;
	class Shader;
	class Buffer;

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
		void SetCubeMapPath(const std::filesystem::path& cubeMapPath);
		void ClearAllTexturePaths();
		Shader* GetShader();
		D3D12_BLEND_DESC& GetBlendDesc();
		D3D12_DEPTH_STENCIL_DESC& GetDepthStencilDesc();
		UINT GetNumTextures();
		Buffer* GetTextureForRegister(UINT i);
		void SetCastsShadows(bool castsShadows);
		bool GetCastsShadows() const;
		CubeMapBuffer* GetCubeMap();
		bool RendersTransparency() const;

	private:
		Reference<Shader> shader;
		Reference<CubeMapBuffer> cubeMap;
		std::map<std::string, Reference<TextureBuffer>> textureMap;
		std::map<std::string, std::filesystem::path> textureFileMap;
		std::filesystem::path cubeMapPath;
		D3D12_BLEND_DESC blendDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
		std::filesystem::path shaderPath;
		bool castsShadows;
	};
}