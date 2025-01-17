#include "GameServer.h"
#include "Thebe/Log.h"

using namespace Thebe;

//---------------------------------- ChineseCheckersServer ----------------------------------

ChineseCheckersServer::ChineseCheckersServer()
{
	this->game = nullptr;
	this->whoseTurnZoneID = 0;
}

/*virtual*/ ChineseCheckersServer::~ChineseCheckersServer()
{
}

/*virtual*/ bool ChineseCheckersServer::Setup()
{
	if (!this->game)
	{
		THEBE_LOG("Game server can't start, because no game is set.");
		return false;
	}

	this->game->GenerateFreeZoneIDStack(this->freeZoneIDStack);

	return JsonServer::Setup();
}

/*virtual*/ void ChineseCheckersServer::Shutdown()
{
	this->SetGame(nullptr);

	JsonServer::Shutdown();
}

void ChineseCheckersServer::SetGame(ChineseCheckersGame* game)
{
	this->game = game;
}

/*virtual*/ void ChineseCheckersServer::Serve()
{
	JsonServer::Serve();
}

void ChineseCheckersServer::ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply)
{
	using namespace ParseParty;

	auto requestRootValue = dynamic_cast<const JsonObject*>(message->jsonValue);
	if (!requestRootValue)
	{
		THEBE_LOG("Request JSON root was not an object.");
		return;
	}

	auto requestValue = dynamic_cast<const JsonString*>(requestRootValue->GetValue("request"));
	if (!requestValue)
	{
		THEBE_LOG("No request value found.");
		return;
	}

	std::string request = requestValue->GetValue();
	THEBE_LOG("Server got request \"%s\" from client.", request.c_str());

	if (request == "ping")
	{
		THEBE_LOG("PING!");
		auto jsonResponseValue = new JsonObject();
		jsonReply.reset(jsonResponseValue);
		jsonResponseValue->SetValue("response", new JsonString("pong"));
	}
	else if (request == "get_game_state")
	{
		std::unique_ptr<JsonValue> jsonGameValue;
		if (!this->game->ToJson(jsonGameValue))
		{
			THEBE_LOG("Failed to package game-state into JSON.");
			return;
		}

		auto jsonResponseValue = new JsonObject();
		jsonReply.reset(jsonResponseValue);
		jsonResponseValue->SetValue("response", new JsonString("get_game_state"));
		jsonResponseValue->SetValue("game_type", new JsonString(this->game->GetGameType()));
		jsonResponseValue->SetValue("game_state", jsonGameValue.release());
	}
	else if (request == "get_source_zone_id")
	{
		auto jsonResponseValue = new JsonObject();
		jsonReply.reset(jsonResponseValue);
		jsonResponseValue->SetValue("response", new JsonString("get_source_zone_id"));
		jsonResponseValue->SetValue("source_zone_id", new JsonInt(message->client->GetUserData()));
	}
	else if (request == "take_turn")
	{
		auto nodeOffsetArrayValue = dynamic_cast<const JsonArray*>(requestRootValue->GetValue("node_offset_array"));
		if (!nodeOffsetArrayValue)
		{
			THEBE_LOG("Can't take turn if there is no offset array.");
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

		if (nodeArray.size() == 0)
		{
			THEBE_LOG("Can't take turn if node size is zero.");
			return;
		}

		if (!nodeArray[0]->occupant)
		{
			THEBE_LOG("Can't take turn if node sequence doesn't start at an occupied node.");
			return;
		}

		if (nodeArray[0]->occupant->sourceZoneID != this->whoseTurnZoneID)
		{
			THEBE_LOG("Can't take turn when it is not your turn.");
			return;
		}

		if (!this->game->ExecutePath(nodeArray))
		{
			THEBE_LOG("Failed to execute node path.  It was not legal.");
			return;
		}

		std::unique_ptr<JsonObject> responseValue(new JsonObject());
		responseValue->SetValue("response", new JsonString("apply_turn"));
		auto offsetArrayValue = new JsonArray();
		responseValue->SetValue("node_offset_array", offsetArrayValue);
		for (int i : nodeOffsetArray)
			offsetArrayValue->PushValue(new JsonInt(i));

		this->clientManager->SendJsonToAllClients(responseValue.get());

		this->whoseTurnZoneID = this->game->GetNextZone(this->whoseTurnZoneID);

		this->NotifyAllClientsOfWhoseTurnItIs();
	}
	else
	{
		THEBE_LOG("Request \"%s\" not recognized.", request.c_str());
	}
}

void ChineseCheckersServer::NotifyAllClientsOfWhoseTurnItIs()
{
	using namespace ParseParty;

	std::unique_ptr<JsonObject> responseValue(new JsonObject());
	responseValue->SetValue("response", new JsonString("turn_notify"));
	responseValue->SetValue("whose_turn_zone_id", new JsonInt(this->whoseTurnZoneID));

	this->clientManager->SendJsonToAllClients(responseValue.get());
}

/*virtual*/ void ChineseCheckersServer::OnClientConnected(ConnectedClient* client)
{
	if (this->freeZoneIDStack.size() > 0)
	{
		int sourceZoneID = this->freeZoneIDStack.back();
		this->freeZoneIDStack.pop_back();
		client->SetUserData(sourceZoneID);
	}

	if (this->whoseTurnZoneID == 0)
		this->whoseTurnZoneID = (int)client->GetUserData();

	if (this->clientManager->GetNumConnectedClients() == this->game->GetNumActivePlayers())
		this->NotifyAllClientsOfWhoseTurnItIs();
}

/*virtual*/ void ChineseCheckersServer::OnClientDisconnected(ConnectedClient* client)
{
	int sourceZoneID = (int)client->GetUserData();
	this->freeZoneIDStack.push_back(sourceZoneID);
}