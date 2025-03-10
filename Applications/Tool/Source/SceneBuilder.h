#pragma once

#include "App.h"
#include "Builder.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "TextureBuilder.h"
#include <set>

#define SCENE_BUILDER_FLAG_COLLAPSE_TREE			0x00000001
#define SCENE_BUILDER_FLAG_APPLY_MESH_TRANSFORM		0x00000002
#define SCENE_BUILDER_FLAG_TRI_STRIP_MESHES			0x00000004

class SceneBuilder : public Builder
{
public:
	SceneBuilder();
	virtual ~SceneBuilder();

	void SetAssetsFolder(const std::filesystem::path& outputAssetsFolder);
	void SetFlags(uint32_t flags);

	bool BuildScene(const std::filesystem::path& inputSceneFile);

private:
	Thebe::Reference<Thebe::Space> GenerateSceneTree(const aiNode* inputParentNode);
	Thebe::Reference<Thebe::MeshInstance> GenerateMeshInstance(const aiMesh* inputMesh, const aiNode* inputNode);
	Thebe::Reference<Thebe::Mesh> GenerateMesh(const aiMesh* inputMesh, Thebe::MeshInstance* meshInstance);
	Thebe::Reference<Thebe::IndexBuffer> GenerateIndexBuffer(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::VertexBuffer> GenerateVertexBuffer(const aiMesh* inputMesh, const Thebe::Transform& vertexTransform);
	Thebe::Reference<Thebe::Material> GenerateMaterial(const aiMaterial* inputMaterial);

	std::string PrefixWithSceneName(const std::string& givenString);

	std::filesystem::path GenerateMeshPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateMaterialPath(const aiMaterial* inputMaterial);
	std::filesystem::path GenerateIndexBufferPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateVertexBufferPath(const aiMesh* inputMesh);

	uint32_t flags;
	Assimp::Importer importer;
	std::filesystem::path outputAssetsFolder;
	std::filesystem::path inputSceneFileFolder;
	TextureBuilder textureBuilder;
	std::unordered_map<const aiMesh*, Thebe::MeshInstance*> meshMap;
};