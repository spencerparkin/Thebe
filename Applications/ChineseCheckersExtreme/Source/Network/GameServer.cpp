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

	this->SetSocketFactory([=](SOCKET socket) -> NetworkSocket*
		{
			NetworkSocket* networkSocket = new Socket(socket, this);
			networkSocket->SetPeriodicWakeup(0);
			return networkSocket;
		});

	return NetworkServer::Setup();
}

/*virtual*/ void ChineseCheckersServer::Shutdown()
{
	THEBE_LOG("Game server shutdown");

	this->SetGame(nullptr);

	NetworkServer::Shutdown();
}

void ChineseCheckersServer::SetGame(ChineseCheckersGame* game)
{
	this->game = game;
}

bool ChineseCheckersServer::ServeRequest(const ParseParty::JsonValue* jsonRequest, std::unique_ptr<ParseParty::JsonValue>& jsonResponse, Socket* client)
{
	using namespace ParseParty;

	std::lock_guard<std::mutex> lock(this->serverMutex);

	auto requestRootValue = dynamic_cast<const JsonObject*>(jsonRequest);
	if (!requestRootValue)
	{
		THEBE_LOG("Request JSON root was not an object.");
		return false;
	}

	auto requestValue = dynamic_cast<const JsonString*>(requestRootValue->GetValue("request"));
	if (!requestValue)
	{
		THEBE_LOG("No request value found.");
		return false;
	}

	std::string request = requestValue->GetValue();
	THEBE_LOG("Server got request \"%s\" from client.", request.c_str());

	if (request == "ping")
	{
		THEBE_LOG("PING!");
		auto jsonResponseValue = new JsonObject();
		jsonResponse.reset(jsonResponseValue);
		jsonResponseValue->SetValue("response", new JsonString("pong"));
	}
	else if (request == "get_game_state")
	{
		std::unique_ptr<JsonValue> jsonGameValue;
		if (!this->game->ToJson(jsonGameValue))
		{
			THEBE_LOG("Failed to package game-state into JSON.");
			return false;
		}

		auto jsonResponseValue = new JsonObject();
		jsonResponse.reset(jsonResponseValue);
		jsonResponseValue->SetValue("response", new JsonString("get_game_state"));
		jsonResponseValue->SetValue("game_type", new JsonString(this->game->GetGameType()));
		jsonResponseValue->SetValue("game_state", jsonGameValue.release());
	}
	else if (request == "get_source_zone_id")
	{
		auto jsonResponseValue = new JsonObject();
		jsonResponse.reset(jsonResponseValue);
		jsonResponseValue->SetValue("response", new JsonString("get_source_zone_id"));
		jsonResponseValue->SetValue("source_zone_id", new JsonInt(client->sourceZoneID));
	}
	else if (request == "take_turn")
	{
		auto nodeOffsetArrayValue = dynamic_cast<const JsonArray*>(requestRootValue->GetValue("node_offset_array"));
		if (!nodeOffsetArrayValue)
		{
			THEBE_LOG("Can't take turn if there is no offset array.");
			return false;
		}

		std::vector<int> nodeOffsetArray;
		for (int i = 0; i < (int)nodeOffsetArrayValue->GetSize(); i++)
		{
			auto offsetValue = dynamic_cast<const JsonInt*>(nodeOffsetArrayValue->GetValue(i));
			if (!offsetValue)
			{
				THEBE_LOG("Offset value was not an integer.");
				return false;
			}

			nodeOffsetArray.push_back((int)offsetValue->GetValue());
		}

		std::vector<ChineseCheckersGame::Node*> nodeArray;
		if (!this->game->NodeArrayFromOffsetArray(nodeArray, nodeOffsetArray))
		{
			THEBE_LOG("Failed to convert node offset array to node array.");
			return false;
		}

		if (nodeArray.size() == 0)
		{
			THEBE_LOG("Can't take turn if node size is zero.");
			return false;
		}

		if (!nodeArray[0]->occupant)
		{
			THEBE_LOG("Can't take turn if node sequence doesn't start at an occupied node.");
			return false;
		}

		if (nodeArray[0]->occupant->sourceZoneID != this->whoseTurnZoneID)
		{
			THEBE_LOG("Can't take turn when it is not your turn.");
			return false;
		}

		if (!this->game->ExecutePath(nodeArray))
		{
			THEBE_LOG("Failed to execute node path.  It was not legal.");
			return false;
		}

		std::unique_ptr<JsonObject> responseValue(new JsonObject());
		responseValue->SetValue("response", new JsonString("apply_turn"));
		auto offsetArrayValue = new JsonArray();
		responseValue->SetValue("node_offset_array", offsetArrayValue);
		for (int i : nodeOffsetArray)
			offsetArrayValue->PushValue(new JsonInt(i));

		for (auto networkSocket : connectedClientList)
		{
			auto client = dynamic_cast<Socket*>(networkSocket);
			client->SendJson(responseValue.get());
		}
	}
	else
	{
		THEBE_LOG("Request \"%s\" not recognized.", request.c_str());
		return false;
	}

	return true;
}

/*virtual*/ void ChineseCheckersServer::OnClientAdded(Thebe::NetworkSocket* networkSocket)
{
	auto client = dynamic_cast<Socket*>(networkSocket);
	THEBE_ASSERT_FATAL(client);

	std::lock_guard<std::mutex> lock(this->serverMutex);

	if (this->freeZoneIDStack.size() > 0)
	{
		client->sourceZoneID = this->freeZoneIDStack.back();
		this->freeZoneIDStack.pop_back();
	}

	if (this->whoseTurnZoneID == 0)
		this->whoseTurnZoneID = client->sourceZoneID;
}

/*virtual*/ void ChineseCheckersServer::OnClientRemoved(Thebe::NetworkSocket* networkSocket)
{
	auto client = dynamic_cast<Socket*>(networkSocket);
	THEBE_ASSERT_FATAL(client);

	std::lock_guard<std::mutex> lock(this->serverMutex);

	this->freeZoneIDStack.push_back(client->sourceZoneID);
}

//---------------------------------- ChineseCheckersServer::Socket ----------------------------------

ChineseCheckersServer::Socket::Socket(SOCKET socket, ChineseCheckersServer* server) : JsonNetworkSocket(socket)
{
	this->ringBufferSize = 64 * 1024;
	this->recvBufferSize = 4 * 1024;
	this->sourceZoneID = 0;
	this->server = server;
}

/*virtual*/ ChineseCheckersServer::Socket::~Socket()
{
}

/*virtual*/ bool ChineseCheckersServer::Socket::ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue)
{
	using namespace ParseParty;

	std::unique_ptr<JsonValue> jsonResponse;

	if (this->server->ServeRequest(jsonRootValue.get(), jsonResponse, this))
	{
		// Not all requests require a response.
		if (jsonResponse.get())
			this->SendJson(jsonResponse.get());
	}

	return true;
}