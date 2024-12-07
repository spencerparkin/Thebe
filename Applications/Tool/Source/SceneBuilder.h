#pragma once

#include "App.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Vector2.h"
#include "Thebe/Math/Quaternion.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

class SceneBuilder
{
public:
	SceneBuilder();
	virtual ~SceneBuilder();

	void SetAssetsFolder(const std::filesystem::path& outputAssetsFolder);

	bool BuildScene(const std::filesystem::path& inputSceneFile);

private:
	Thebe::Reference<Thebe::Space> GenerateSceneTree(const aiNode* inputParentNode);
	Thebe::Reference<Thebe::MeshInstance> GenerateMeshInstance(const aiMesh* inputMesh);

	Thebe::Transform MakeTransform(const aiMatrix4x4& givenMatrix);
	Thebe::Vector3 MakeVector(const aiVector3D& givenVector);
	Thebe::Vector2 MakeTexCoords(const aiVector3D& givenTexCoords);
	Thebe::Quaternion MakeQuat(const aiQuaternion& quatQuaternion);

	std::string NoSpaces(const std::string& givenString);

	Assimp::Importer importer;
	std::filesystem::path outputAssetsFolder;
};