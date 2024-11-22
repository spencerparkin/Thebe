#pragma once

#include "Space.h"

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

	private:
		Reference<VertexBuffer> vertexBuffer;
		Reference<Material> material;
	};
}