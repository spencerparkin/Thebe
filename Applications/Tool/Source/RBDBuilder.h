#pragma once

#include "App.h"
#include "Builder.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

class RBDBuilder : public Builder
{
public:
	RBDBuilder();
	virtual ~RBDBuilder();

	bool BuildRigidBody(const std::filesystem::path& inputSceneFile, const std::filesystem::path& outputAssetsFolder);

	void SetDesiredMeshName(const aiString& desiredMeshName);
	void SetStationary(bool stationary);

private:

	bool FindMeshForRigidBody(const aiNode* parentNode, const Thebe::Transform& parentToWorld, const aiMesh*& desiredInputMesh, Thebe::Transform& meshToWorld);

	Assimp::Importer importer;
	aiString desiredMeshName;
	bool stationary;
};