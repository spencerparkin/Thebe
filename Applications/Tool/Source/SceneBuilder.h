#pragma once

#include "App.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Vector2.h"
#include "Thebe/Math/Quaternion.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <set>

class SceneBuilder
{
public:
	SceneBuilder();
	virtual ~SceneBuilder();

	void SetAssetsFolder(const std::filesystem::path& outputAssetsFolder);

	bool BuildScene(const std::filesystem::path& inputSceneFile);

private:
	struct TextureBuildInfo
	{
		UINT numComponentsPerPixel;
	};

	Thebe::Reference<Thebe::Space> GenerateSceneTree(const aiNode* inputParentNode);
	Thebe::Reference<Thebe::MeshInstance> GenerateMeshInstance(const aiMesh* inputMesh, const aiNode* inputNode);
	Thebe::Reference<Thebe::Mesh> GenerateMesh(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::IndexBuffer> GenerateIndexBuffer(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::VertexBuffer> GenerateVertexBuffer(const aiMesh* inputMesh);
	Thebe::Reference<Thebe::Material> GenerateMaterial(const aiMaterial* inputMaterial);
	Thebe::Reference<Thebe::TextureBuffer> GenerateTextureBuffer(const std::filesystem::path& inputTexturePath, const TextureBuildInfo& buildInfo);

	Thebe::Transform MakeTransform(const aiMatrix4x4& givenMatrix);
	Thebe::Vector3 MakeVector(const aiVector3D& givenVector);
	Thebe::Vector2 MakeTexCoords(const aiVector3D& givenTexCoords);
	Thebe::Quaternion MakeQuat(const aiQuaternion& quatQuaternion);

	std::string NoSpaces(const std::string& givenString);
	std::string PrefixWithSceneName(const std::string& givenString);

	std::filesystem::path GenerateMeshPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateMaterialPath(const aiMaterial* inputMaterial);
	std::filesystem::path GenerateIndexBufferPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateVertexBufferPath(const aiMesh* inputMesh);
	std::filesystem::path GenerateTextureBufferPath(const std::filesystem::path& inputTexturePath);

	Assimp::Importer importer;
	std::filesystem::path outputAssetsFolder;
	std::filesystem::path inputSceneFileFolder;
	std::map<std::filesystem::path, TextureBuildInfo> texturesToBuildMap;
	bool compressTextures;
};