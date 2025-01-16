#pragma once

#include "Thebe/Network/Server.h"
#include "Game.h"

/**
 * This is the single source of truth for the state of the game
 * across all partipants on the network.
 */
class ChineseCheckersServer : public Thebe::NetworkServer
{
public:
	ChineseCheckersServer();
	virtual ~ChineseCheckersServer();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual bool Serve() override;

	void SetGame(ChineseCheckersGame* game);

	class Socket : public Thebe::JsonNetworkSocket
	{
	public:
		Socket(SOCKET socket, ChineseCheckersServer* server);
		virtual ~Socket();

		virtual bool ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue) override;

		ChineseCheckersServer* server;
		int sourceZoneID;
	};

	bool ServeRequest(const ParseParty::JsonValue* jsonRequest, std::unique_ptr<ParseParty::JsonValue>& jsonResponse, Socket* client);

	void AddRequest(std::unique_ptr<ParseParty::JsonValue>& jsonRequest, Socket* client);

protected:
	virtual void OnClientAdded(Thebe::NetworkSocket* networkSocket) override;
	virtual void OnClientRemoved(Thebe::NetworkSocket* networkSocket) override;

	void NotifyAllClientsOfWhoseTurnItIs();

	struct Request
	{
		const ParseParty::JsonValue* jsonRequest;
		Socket* client;
	};

	bool RemoveRequest(std::unique_ptr<const ParseParty::JsonValue>& jsonRequest, Socket*& client);

	Thebe::Reference<ChineseCheckersGame> game;
	Thebe::ThreadSafeQueue<Request> requestQueue;
	std::mutex serverMutex;
	std::vector<int> freeZoneIDStack;
	int whoseTurnZoneID;
};