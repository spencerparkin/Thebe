#include "GameClient.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------------- ChineseCheckersClient -------------------------------------

ChineseCheckersClient::ChineseCheckersClient()
{
	this->game = nullptr;
	this->pingFrequencySecondsPerPing = 2.0;
	this->timeToNextPingSeconds = 0.0;
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

	jsonRequestValue.reset(new JsonObject());
	jsonRequestValue->SetValue("request", new JsonString("get_source_zone_id"));
	client->SendJson(jsonRequestValue.get());

	return true;
}

/*virtual*/ void ChineseCheckersClient::Shutdown()
{
	THEBE_LOG("Game client shutdown");

	NetworkClient::Shutdown();
}

/*virtual*/ void ChineseCheckersClient::Update(double deltaTimeSeconds)
{
	std::unique_ptr<const ParseParty::JsonValue> jsonResponse;
	while (this->RemoveResponse(jsonResponse))
		this->HandleResponse(jsonResponse.get());

	using namespace ParseParty;

	if (this->pingFrequencySecondsPerPing > 0.0)
	{
		this->timeToNextPingSeconds -= deltaTimeSeconds;
		if (this->timeToNextPingSeconds <= 0.0)
		{
			this->timeToNextPingSeconds += this->pingFrequencySecondsPerPing;
			auto socket = dynamic_cast<Socket*>(this->GetSocket());
			std::unique_ptr<JsonObject> pingValue(new JsonObject());
			pingValue->SetValue("request", new JsonString("ping"));
			socket->SendJson(pingValue.get());
		}
	}
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

	if (response == "pong")
	{
		THEBE_LOG("PONG!");
	}
	else if (response == "get_game_state")
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
	else if (response == "get_source_zone_id")
	{
		auto sourceZoneIDValue = dynamic_cast<const JsonInt*>(responseRootValue->GetValue("source_zone_id"));
		if (!sourceZoneIDValue)
		{
			THEBE_LOG("No source zone ID value found in response.");
			return false;
		}

		auto client = dynamic_cast<Socket*>(this->GetSocket());
		THEBE_ASSERT_FATAL(client != nullptr);
		client->sourceZoneID = sourceZoneIDValue->GetValue();
	}
	else if (response == "apply_turn")
	{
		auto nodeOffsetArrayValue = dynamic_cast<const JsonArray*>(responseRootValue->GetValue("node_offset_array"));
		if (!nodeOffsetArrayValue)
		{
			THEBE_LOG("Can't apply turn if there is no offset array.");
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

		if (!this->game->ExecutePath(nodeArray))
		{
			THEBE_LOG("Failed to apply node path.");
			return false;
		}
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

int ChineseCheckersClient::GetSourceZoneID()
{
	auto socket = dynamic_cast<Socket*>(this->clientSocket);
	THEBE_ASSERT_FATAL(socket != nullptr);
	return socket->sourceZoneID;
}

//------------------------------------- ChineseCheckersClient::Socket -------------------------------------

ChineseCheckersClient::Socket::Socket(SOCKET socket, ChineseCheckersClient* client) : JsonNetworkSocket(socket)
{
	this->ringBufferSize = 64 * 1024;
	this->recvBufferSize = 4 * 1024;
	this->sourceZoneID = 0;
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