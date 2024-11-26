#pragma once

#include "Thebe/EngineParts/Space.h"

namespace Thebe
{
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

	private:
		Reference<Mesh> mesh;
		Reference<ConstantsBuffer> constantsBuffer;
	};
}