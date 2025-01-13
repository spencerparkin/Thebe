#pragma once

#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/Math/Vector4.h"
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class ConstantsBuffer;
	class Mesh;
	class Material;

	/**
	 *
	 */
	class THEBE_API MeshInstance : public Space
	{
	public:
		MeshInstance();
		virtual ~MeshInstance();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, RenderContext* context) override;
		virtual bool RendersToTarget(RenderTarget* renderTarget) const override;
		virtual uint32_t GetRenderOrder() const override;

		void SetMeshPath(const std::filesystem::path& meshPath);
		void SetOverrideMaterialPath(const std::filesystem::path& overrideMaterialPath);
		void SetMesh(Mesh* mesh);
		Mesh* GetMesh();
		void SetColor(const Vector4& color);
		const Vector4& GetColor() const;

	private:
		Reference<Mesh> mesh;
		Reference<Material> material;
		Reference<ConstantsBuffer> constantsBuffer;
		Reference<ConstantsBuffer> shadowConstantsBuffer;
		DescriptorHeap::DescriptorSet csuConstantsBufferDescriptorSet;
		DescriptorHeap::DescriptorSet csuMaterialTexturesDescriptorSet;
		DescriptorHeap::DescriptorSet csuShadowConstantsBufferDescriptorSet;
		Reference<Material> shadowMaterial;
		std::filesystem::path overrideMaterialPath;
		std::filesystem::path meshPath;
		Vector4 color;
	};
}