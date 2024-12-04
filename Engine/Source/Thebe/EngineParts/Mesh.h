#pragma once

#include "Thebe/EnginePart.h"

namespace Thebe
{
	class VertexBuffer;
	class IndexBuffer;
	class Material;
	class MeshInstance;

	/**
	 * Meshes are the fundamental rendering asset of the graphics engine.
	 * Note that they cannot be drawn directly.  Rather, you must instantiate
	 * the mesh using the @ref MeshInstance class.
	 */
	class THEBE_API Mesh : public EnginePart
	{
	public:
		Mesh();
		virtual ~Mesh();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;

		VertexBuffer* GetVertexBuffer();
		IndexBuffer* GetIndexBuffer();
		Material* GetMaterial();
		const std::filesystem::path& GetMeshPath() const;

	private:
		Reference<VertexBuffer> vertexBuffer;
		Reference<IndexBuffer> indexBuffer;
		Reference<Material> material;
		std::string name;
		std::filesystem::path vertexBufferPath;
		std::filesystem::path indexBufferPath;
		std::filesystem::path materialPath;
		std::filesystem::path meshPath;
	};
}