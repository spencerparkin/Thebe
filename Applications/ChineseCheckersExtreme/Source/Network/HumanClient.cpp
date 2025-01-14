#include "HumanClient.h"
#include "Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/RigidBody.h"
#include "Thebe/EngineParts/MeshInstance.h"

using namespace Thebe;

HumanClient::HumanClient()
{
}

/*virtual*/ HumanClient::~HumanClient()
{
}

/*virtual*/ bool HumanClient::HandleResponse(const ParseParty::JsonValue* jsonResponse)
{
	using namespace ParseParty;

	if (!ChineseCheckersClient::HandleResponse(jsonResponse))
		return false;

	std::string response = ((const JsonString*)((const JsonObject*)jsonResponse)->GetValue("response"))->GetValue();
	if (response == "get_game_state")
	{
		GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
		graphicsEngine->WaitForGPUIdle();

		std::filesystem::path platformMeshPath, ringMeshPath;
		std::filesystem::path platformBodyPath;

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
			return false;
		}

		Reference<Mesh> platformMesh;
		if (!graphicsEngine->LoadEnginePartFromFile(platformMeshPath, platformMesh))
		{
			THEBE_LOG("Failed to load platform mesh: %s", platformMeshPath.string().c_str());
			return false;
		}

		Reference<Mesh> ringMesh;
		if (!graphicsEngine->LoadEnginePartFromFile(ringMeshPath, ringMesh))
		{
			THEBE_LOG("Failed to load ring mesh: %s", ringMeshPath.string().c_str());
			return false;
		}

		auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
		if (!scene)
		{
			THEBE_LOG("Expected render object to be a scene object.");
			return false;
		}

		Space* rootSpace = scene->GetRootSpace();
		if (!rootSpace)
			return false;

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
				return false;
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
					return false;
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
				return false;
			}

			adjustmentTransform.matrix.SetIdentity();
			adjustmentTransform.translation.SetComponents(0.0, 0.0, 0.5);
			platformBody->SetObjectToWorld(objectToWorld * adjustmentTransform);
		}
	}

	return true;
}