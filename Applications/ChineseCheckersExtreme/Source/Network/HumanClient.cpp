#include "HumanClient.h"
#include "Application.h"
#include "Frame.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/RigidBody.h"
#include "Thebe/EngineParts/MeshInstance.h"

using namespace Thebe;

HumanClient::HumanClient()
{
	this->animate = false;
	this->connectionProgressDialog = nullptr;
}

/*virtual*/ HumanClient::~HumanClient()
{
	THEBE_ASSERT(this->connectionProgressDialog == nullptr);
}

/*virtual*/ void HumanClient::HandleConnectionStatus(ConnectionStatus status, int i, bool* abort)
{
	switch (status)
	{
		case ConnectionStatus::STARTING_TO_CONNECT:
		{
			this->connectionProgressDialog = new wxProgressDialog(
								wxString::Format("Trying to connect to %s...", this->address.GetAddress().c_str()),
								"Connecting...",
								this->maxConnectionAttempts,
								wxGetApp().GetFrame(),
								wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_SMOOTH | wxPD_AUTO_HIDE);
			this->connectionProgressDialog->Show();
			break;
		}
		case ConnectionStatus::GIVING_UP:
		{
			this->connectionProgressDialog->Update(0, "Failed to connect!");
			break;
		}
		case ConnectionStatus::MAKING_CONNECTION_ATTEMPT:
		{
			this->connectionProgressDialog->Update(i, wxString::Format("Connection attempt #%d...", i + 1));
			if (abort)
				*abort = this->connectionProgressDialog->WasCancelled();
			break;
		}
		case ConnectionStatus::SUCCESSFULLY_CONNECTED:
		{
			this->connectionProgressDialog->Update(this->maxConnectionAttempts, "Connected!");
			break;
		}
		case ConnectionStatus::DONE_TRYING_TO_CONNECT:
		{
			this->connectionProgressDialog->Destroy();
			delete this->connectionProgressDialog;
			this->connectionProgressDialog = nullptr;
			break;
		}
	}
}

/*virtual*/ void HumanClient::ProcessServerMessage(const ParseParty::JsonValue* jsonValue)
{
	using namespace ParseParty;

	ChineseCheckersClient::ProcessServerMessage(jsonValue);

	auto messageValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!messageValue)
		return;

	auto responseValue = dynamic_cast<const JsonString*>(messageValue->GetValue("response"));
	if (!responseValue)
		return;

	std::string response = responseValue->GetValue();
	if (response == "get_game_state")
	{
		GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
		graphicsEngine->WaitForGPUIdle();

		std::filesystem::path platformMeshPath, ringMeshPath;
		std::filesystem::path platformBodyPath;
		std::filesystem::path cubieMeshPath = "Meshes/cubie.mesh";
		std::filesystem::path cubieBodyPath = "PhysicsObjects/cubie.rigid_body";

		std::string gameType = this->game->GetGameType();
		if (gameType == "cubic")
		{
			platformMeshPath = "Meshes/CubicPlatform.mesh";
			platformBodyPath = "PhysicsObjects/CubicPlatform.rigid_body";
			ringMeshPath = "Meshes/CubicRing.mesh";
		}
		else if (gameType == "hexagonal")
		{
			platformMeshPath = "Meshes/HexagonalPlatform.mesh";
			platformBodyPath = "PhysicsObjects/HexagonalPlatform.rigid_body";
			ringMeshPath = "Meshes/HexagonalRing.mesh";
		}
		else if (gameType == "octagonal")
		{
			platformMeshPath = "Meshes/OctagonalPlatform.mesh";
			platformBodyPath = "PhysicsObjects/OctagonalPlatform.rigid_body";
			ringMeshPath = "Meshes/OctagonalRing.mesh";
		}
		else
		{
			THEBE_LOG("Game type \"%s\" not recognized.", gameType);
			return;
		}

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
			rootSpace->AddSubSpace(boardSpace);
		}

		boardSpace->ClearAllSubSpaces();

		Transform adjustmentTransform;

		const std::vector<Thebe::Reference<ChineseCheckersGame::Node>>& nodeArray = this->game->GetNodeArray();
		for (const auto& node : nodeArray)
		{
			Reference<MeshInstance> platformMeshInstance(new MeshInstance());
			platformMeshInstance->SetGraphicsEngine(graphicsEngine);
			platformMeshInstance->SetMesh(platformMesh);
			if (!platformMeshInstance->Setup())
			{
				THEBE_LOG("Failed to setup platform mesh instance.");
				return;
			}

			Transform objectToWorld;
			objectToWorld.matrix.SetFromAxisAngle(Vector3::XAxis(), -M_PI / 2.0);
			objectToWorld.translation = node->location;
			platformMeshInstance->SetChildToParentTransform(objectToWorld);
			boardSpace->AddSubSpace(platformMeshInstance);

			Vector3 zoneColor;
			if (game->GetZoneColor(node->zoneID, zoneColor))
			{
				Vector4 zoneColorWithAlpha(zoneColor.x, zoneColor.y, zoneColor.z, 0.5);
				
				Reference<MeshInstance> ringMeshInstance(new MeshInstance());
				ringMeshInstance->SetGraphicsEngine(graphicsEngine);
				ringMeshInstance->SetMesh(ringMesh);
				ringMeshInstance->SetColor(zoneColorWithAlpha);
				if (!ringMeshInstance->Setup())
				{
					THEBE_LOG("Failed to setup ring mesh instance.");
					return;
				}

				adjustmentTransform.matrix.SetIdentity();
				adjustmentTransform.translation.SetComponents(0.0, 0.0, 1.0);
				ringMeshInstance->SetChildToParentTransform(objectToWorld * adjustmentTransform);
				boardSpace->AddSubSpace(ringMeshInstance);
			}

			Reference<RigidBody> platformBody;
			if (!graphicsEngine->LoadEnginePartFromFile(platformBodyPath, platformBody, THEBE_LOAD_FLAG_DONT_CACHE_PART | THEBE_LOAD_FLAG_DONT_CHECK_CACHE))
			{
				THEBE_LOG("Failed to load platform body: %s", platformBodyPath.string().c_str());
				return;
			}

			adjustmentTransform.matrix.SetIdentity();
			adjustmentTransform.translation.SetComponents(0.0, 0.0, 0.5);
			platformBody->SetObjectToWorld(objectToWorld * adjustmentTransform);
			platformBody->GetCollisionObject()->SetUserData(node->GetHandle());

			if (node->occupant && game->GetZoneColor(node->occupant->sourceZoneID, zoneColor))
			{
				Vector4 zoneColorNoAlpha(zoneColor.x, zoneColor.y, zoneColor.z, 1.0);

				Reference<MeshInstance> cubieMeshInstance(new MeshInstance());
				cubieMeshInstance->SetGraphicsEngine(graphicsEngine);
				cubieMeshInstance->SetMesh(cubieMesh);
				cubieMeshInstance->SetColor(zoneColorNoAlpha);
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

				adjustmentTransform.translation.SetComponents(0.0, 0.0, -1.5);
				cubieBody->GetCollisionObject()->SetTargetSpace(cubieMeshInstance, adjustmentTransform);

				node->occupant->collisionObjectHandle = cubieBody->GetCollisionObject()->GetHandle();
			}
		}

		this->SnapCubiesIntoPosition();
	}
	else if (response == "apply_turn")
	{
		if(!this->animate)
			this->SnapCubiesIntoPosition();
		else
		{
			// TODO: Write this.
		}
	}

	return;
}

void HumanClient::TakeTurn(const std::vector<ChineseCheckersGame::Node*>& nodeArray)
{
	if (!this->game.Get())
		return;

	std::vector<int> nodeOffsetArray;
	this->game->NodeArrayToOffsetArray(nodeArray, nodeOffsetArray);

	using namespace ParseParty;

	std::unique_ptr<JsonObject> requestValue(new JsonObject());
	requestValue->SetValue("request", new JsonString("take_turn"));
	
	auto nodeOffsetArrayValue = new JsonArray();
	requestValue->SetValue("node_offset_array", nodeOffsetArrayValue);
	for (int i : nodeOffsetArray)
		nodeOffsetArrayValue->PushValue(new JsonInt(i));

	this->SendJson(requestValue.get());
}

void HumanClient::SnapCubiesIntoPosition()
{
	// This function can be used to update the visual representation of the board
	// to match the game state.  Ultimately, however, the goal is to animate the
	// game state changes, but this will get us by until then.

	if (!this->game.Get())
		return;

	const std::vector<Thebe::Reference<ChineseCheckersGame::Node>>& nodeArray = this->game->GetNodeArray();
	for (const auto& node : nodeArray)
	{
		if (!node->occupant)
			continue;

		Reference<CollisionObject> collisionObject;
		if (!HandleManager::Get()->GetObjectFromHandle(node->occupant->collisionObjectHandle, collisionObject))
			continue;

		Transform adjustmentTransform;
		adjustmentTransform.matrix.SetIdentity();
		adjustmentTransform.translation.SetComponents(0.0, 2.5, 0.0);

		Transform objectToWorld;
		objectToWorld.SetIdentity();
		objectToWorld.translation = node->location;

		collisionObject->SetObjectToWorld(objectToWorld * adjustmentTransform);
	}
}