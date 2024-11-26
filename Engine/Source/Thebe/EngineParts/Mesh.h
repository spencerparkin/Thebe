#pragma once

#include "Thebe/EnginePart.h"

namespace Thebe
{
	class VertexBuffer;
	class IndexBuffer;
	class Material;
	class MeshInstance;

	/**
	 *
	 */
	class THEBE_API Mesh : public EnginePart
	{
	public:
		Mesh();
		virtual ~Mesh();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		/**
		 * 
		 */
		bool Instantiate(Reference<MeshInstance>& meshInstance);

	private:
		Reference<VertexBuffer> vertexBuffer;
		Reference<IndexBuffer> indexBuffer;
		Reference<Material> material;
	};
}