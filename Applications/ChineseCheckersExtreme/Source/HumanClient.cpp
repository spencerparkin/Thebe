#include "Application.h"
#include "HumanClient.h"
#include "Factory.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/RigidBody.h"

using namespace Thebe;

HumanClient::HumanClient()
{
}

/*virtual*/ HumanClient::~HumanClient()
{
}

/*virtual*/ void HumanClient::ProcessServerMessage(const ParseParty::JsonValue* jsonValue)
{
	ChineseCheckersGameClient::ProcessServerMessage(jsonValue);

	using namespace ParseParty;

	auto responseValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!responseValue)
		return;

	auto responseTypeValue = dynamic_cast<const JsonString*>(responseValue->GetValue("response"));
	if (!responseTypeValue)
		return;

	std::string response = responseTypeValue->GetValue();

	if (response == "get_graph")
	{
		this->RegenerateScene();
	}
	else if (response == "make_move")
	{
		// TODO: Generate animation and queue it up for playing.  Note that
		//       it can be interrupted at any time by the user, and this has
		//       no bearing on an actual update of the internal game state.

		this->SnapCubiesIntoPosition();
	}
}

void HumanClient::RegenerateScene()
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->WaitForGPUIdle();

	std::filesystem::path platformMeshPath, ringMeshPath;
	std::filesystem::path platformBodyPath;
	std::filesystem::path cubieMeshPath = "Meshes/cubie.mesh";
	std::filesystem::path cubieBodyPath = "PhysicsObjects/cubie.rigid_body";

	//platformMeshPath = "Meshes/CubicPlatform.mesh";
	//platformBodyPath = "PhysicsObjects/CubicPlatform.rigid_body";
	//ringMeshPath = "Meshes/CubicRing.mesh";
	
	platformMeshPath = "Meshes/HexagonalPlatform.mesh";
	platformBodyPath = "PhysicsObjects/HexagonalPlatform.rigid_body";
	ringMeshPath = "Meshes/HexagonalRing.mesh";
	
	//platformMeshPath = "Meshes/OctagonalPlatform.mesh";
	//platformBodyPath = "PhysicsObjects/OctagonalPlatform.rigid_body";
	//ringMeshPath = "Meshes/OctagonalRing.mesh";

	Reference<Mesh> platformMesh;
	if (!graphicsEngine->LoadEnginePartFromFile(platformMeshPath, platformMesh))
	{
		THEBE_LOG("Failed to load platform mesh: %s", platformMeshPath.string().c_str());
		return;
	}

	Reference<Mesh> ringMesh;
	if (!graphicsEngine->LoadEnginePartFromFile(ringMeshPath, ringMesh))
	{
		THEBE_LOG("Failed to load ring mesh: %s", ringMeshPath.string().c_str());
		return;
	}

	Reference<Mesh> cubieMesh;
	if (!graphicsEngine->LoadEnginePartFromFile(cubieMeshPath, cubieMesh))
	{
		THEBE_LOG("Failed to load cubie mesh: %s", cubieMeshPath.string().c_str());
		return;
	}

	auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
	if (!scene)
	{
		THEBE_LOG("Expected render object to be a scene object.");
		return;
	}

	Space* rootSpace = scene->GetRootSpace();
	if (!rootSpace)
		return;

	Reference<Space> boardSpace(rootSpace->FindSpaceByName("board"));
	if (!boardSpace.Get())
	{
		boardSpace.Set(new Space());
		boardSpace->SetName("board");
		rootSpace->AddSubSpace(boardSpace);
	}

	boardSpace->ClearAllSubSpaces();

	for (int i = 0; i < (int)this->graph->GetNodeArray().size(); i++)
	{
		ChineseCheckers::Node* nativeNode = this->graph->GetNodeArray()[i];

		Node* node = dynamic_cast<Node*>(nativeNode);
		if (!node)
		{
			THEBE_LOG("Factory not working right.");
			return;
		}

		Reference<MeshInstance> platformMeshInstance(new MeshInstance());
		platformMeshInstance->SetGraphicsEngine(graphicsEngine);
		platformMeshInstance->SetMesh(platformMesh);
		if (!platformMeshInstance->Setup())
		{
			THEBE_LOG("Failed to setup platform mesh instance.");
			return;
		}

		Transform objectToWorld;
		objectToWorld.translation = node->GetLocation3D();
		platformMeshInstance->SetChildToParentTransform(objectToWorld);
		boardSpace->AddSubSpace(platformMeshInstance);

		if(node->GetColor() != ChineseCheckers::Marble::Color::NONE)
		{
			Vector4 zoneColor = this->MarbleColor(node->GetColor(), 0.5);
		
			Reference<MeshInstance> ringMeshInstance(new MeshInstance());
			ringMeshInstance->SetGraphicsEngine(graphicsEngine);
			ringMeshInstance->SetMesh(ringMesh);
			ringMeshInstance->SetColor(zoneColor);
			if (!ringMeshInstance->Setup())
			{
				THEBE_LOG("Failed to setup ring mesh instance.");
				return;
			}

			ringMeshInstance->SetChildToParentTransform(objectToWorld);
			boardSpace->AddSubSpace(ringMeshInstance);
		}

		Reference<RigidBody> platformBody;
		if (!graphicsEngine->LoadEnginePartFromFile(platformBodyPath, platformBody, THEBE_LOAD_FLAG_DONT_CACHE_PART | THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
		{
			THEBE_LOG("Failed to load platform body: %s", platformBodyPath.string().c_str());
			return;
		}

		platformBody->SetObjectToWorld(objectToWorld);
		platformBody->GetCollisionObject()->SetUserData(i);

		ChineseCheckers::Marble* nativeMarble = node->GetOccupant();
		if (nativeMarble)
		{
			Vector4 marbleColor = this->MarbleColor(nativeMarble->GetColor(), 1.0);

			Reference<MeshInstance> cubieMeshInstance(new MeshInstance());
			cubieMeshInstance->SetGraphicsEngine(graphicsEngine);
			cubieMeshInstance->SetMesh(cubieMesh);
			cubieMeshInstance->SetColor(marbleColor);
			if (!cubieMeshInstance->Setup())
			{
				THEBE_LOG("Failed to setup cubie mesh instance.");
				return;
			}

			boardSpace->AddSubSpace(cubieMeshInstance);

			Reference<RigidBody> cubieBody;
			if (!graphicsEngine->LoadEnginePartFromFile(cubieBodyPath, cubieBody, THEBE_LOAD_FLAG_DONT_CACHE_PART | THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
			{
				THEBE_LOG("Failed to load cubie body.");
				return;
			}

			cubieBody->GetCollisionObject()->SetTargetSpace(cubieMeshInstance, Transform::Identity());

			Marble* marble = dynamic_cast<Marble*>(nativeMarble);
			if (!marble)
			{
				THEBE_LOG("Factory not working to create proper marbles.");
				return;
			}

			marble->collisionObjectHandle = cubieBody->GetCollisionObject()->GetHandle();
		}
	}

	this->SnapCubiesIntoPosition();
}

void HumanClient::SnapCubiesIntoPosition()
{
	if (!this->graph)
		return;

	const std::vector<ChineseCheckers::Node*>& nodeArray = this->graph->GetNodeArray();
	for (const auto& nativeNode : nodeArray)
	{
		const ChineseCheckers::Marble* nativeMarble = nativeNode->GetOccupant();
		if (!nativeMarble)
			continue;

		auto marble = dynamic_cast<const Marble*>(nativeMarble);
		if (!marble)
			continue;

		auto node = dynamic_cast<const Node*>(nativeNode);
		if (!node)
			continue;

		Reference<CollisionObject> collisionObject;
		if (!HandleManager::Get()->GetObjectFromHandle(marble->collisionObjectHandle, collisionObject))
			continue;

		Transform objectToWorld;
		objectToWorld.SetIdentity();
		objectToWorld.translation = node->GetLocation3D() + Vector3(0.0, 2.5, 0.0);

		collisionObject->SetObjectToWorld(objectToWorld);

		Reference<PhysicsObject> physicsObject;
		RefHandle handle = (RefHandle)collisionObject->GetPhysicsData();
		if (HandleManager::Get()->GetObjectFromHandle(handle, physicsObject))
			physicsObject->SetFrozen(true);
	}
}

/*static*/ Thebe::Vector4 HumanClient::MarbleColor(ChineseCheckers::Marble::Color color, double alpha)
{
	switch (color)
	{
	case ChineseCheckers::Marble::Color::BLACK:
		return Vector4(0.0, 0.0, 0.0, alpha);
	case ChineseCheckers::Marble::Color::WHITE:
		return Vector4(1.0, 1.0, 1.0, alpha);
	case ChineseCheckers::Marble::Color::RED:
		return Vector4(1.0, 0.0, 0.0, alpha);
	case ChineseCheckers::Marble::Color::GREEN:
		return Vector4(0.0, 1.0, 0.0, alpha);
	case ChineseCheckers::Marble::Color::BLUE:
		return Vector4(0.0, 0.0, 1.0, alpha);
	case ChineseCheckers::Marble::Color::CYAN:
		return Vector4(0.0, 1.0, 1.0, alpha);
	case ChineseCheckers::Marble::Color::MAGENTA:
		return Vector4(1.0, 0.0, 1.0, alpha);
	case ChineseCheckers::Marble::Color::ORANGE:
		return Vector4(1.0, 0.5, 0.0, alpha);
	case ChineseCheckers::Marble::Color::YELLOW:
		return Vector4(1.0, 1.0, 0.0, alpha);
	}

	return Vector4(0.5, 0.5, 0.5, alpha);
}