#include "RBDBuilder.h"
#include "Thebe/EngineParts/RigidBody.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/Log.h"
#include <assimp/postprocess.h>

RBDBuilder::RBDBuilder()
{
	this->stationary = false;
}

/*virtual*/ RBDBuilder::~RBDBuilder()
{
}

void RBDBuilder::SetStationary(bool stationary)
{
	this->stationary = stationary;
}

bool RBDBuilder::BuildRigidBody(const std::filesystem::path& inputSceneFile, const std::filesystem::path& outputAssetsFolder)
{
	THEBE_LOG("Building rigid body!");
	THEBE_LOG("Input file: %s", inputSceneFile.string().c_str());

	wxGetApp().GetGraphicsEngine()->RemoveAllAssetFolders();
	wxGetApp().GetGraphicsEngine()->AddAssetFolder(outputAssetsFolder);

	this->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0);

	THEBE_LOG("Loading scene file: %s", inputSceneFile.string().c_str());
	const aiScene* inputScene = this->importer.ReadFile(inputSceneFile.string().c_str(), aiProcess_GlobalScale);
	if (!inputScene)
	{
		THEBE_LOG("Failed to read scene file!  Error: %s", this->importer.GetErrorString());
		return false;
	}

	const aiMesh* desiredInputMesh = nullptr;
	Thebe::Transform meshToWorld;

	THEBE_LOG("Looking for desired mesh: %s", this->desiredMeshName.C_Str());
	if (!this->FindMeshForRigidBody(inputScene->mRootNode, Thebe::Transform::Identity(), desiredInputMesh, meshToWorld))
	{
		THEBE_LOG("Failed to find mesh \"%s\".", this->desiredMeshName.C_Str());
		return false;
	}

	THEBE_LOG("Found it!  Processing mesh...");
	std::vector<Thebe::Vector3> vertexArray;
	for (unsigned int i = 0; i < desiredInputMesh->mNumVertices; i++)
	{
		Thebe::Vector3 vertex = this->MakeVector(desiredInputMesh->mVertices[i]);
		vertex = meshToWorld.TransformPoint(vertex);
		vertexArray.push_back(vertex);
	}

	Thebe::Reference<Thebe::CollisionObject> collisionObject(new Thebe::CollisionObject());
	collisionObject->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());

	auto convexHull = new Thebe::GJKConvexHull();
	collisionObject->SetShape(convexHull);
	if (!convexHull->hull.GenerateConvexHull(vertexArray))
	{
		THEBE_LOG("Failed to generate convex hull!");
		return false;
	}

	convexHull->hull.SimplifyFaces(true);

	Thebe::Reference<Thebe::RigidBody> rigidBody(new Thebe::RigidBody());
	rigidBody->SetCollisionObject(collisionObject.Get());
	if (!rigidBody->CalculateRigidBodyCharacteristics())	// This can shift the collision object, so do this before dumping the collision object.
	{
		THEBE_LOG("Failed to calculated RBD data.");
		return false;
	}

	rigidBody->SetStationary(this->stationary);

	std::string name = this->NoSpaces(std::string(this->desiredMeshName.C_Str()));

	rigidBody->SetName(name + "_RigidBody");
	collisionObject->SetName(name + "_Collision");

	std::filesystem::path collisionObjectPath = outputAssetsFolder / std::format("CollisionObjects/{}.collision_object", name.c_str());
	if (!wxGetApp().GetGraphicsEngine()->DumpEnginePartToFile(collisionObjectPath, collisionObject, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to dump collision object.");
		return false;
	}

	std::filesystem::path rigidBodyPath = outputAssetsFolder / std::format("PhysicsObjects/{}.rigid_body", name.c_str());
	wxGetApp().GetGraphicsEngine()->GetRelativeToAssetFolder(collisionObjectPath);
	rigidBody->SetCollisionObjectPath(collisionObjectPath);
	if (!wxGetApp().GetGraphicsEngine()->DumpEnginePartToFile(rigidBodyPath, rigidBody, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to dump rigid body object.");
		return false;
	}

	THEBE_LOG("Done!");
	return true;
}

bool RBDBuilder::FindMeshForRigidBody(const aiNode* parentNode, const Thebe::Transform& parentToWorld, const aiMesh*& desiredInputMesh, Thebe::Transform& meshToWorld)
{
	Thebe::Transform nodeToParent = this->MakeTransform(parentNode->mTransformation);
	Thebe::Transform nodeToWorld = parentToWorld * nodeToParent;

	for (unsigned int i = 0; i < parentNode->mNumMeshes; i++)
	{
		const aiMesh* inputMesh = this->importer.GetScene()->mMeshes[parentNode->mMeshes[i]];
		if (inputMesh->mName == this->desiredMeshName)
		{
			meshToWorld = nodeToWorld;
			desiredInputMesh = inputMesh;
			return true;
		}
	}

	for (unsigned int i = 0; i < parentNode->mNumChildren; i++)
	{
		const aiNode* childNode = parentNode->mChildren[i];
		if (this->FindMeshForRigidBody(childNode, nodeToWorld, desiredInputMesh, meshToWorld))
			return true;
	}

	return false;
}

void RBDBuilder::SetDesiredMeshName(const aiString& desiredMeshName)
{
	this->desiredMeshName = desiredMeshName;
}