#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API Shader : public EnginePart
	{
	public:
		Shader();
		virtual ~Shader();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;

		struct Parameter
		{
			enum Type
			{
				FLOAT,
				VECTOR3,
				MATRIX3X3,
				MATRIX4X4
			};

			Type type;
			std::string name;
			UINT32 offset;
		};

		struct TextureAssociation
		{
			enum Type
			{
				DIFFUSE,
				NORMAL
			};

			Type type;
			UINT textureRegister;
			UINT samplerRegister;
		};

	private:
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		std::filesystem::path shaderFilePath;
		std::vector<TextureAssociation> textureAssocArray;
		std::vector<Parameter> parameterArray;
	};
}