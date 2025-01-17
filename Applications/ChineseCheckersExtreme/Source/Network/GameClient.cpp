#include "GameClient.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------------- ChineseCheckersClient -------------------------------------

ChineseCheckersClient::ChineseCheckersClient()
{
	this->sourceZoneID = 0;
	this->whoseTurnZoneID = 0;
}

/*virtual*/ ChineseCheckersClient::~ChineseCheckersClient()
{
	this->game = nullptr;
}

ChineseCheckersGame* ChineseCheckersClient::GetGame()
{
	return this->game;
}

/*virtual*/ bool ChineseCheckersClient::Setup()
{
	using namespace ParseParty;

	if (!JsonClient::Setup())
		return false;

	std::unique_ptr<JsonObject> jsonRequestValue(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_game_state"));
	this->SendJson(jsonRequestValue.get());

	jsonRequestValue.reset(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_source_zone_id"));
	this->SendJson(jsonRequestValue.get());

	return true;
}

/*virtual*/ void ChineseCheckersClient::Shutdown()
{
	JsonClient::Shutdown();
}

/*virtual*/ void ChineseCheckersClient::Update()
{
	JsonClient::Update();

	// This is where we might continuously ping the server if WinSock keeps failing to flush the sockets.
}

/*virtual*/ void ChineseCheckersClient::ProcessServerMessage(const ParseParty::JsonValue* jsonResponse)
{
	using namespace ParseParty;

	auto responseRootValue = dynamic_cast<const JsonObject*>(jsonResponse);
	if (!responseRootValue)
	{
		THEBE_LOG("Json response was not an object.");
		return;
	}

	auto responseValue = dynamic_cast<const JsonString*>(responseRootValue->GetValue("response"));
	if (!responseValue)
	{
		THEBE_LOG("No response value in JSON response object.");
		return;
	}

	std::string response = responseValue->GetValue();
	THEBE_LOG("Client got response \"%s\" from server.", response.c_str());

	if (response == "pong")
	{
		THEBE_LOG("PONG!");
	}
	else if (response == "turn_notify")
	{
		auto whoseTurnZoneIDValue = dynamic_cast<const JsonInt*>(responseRootValue->GetValue("whose_turn_zone_id"));
		if (!whoseTurnZoneIDValue)
		{
			THEBE_LOG("No zone ID found in turn notify response.");
			return;
		}

		this->whoseTurnZoneID = (int)whoseTurnZoneIDValue->GetValue();
	}
	else if (response == "get_game_state")
	{
		auto gameTypeValue = dynamic_cast<const JsonString*>(responseRootValue->GetValue("game_type"));
		if (!gameTypeValue)
		{
			THEBE_LOG("Game type not found in response.");
			return;
		}

		std::string gameType = gameTypeValue->GetValue();
		this->game = ChineseCheckersGame::Factory(gameType.c_str());
		if (!this->game.Get())
		{
			THEBE_LOG("Game type %s not recognized.", gameType.c_str());
			return;
		}

		auto gameStateValue = dynamic_cast<const JsonObject*>(responseRootValue->GetValue("game_state"));
		if (!gameStateValue)
		{
			THEBE_LOG("Game state not found in response.");
			return;
		}

		if (!this->game->FromJson(gameStateValue))
		{
			THEBE_LOG("Failed to create game state from JSON.");
			return;
		}
	}
	else if (response == "get_source_zone_id")
	{
		auto sourceZoneIDValue = dynamic_cast<const JsonInt*>(responseRootValue->GetValue("source_zone_id"));
		if (!sourceZoneIDValue)
		{
			THEBE_LOG("No source zone ID value found in response.");
			return;
		}

		this->sourceZoneID = sourceZoneIDValue->GetValue();
	}
	else if (response == "apply_turn")
	{
		auto nodeOffsetArrayValue = dynamic_cast<const JsonArray*>(responseRootValue->GetValue("node_offset_array"));
		if (!nodeOffsetArrayValue)
		{
			THEBE_LOG("Can't apply turn if there is no offset array.");
			return;
		}

		std::vector<int> nodeOffsetArray;
		for (int i = 0; i < (int)nodeOffsetArrayValue->GetSize(); i++)
		{
			auto offsetValue = dynamic_cast<const JsonInt*>(nodeOffsetArrayValue->GetValue(i));
			if (!offsetValue)
			{
				THEBE_LOG("Offset value was not an integer.");
				return;
			}

			nodeOffsetArray.push_back((int)offsetValue->GetValue());
		}

		std::vector<ChineseCheckersGame::Node*> nodeArray;
		if (!this->game->NodeArrayFromOffsetArray(nodeArray, nodeOffsetArray))
		{
			THEBE_LOG("Failed to convert node offset array to node array.");
			return;
		}

		if (!this->game->ExecutePath(nodeArray))
		{
			THEBE_LOG("Failed to apply node path.");
			return;
		}
	}
	else
	{
		THEBE_LOG("Response \"%s\" not recognized.", response.c_str());
		return;
	}
}

int ChineseCheckersClient::GetSourceZoneID()
{
	return this->sourceZoneID;
}