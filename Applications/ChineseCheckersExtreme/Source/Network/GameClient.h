#pragma once

#include "Thebe/Network/Client.h"
#include "Game.h"

/**
 * These can be human or computer controlled.
 */
class ChineseCheckersClient : public Thebe::NetworkClient
{
public:
	ChineseCheckersClient();
	virtual ~ChineseCheckersClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;

	virtual void Update();

	ChineseCheckersGame* GetGame();

	class Socket : public Thebe::JsonNetworkSocket
	{
	public:
		Socket(SOCKET socket, ChineseCheckersClient* client);
		virtual ~Socket();

		virtual bool ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue) override;

		ChineseCheckersClient* client;
		int playerID;
	};

	virtual bool HandleResponse(const ParseParty::JsonValue* jsonResponse);

	void AddResponse(const ParseParty::JsonValue* jsonResponse);

private:

	bool RemoveResponse(std::unique_ptr<const ParseParty::JsonValue>& jsonResponse);

	Thebe::Reference<ChineseCheckersGame> game;
	std::list<const ParseParty::JsonValue*> responseList;
	std::mutex responseListMutex;
};