#pragma once

#include "Thebe/EngineParts/Space.h"
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
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

		void SetMesh(Mesh* mesh);
		Mesh* GetMesh();

	private:
		ComPtr<ID3D12PipelineState> pipelineState;
		Reference<Mesh> mesh;
		Reference<ConstantsBuffer> constantsBuffer;
	};
}