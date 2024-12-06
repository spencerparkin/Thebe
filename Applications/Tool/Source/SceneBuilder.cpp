#include "SceneBuilder.h"
#include "Thebe/Log.h"
#include "Thebe/EngineParts/Space.h"
#include <assimp/postprocess.h>

SceneBuilder::SceneBuilder()
{
}

/*virtual*/ SceneBuilder::~SceneBuilder()
{
}

bool SceneBuilder::BuildScene(const std::filesystem::path& inputSceneFile, const std::filesystem::path& outputAssetsFolder)
{
	THEBE_LOG("Building scene!");
	THEBE_LOG("Input file: %s", inputSceneFile.string().c_str());
	THEBE_LOG("Output assets folder: %s", outputAssetsFolder.string().c_str());

	this->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0);

	const aiScene* inputScene = importer.ReadFile(inputSceneFile.string().c_str(), aiProcess_GlobalScale);
	if (!inputScene)
	{
		THEBE_LOG("Failed to read scene file!  Error: %s", this->importer.GetErrorString());
		return false;
	}

	Thebe::Reference<Thebe::Space> outputRootNode = this->GenerateSceneTree(inputScene->mRootNode);
	Thebe::Reference<Thebe::Scene> outputScene(new Thebe::Scene());
	outputScene->SetName(inputScene->mName.C_Str());
	outputScene->SetRootSpace(outputRootNode);

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path outputSceneFile = outputAssetsFolder / "Scenes" / inputSceneFile.filename();
	outputSceneFile.replace_extension(".scene");
	if (!graphicsEngine->DumpEnginePartToFile(outputSceneFile, outputScene, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to write scene file!");
		return false;
	}

	return true;
}

Thebe::Reference<Thebe::Space> SceneBuilder::GenerateSceneTree(const aiNode* inputParentNode)
{
	Thebe::Reference<Thebe::Space> outputParentNode(new Thebe::Space());
	outputParentNode->SetName(inputParentNode->mName.C_Str());
	outputParentNode->SetChildToParentTransform(this->MakeTransform(inputParentNode->mTransformation));

	for (int i = 0; i < (int)inputParentNode->mNumChildren; i++)
	{
		const aiNode* inputChildNode = inputParentNode->mChildren[i];
		Thebe::Reference<Thebe::Space> outputChildNode = this->GenerateSceneTree(inputChildNode);
		outputParentNode->AddSubSpace(outputChildNode);
	}

	for (int i = 0; i < (int)inputParentNode->mNumMeshes; i++)
	{
		const aiMesh* inputMesh = this->importer.GetScene()->mMeshes[inputParentNode->mMeshes[i]];

		// TODO: Process the mesh here.
	}

	return outputParentNode;
}

Thebe::Transform SceneBuilder::MakeTransform(const aiMatrix4x4& givenMatrix)
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

Thebe::Vector3 SceneBuilder::MakeVector(const aiVector3D& givenVector)
{
	Thebe::Vector3 vec;
	vec.SetComponents(givenVector.x, givenVector.y, givenVector.z);
	return vec;
}

Thebe::Vector2 SceneBuilder::MakeTexCoords(const aiVector3D& givenTexCoords)
{
	Thebe::Vector2 vec;
	vec.SetComponents(givenTexCoords.x, givenTexCoords.y);
	return vec;
}

Thebe::Quaternion SceneBuilder::MakeQuat(const aiQuaternion& givenQuaternion)
{
	Thebe::Quaternion quat;
	quat.w = givenQuaternion.w;
	quat.x = givenQuaternion.x;
	quat.y = givenQuaternion.y;
	quat.z = givenQuaternion.z;
	return quat;
}