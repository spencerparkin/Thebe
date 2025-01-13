#include "Builder.h"

Builder::Builder()
{
}

/*virtual*/ Builder::~Builder()
{
}

std::string Builder::NoSpaces(const std::string& givenString)
{
	std::string resultString = givenString;

	while (true)
	{
		size_t pos = resultString.find(' ');
		if (pos == std::string::npos)
			break;

		resultString.replace(pos, 1, "_");
	}

	return resultString;
}

Thebe::Transform Builder::MakeTransform(const aiMatrix4x4& givenMatrix)
{
	THEBE_ASSERT(givenMatrix.d1 == 0.0 && givenMatrix.d2 == 0.0 && givenMatrix.d3 == 0.0 && givenMatrix.d4 == 1.0);

	Thebe::Transform xform;

	xform.matrix.ele[0][0] = givenMatrix.a1;
	xform.matrix.ele[0][1] = givenMatrix.a2;
	xform.matrix.ele[0][2] = givenMatrix.a3;

	xform.matrix.ele[1][0] = givenMatrix.b1;
	xform.matrix.ele[1][1] = givenMatrix.b2;
	xform.matrix.ele[1][2] = givenMatrix.b3;

	xform.matrix.ele[2][0] = givenMatrix.c1;
	xform.matrix.ele[2][1] = givenMatrix.c2;
	xform.matrix.ele[2][2] = givenMatrix.c3;

	xform.translation.x = givenMatrix.a4;
	xform.translation.y = givenMatrix.b4;
	xform.translation.z = givenMatrix.c4;

	return xform;
}

Thebe::Vector3 Builder::MakeVector(const aiVector3D& givenVector)
{
	Thebe::Vector3 vec;
	vec.SetComponents(givenVector.x, givenVector.y, givenVector.z);
	return vec;
}

Thebe::Vector2 Builder::MakeTexCoords(const aiVector3D& givenTexCoords)
{
	Thebe::Vector2 vec;
	vec.SetComponents(givenTexCoords.x, givenTexCoords.y);
	return vec;
}

Thebe::Quaternion Builder::MakeQuat(const aiQuaternion& givenQuaternion)
{
	Thebe::Quaternion quat;
	quat.w = givenQuaternion.w;
	quat.x = givenQuaternion.x;
	quat.y = givenQuaternion.y;
	quat.z = givenQuaternion.z;
	return quat;
}