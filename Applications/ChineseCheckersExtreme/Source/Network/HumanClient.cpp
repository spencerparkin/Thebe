#include "HumanClient.h"
#include "Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
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

		std::filesystem::path platformMeshPath;
		std::string gameType = this->game->GetGameType();
		if (gameType == "cubic")
			platformMeshPath = "Meshes/CubicPlatform.mesh";
		else if (gameType == "hexagonal")
			platformMeshPath = "Meshes/HexagonalPlatform.mesh";
		else if (gameType == "octagonal")
			platformMeshPath = "Meshes/OctagonalPlatform.mesh";
		else
		{
			THEBE_LOG("Game type \"%s\" not recognized.", gameType);
			return false;
		}

		Reference<Mesh> platformMesh;
		if (!graphicsEngine->LoadEnginePartFromFile(platformMeshPath, platformMesh))
		{
			THEBE_LOG("Failed to load mesh: %s", platformMeshPath.string().c_str());
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

		const std::vector<Thebe::Reference<ChineseCheckersGame::Node>>& nodeArray = this->game->GetNodeArray();
		for (const auto& node : nodeArray)
		{
			Reference<MeshInstance> meshInstance(new MeshInstance());
			meshInstance->SetGraphicsEngine(graphicsEngine);
			meshInstance->SetMesh(platformMesh);
			if (!meshInstance->Setup())
			{
				THEBE_LOG("Failed to setup mesh instance.");
				return false;
			}

			Transform childToParent;
			childToParent.matrix.SetFromAxisAngle(Vector3::XAxis(), M_PI / 2.0);
			childToParent.translation = node->location;
			meshInstance->SetChildToParentTransform(childToParent);
			boardSpace->AddSubSpace(meshInstance);
		}
	}

	return true;
}