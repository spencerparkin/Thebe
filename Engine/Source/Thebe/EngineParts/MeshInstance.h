#pragma once

#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
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

		void SetMeshPath(std::filesystem::path& meshPath);
		void SetMesh(Mesh* mesh);
		Mesh* GetMesh();

	private:
		ComPtr<ID3D12PipelineState> pipelineState;
		ComPtr<ID3D12PipelineState> shadowPipelineState;
		Reference<Mesh> mesh;
		Reference<ConstantsBuffer> constantsBuffer;
		Reference<ConstantsBuffer> shadowConstantsBuffer;
		DescriptorHeap::DescriptorSet csuDescriptorSet;
		DescriptorHeap::DescriptorSet csuShadowDescriptorSet;
		Reference<Material> shadowMaterial;
		std::filesystem::path meshPath;
	};
}