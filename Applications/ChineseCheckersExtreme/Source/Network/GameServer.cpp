#include "GameServer.h"
#include "Thebe/Log.h"

using namespace Thebe;

//---------------------------------- ChineseCheckersServer ----------------------------------

ChineseCheckersServer::ChineseCheckersServer()
{
	this->game = nullptr;
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

	if (request == "get_game_state")
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
		// TODO: Try to do the given move on our game state.

		// TODO: If the move is acceptable, go tell all the clients to make that move.  This includes the client that made the request.
		for (auto client : connectedClientList)
		{
			//...
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