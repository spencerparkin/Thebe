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
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;

		struct Parameter
		{
			enum Type
			{
				UNKNOWN,
				FLOAT,
				FLOAT2,
				FLOAT3,
				FLOAT2x2,
				FLOAT3x3,
				FLOAT4x4
			};

			UINT32 GetSize() const;

			Type type;
			std::string name;
			UINT32 offset;
		};

		const std::vector<Parameter>& GetParameterArray() const;
		const Parameter* FindParameter(const std::string& name) const;

		ID3D12RootSignature* GetRootSignature();
		ID3DBlob* GetVertexShaderBlob();
		ID3DBlob* GetPixelShaderBlob();

		std::string GetTextureUsageForRegister(UINT registerNumber);

	private:
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
		std::filesystem::path vertesShaderBlobFile;
		std::filesystem::path pixelShaderBlobFile;
		std::vector<Parameter> parameterArray;
	};
}