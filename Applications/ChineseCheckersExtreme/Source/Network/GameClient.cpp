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
	delete this->game;
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
			return new Socket(socket, this);
		});

	if (!NetworkClient::Setup())
		return false;

	auto client = dynamic_cast<Socket*>(this->GetSocket());
	THEBE_ASSERT_FATAL(client != nullptr);

	std::unique_ptr<JsonObject> jsonRequestValue(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_game_state"));
	client->SendJson(jsonRequestValue.get());

	jsonRequestValue.reset(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_player_id"));
	client->SendJson(jsonRequestValue.get());

	return true;
}

/*virtual*/ void ChineseCheckersClient::Shutdown()
{
	NetworkClient::Shutdown();
}

bool ChineseCheckersClient::HandleResponse(const ParseParty::JsonValue* jsonResponse)
{
	using namespace ParseParty;

	// Typically we'd want any mutex lock to be much tighter than this, but I think this will be okay.
	std::lock_guard<std::mutex> lock(this->clientMutex);

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

	if (response == "get_game_state")
	{
		auto gameTypeValue = dynamic_cast<const JsonString*>(responseRootValue->GetValue("game_type"));
		if (!gameTypeValue)
		{
			THEBE_LOG("Game type not found in response.");
			return false;
		}

		std::string gameType = gameTypeValue->GetValue();
		delete this->game;
		this->game = ChineseCheckersGame::Factory(gameType.c_str());
		if (!this->game)
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
	else
	{
		THEBE_LOG("Response \"%s\" not recognized.", response.c_str());
		return false;
	}

	return true;
}

//------------------------------------- ChineseCheckersClient::Socket -------------------------------------

ChineseCheckersClient::Socket::Socket(SOCKET socket, ChineseCheckersClient* client) : JsonNetworkSocket(socket)
{
	this->playerID = 0;
	this->client = client;
}

/*virtual*/ ChineseCheckersClient::Socket::~Socket()
{
}

/*virtual*/ bool ChineseCheckersClient::Socket::ReceiveJson(const ParseParty::JsonValue* jsonRootValue)
{
	this->client->HandleResponse(jsonRootValue);
	return true;
}