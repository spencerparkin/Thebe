#include "Application.h"
#include "HumanClient.h"
#include "Factory.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/RigidBody.h"
#include "LifeText.h"
#include "BoardCam.h"
#include "Frame.h"
#include <wx/msgdlg.h>

using namespace Thebe;

HumanClient::HumanClient()
{
	this->connectionProgressDialog = nullptr;
	this->audioEventHandlerID = 0;
}

/*virtual*/ HumanClient::~HumanClient()
{
}

/*virtual*/ bool HumanClient::Setup()
{
	if (!ChineseCheckersGameClient::Setup())
		return false;

	this->random.SetSeed(::time(nullptr));

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();

	CameraSystem* cameraSystem = graphicsEngine->GetCameraSystem();
	Reference<BoardCam> boardCam(new BoardCam());
	cameraSystem->AddController("board_cam", boardCam);
	cameraSystem->SetActiveController("board_cam");

	return true;
}

/*virtual*/ void HumanClient::Shutdown()
{
	ChineseCheckersGameClient::Shutdown();

	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->WaitForGPUIdle();

	graphicsEngine->GetCameraSystem()->RemoveController("board_cam");

	auto scene = dynamic_cast<Scene*>(graphicsEngine->GetRenderObject());
	if (scene)
	{
		Space* rootSpace = scene->GetRootSpace();
		if (rootSpace)
		{
			Space* parentSpace = nullptr;
			Reference<Space> boardSpace(rootSpace->FindSpaceByName("board", &parentSpace));
			if (boardSpace)
			{
				boardSpace->Shutdown();
				parentSpace->RemoveSubSpace(boardSpace);
			}
		}
	}

	ChineseCheckersFrame* frame = wxGetApp().GetFrame();
	if (frame)
	{
		frame->SetStatusText("");
		frame->SetInfoText("");
	}

	graphicsEngine->GetEventSystem()->UnregisterEventHandler(this->audioEventHandlerID);

	AudioSystem* audioSystem = graphicsEngine->GetAudioSystem();
	audioSystem->ClearMidiSongQueue();
	audioSystem->SkipCurrentSong();

	graphicsEngine->GetPhysicsSystem()->UntrackAllObjects();
	graphicsEngine->GetCollisionSystem()->UntrackAllObjects();
}

/*virtual*/ void HumanClient::Update(double deltaTimeSeconds)
{
	ChineseCheckersGameClient::Update(deltaTimeSeconds);

	this->animationProcessor.Animate(deltaTimeSeconds, this->graph.get());
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

	auto responseValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!responseValue)
		return;

	auto responseTypeValue = dynamic_cast<const JsonString*>(responseValue->GetValue("response"));
	if (!responseTypeValue)
		return;

	std::string response = responseTypeValue->GetValue();

	if (response == "make_move")
	{
		ChineseCheckers::MoveSequence moveSequence;
		if (!moveSequence.FromJson(responseValue->GetValue("move_sequence")))
			return;

		this->animationProcessor.EnqueueAnimationForMoveSequence(moveSequence, this->graph.get());

		auto winnerValue = dynamic_cast<const JsonInt*>(responseValue->GetValue("winner"));
		if (winnerValue)
		{
			auto winner = (ChineseCheckers::Marble::Color)winnerValue->GetValue();
			if (winner != ChineseCheckers::Marble::Color::NONE)
			{
				wxMessageBox("Color " + this->MarbleText(winner) + " wins!", "Game Over!", wxICON_INFORMATION | wxOK, wxGetApp().GetFrame());
			}
		}
	}

	ChineseCheckersGameClient::ProcessServerMessage(jsonValue);

	if (response == "get_graph")
	{
		auto gameTypeValue = dynamic_cast<const JsonString*>(responseValue->GetValue("game_type"));
		if (!gameTypeValue)
			return;

		std::string gameType = gameTypeValue->GetValue();
		this->RegenerateScene(gameType);

		this->QueueUpSongs();

		GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
		//graphicsEngine->GetAudioSystem()->ChangeMidiVolume(0.7);

		this->audioEventHandlerID = graphicsEngine->GetEventSystem()->RegisterEventHandler("Audio", [=](const Event* event)
			{
				this->HandleAudioEvent(dynamic_cast<const AudioEvent*>(event));
			});
	}
	
	wxGetApp().GetFrame()->SetStatusText("You are color " + this->MarbleText(this->GetColor()) + ".");
	wxGetApp().GetFrame()->SetInfoText("Waiting for color " + this->MarbleText(this->whoseTurn) + " to take their turn.");
}

void HumanClient::HandleAudioEvent(const Thebe::AudioEvent* audioEvent)
{
	if (audioEvent->type == AudioEvent::SONG_QUEUE_EMPTY)
	{
		this->QueueUpSongs();
	}
}

void HumanClient::QueueUpSongs()
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();

	const char* songArray[] =
	{
		"AllOfMe",
		"Thriller",
		"OneRingToRuleThemAll",
		"GollumsSong",
		"DontStopTill",
		"ConcerningHobbits",
		"AustinPowers-TheSpyWhoShaggedMe",
		"AustinPowers-SecretAgentMan"
	};

	int songArraySize = sizeof(songArray) / sizeof(const char*);

	this->random.Shuffle(songArray, songArraySize);

	for (int i = 0; i < songArraySize; i++)
		graphicsEngine->GetAudioSystem()->EnqueueMidiSong(songArray[i]);
}

void HumanClient::RegenerateScene(const std::string& gameType)
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->WaitForGPUIdle();

	std::filesystem::path platformMeshPath, ringMeshPath;
	std::filesystem::path platformBodyPath;
	std::filesystem::path cubieMeshPath = "Meshes/cubie.mesh";
	std::filesystem::path cubieBodyPath = "PhysicsObjects/cubie.rigid_body";

	if (gameType == "traditional")
	{
		platformMeshPath = "Meshes/HexagonalPlatform.mesh";
		platformBodyPath = "PhysicsObjects/HexagonalPlatform.rigid_body";
		ringMeshPath = "Meshes/HexagonalRing.mesh";
	}
	else if (gameType == "cubic")
	{
		platformMeshPath = "Meshes/CubicPlatform.mesh";
		platformBodyPath = "PhysicsObjects/CubicPlatform.rigid_body";
		ringMeshPath = "Meshes/CubicRing.mesh";
	}
	else if (gameType == "cubic/donut")
	{
		platformMeshPath = "Meshes/OctagonalPlatform.mesh";
		platformBodyPath = "PhysicsObjects/OctagonalPlatform.rigid_body";
		ringMeshPath = "Meshes/OctagonalRing.mesh";
	}
	else
	{
		THEBE_LOG("Did not recognize game type: %s", gameType.c_str());
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
		platformMeshInstance->SetName("platformMeshInstance");
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
			ringMeshInstance->SetName("ringMeshInstance");
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
		platformBody->GetCollisionObject()->SetUserData(uintptr_t(nativeNode));		// Note that care must be taken to prevent this from going stale.

		std::shared_ptr<ChineseCheckers::Marble> nativeMarble = node->GetOccupant();
		if (nativeMarble)
		{
			Vector4 marbleColor = this->MarbleColor(nativeMarble->GetColor(), 1.0);

			Reference<MeshInstance> cubieMeshInstance(new MeshInstance());
			cubieMeshInstance->SetGraphicsEngine(graphicsEngine);
			cubieMeshInstance->SetMesh(cubieMesh);
			cubieMeshInstance->SetColor(marbleColor);
			cubieMeshInstance->SetName("cubieMeshInstance");
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

			Marble* marble = dynamic_cast<Marble*>(nativeMarble.get());
			if (!marble)
			{
				THEBE_LOG("Factory not working to create proper marbles.");
				return;
			}

			marble->collisionObjectHandle = cubieBody->GetCollisionObject()->GetHandle();

			Transform lifeTextTransform;
			lifeTextTransform.SetIdentity();
			lifeTextTransform.matrix.SetUniformScale(100.0);
			lifeTextTransform.translation.SetComponents(0.0, 2.0, 0.0);

			Reference<LifeText> lifeText(new LifeText());
			lifeText->SetGraphicsEngine(graphicsEngine);
			lifeText->SetChildToParentTransform(lifeTextTransform);
			lifeText->SetTextColor(Vector3(0.0, 0.0, 0.0));
			lifeText->SetFlags(0);
			lifeText->SetName("lifeText");
			lifeText->marbleWeakRef = nativeMarble;
			if (!lifeText->Setup())
			{
				THEBE_LOG("Failed to setup life text for marble.");
				return;
			}

			cubieMeshInstance->AddSubSpace(lifeText);
		}
	}

	this->animationProcessor.SnapAllMarblesToPosition(this->graph.get());
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

/*static*/ wxString HumanClient::MarbleText(ChineseCheckers::Marble::Color color)
{
	switch (color)
	{
	case ChineseCheckers::Marble::Color::BLACK:
		return "Black";
	case ChineseCheckers::Marble::Color::WHITE:
		return "White";
	case ChineseCheckers::Marble::Color::RED:
		return "Red";
	case ChineseCheckers::Marble::Color::GREEN:
		return "Green";
	case ChineseCheckers::Marble::Color::BLUE:
		return "Blue";
	case ChineseCheckers::Marble::Color::CYAN:
		return "Cyan";
	case ChineseCheckers::Marble::Color::MAGENTA:
		return "Magenta";
	case ChineseCheckers::Marble::Color::ORANGE:
		return "Orange";
	case ChineseCheckers::Marble::Color::YELLOW:
		return "Yellow";
	}

	return "?";
}