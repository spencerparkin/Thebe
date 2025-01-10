#pragma once

#include "Thebe/Network/Server.h"
#include "Game.h"

/**
 * This is the single source of truth for the state of the game
 * across all partipants on the network.
 */
class ChineseCheckersGameServer : public Thebe::NetworkServer
{
public:
	ChineseCheckersGameServer();
	virtual ~ChineseCheckersGameServer();

	virtual bool Setup() override;
	virtual void Shutdown() override;

	void SetGame(ChineseCheckersGame* game);

	class Socket : public Thebe::JsonNetworkSocket
	{
	public:
		Socket(SOCKET socket, ChineseCheckersGameServer* server);
		virtual ~Socket();

		virtual bool ReceiveJson(const ParseParty::JsonValue* jsonRootValue) override;

		ChineseCheckersGameServer* server;
		int playerID;
	};

	bool ServeRequest(const ParseParty::JsonValue* jsonRequest, std::unique_ptr<ParseParty::JsonValue>& jsonResponse, Socket* client);

protected:
	virtual void OnClientAdded(Thebe::NetworkSocket* networkSocket) override;
	virtual void OnClientRemoved(Thebe::NetworkSocket* networkSocket) override;

	ChineseCheckersGame* game;
	std::mutex serverMutex;
	std::vector<int> freePlayerIDStack;
};