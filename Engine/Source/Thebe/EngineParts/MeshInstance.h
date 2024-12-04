#pragma once

#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class ConstantsBuffer;
	class Mesh;

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
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

		void SetMeshMesh(std::filesystem::path& meshPath);
		void SetMesh(Mesh* mesh);
		Mesh* GetMesh();

	private:
		ComPtr<ID3D12PipelineState> pipelineState;
		Reference<Mesh> mesh;
		Reference<ConstantsBuffer> constantsBuffer;
		DescriptorHeap::DescriptorSet csuDescriptorSet;
		std::filesystem::path meshPath;
	};
}