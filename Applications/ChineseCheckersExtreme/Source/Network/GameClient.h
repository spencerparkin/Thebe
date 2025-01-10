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

	ChineseCheckersGame* GetGame();

	bool HandleResponse(const ParseParty::JsonValue* jsonResponse);

	class Socket : public Thebe::JsonNetworkSocket
	{
	public:
		Socket(SOCKET socket, ChineseCheckersClient* client);
		virtual ~Socket();

		virtual bool ReceiveJson(const ParseParty::JsonValue* jsonRootValue) override;

		ChineseCheckersClient* client;
		int playerID;
	};

private:
	ChineseCheckersGame* game;
	std::mutex clientMutex;
};