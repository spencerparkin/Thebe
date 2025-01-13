#include "GameClient.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------------- ChineseCheckersClient -------------------------------------

ChineseCheckersClient::ChineseCheckersClient()
{
	this->game = nullptr;
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

	this->SetSocketFactory([=](SOCKET socket) -> NetworkSocket*
		{
			NetworkSocket* networkSocket = new Socket(socket, this);
			networkSocket->SetPeriodicWakeup(0);
			return networkSocket;
		});

	if (!NetworkClient::Setup())
		return false;

	auto client = dynamic_cast<Socket*>(this->GetSocket());
	THEBE_ASSERT_FATAL(client != nullptr);

	std::unique_ptr<JsonObject> jsonRequestValue(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_game_state"));
	client->SendJson(jsonRequestValue.get());
	THEBE_LOG("Client sent game state request.");

	jsonRequestValue.reset(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_player_id"));
	client->SendJson(jsonRequestValue.get());
	THEBE_LOG("Client sent player ID request.");

	return true;
}

/*virtual*/ void ChineseCheckersClient::Shutdown()
{
	THEBE_LOG("Game client shutdown");

	NetworkClient::Shutdown();
}

/*virtual*/ void ChineseCheckersClient::Update()
{
	std::unique_ptr<const ParseParty::JsonValue> jsonResponse;
	while (this->RemoveResponse(jsonResponse))
		this->HandleResponse(jsonResponse.get());
}

/*virtual*/ bool ChineseCheckersClient::HandleResponse(const ParseParty::JsonValue* jsonResponse)
{
	using namespace ParseParty;

	auto responseRootValue = dynamic_cast<const JsonObject*>(jsonResponse);
	if (!responseRootValue)
	{
		THEBE_LOG("Json response was not an object.");
		return false;
	}

	auto responseValue = dynamic_cast<const JsonString*>(responseRootValue->GetValue("response"));
	if (!responseValue)
	{
		THEBE_LOG("No response value in JSON response object.");
		return false;
	}

	std::string response = responseValue->GetValue();
	THEBE_LOG("Client got response \"%s\" from server.", response.c_str());

	if (response == "get_game_state")
	{
		auto gameTypeValue = dynamic_cast<const JsonString*>(responseRootValue->GetValue("game_type"));
		if (!gameTypeValue)
		{
			THEBE_LOG("Game type not found in response.");
			return false;
		}

		std::string gameType = gameTypeValue->GetValue();
		this->game = ChineseCheckersGame::Factory(gameType.c_str());
		if (!this->game.Get())
		{
			THEBE_LOG("Game type %s not recognized.", gameType.c_str());
			return false;
		}

		auto gameStateValue = dynamic_cast<const JsonObject*>(responseRootValue->GetValue("game_state"));
		if (!gameStateValue)
		{
			THEBE_LOG("Game state not found in response.");
			return false;
		}

		if (!this->game->FromJson(gameStateValue))
		{
			THEBE_LOG("Failed to create game state from JSON.");
			return false;
		}
	}
	else if (response == "get_player_id")
	{
		auto playerIDValue = dynamic_cast<const JsonInt*>(responseRootValue->GetValue("player_id"));
		if (!playerIDValue)
		{
			THEBE_LOG("No player ID value found in response.");
			return false;
		}

		auto client = dynamic_cast<Socket*>(this->GetSocket());
		THEBE_ASSERT_FATAL(client != nullptr);
		client->playerID = playerIDValue->GetValue();
	}
	else if (response == "flush")
	{
		return true;
	}
	else
	{
		THEBE_LOG("Response \"%s\" not recognized.", response.c_str());
		return false;
	}

	return true;
}

void ChineseCheckersClient::AddResponse(const ParseParty::JsonValue* jsonResponse)
{
	std::lock_guard<std::mutex> lock(this->responseListMutex);
	this->responseList.push_back(jsonResponse);
}

bool ChineseCheckersClient::RemoveResponse(std::unique_ptr<const ParseParty::JsonValue>& jsonResponse)
{
	if (this->responseList.size() > 0)
	{
		std::lock_guard<std::mutex> lock(this->responseListMutex);
		if (this->responseList.size() > 0)
		{
			jsonResponse.reset(*this->responseList.begin());
			this->responseList.pop_front();
			return true;
		}
	}

	return false;
}

//------------------------------------- ChineseCheckersClient::Socket -------------------------------------

ChineseCheckersClient::Socket::Socket(SOCKET socket, ChineseCheckersClient* client) : JsonNetworkSocket(socket)
{
	this->ringBufferSize = 64 * 1024;
	this->recvBufferSize = 4 * 1024;
	this->playerID = 0;
	this->client = client;
}

/*virtual*/ ChineseCheckersClient::Socket::~Socket()
{
}

/*virtual*/ bool ChineseCheckersClient::Socket::ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue)
{
	this->client->AddResponse(jsonRootValue.release());
	return true;
}

/*virtual*/ void ChineseCheckersClient::Socket::OnWakeup()
{
#if 0
	using namespace ParseParty;

	// When we periodically wake up, send a dummy message in an attempt to flush the socket.
	auto rootValue = new JsonObject();
	rootValue->SetValue("request", new JsonString("flush"));

	std::unique_ptr<JsonValue> jsonDummyResponse(rootValue);
	this->SendJson(jsonDummyResponse.get());
#endif
}