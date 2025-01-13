#pragma once

#include <string>
#include "Thebe/Math/Transform.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Vector2.h"
#include "Thebe/Math/Quaternion.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

class Builder
{
public:
	Builder();
	virtual ~Builder();

	std::string NoSpaces(const std::string& givenString);

	Thebe::Transform MakeTransform(const aiMatrix4x4& givenMatrix);
	Thebe::Vector3 MakeVector(const aiVector3D& givenVector);
	Thebe::Vector2 MakeTexCoords(const aiVector3D& givenTexCoords);
	Thebe::Quaternion MakeQuat(const aiQuaternion& quatQuaternion);
};