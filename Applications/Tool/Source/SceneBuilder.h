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

class SceneBuilder : public Builder
{
public:
	SceneBuilder();
	virtual ~SceneBuilder();

	void SetAssetsFolder(const std::filesystem::path& outputAssetsFolder);

	bool BuildScene(const std::filesystem::path& inputSceneFile);

private:
	Thebe::Reference<Thebe::Space> GenerateSceneTree(const aiNode* inputParentNode);
	Thebe::Reference<Thebe::MeshInstance> GenerateMeshInstance(const aiMesh* inputMesh, const aiNode* inputNode);
	Thebe::Reference<Thebe::Mesh> GenerateMesh(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::IndexBuffer> GenerateIndexBuffer(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::VertexBuffer> GenerateVertexBuffer(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::Material> GenerateMaterial(const aiMaterial* inputMaterial);

	std::string PrefixWithSceneName(const std::string& givenString);

	std::filesystem::path GenerateMeshPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateMaterialPath(const aiMaterial* inputMaterial);
	std::filesystem::path GenerateIndexBufferPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateVertexBufferPath(const aiMesh* inputMesh);

	Assimp::Importer importer;
	std::filesystem::path outputAssetsFolder;
	std::filesystem::path inputSceneFileFolder;
	TextureBuilder textureBuilder;
};