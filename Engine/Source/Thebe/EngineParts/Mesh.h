#pragma once

#include "Thebe/EngineParts/Space.h"

namespace Thebe
{
	class VertexBuffer;
	class Material;

	/**
	 *
	 */
	class THEBE_API Mesh : public Space
	{
	public:
		// TODO: Can we handle dynamic as well as static meshes here?
		Mesh();
		virtual ~Mesh();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

	private:
		Reference<VertexBuffer> vertexBuffer;
		Reference<Material> material;
	};
}