#pragma once

#include "Thebe/EngineParts/Space.h"

namespace Thebe
{
	class VertexBuffer;
	class IndexBuffer;
	class Material;

	/**
	 *
	 */
	class THEBE_API Mesh : public Space
	{
	public:
		Mesh();
		virtual ~Mesh();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

	private:
		Reference<VertexBuffer> vertexBuffer;
		Reference<Material> material;
	};
}