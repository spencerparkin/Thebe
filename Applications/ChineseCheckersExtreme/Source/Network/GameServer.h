#pragma once

#include "Thebe/Network/JsonServer.h"
#include "Game.h"

/**
 * 
 */
class ChineseCheckersServer : public Thebe::JsonServer
{
public:
	ChineseCheckersServer();
	virtual ~ChineseCheckersServer();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Serve() override;

	void SetGame(ChineseCheckersGame* game);

protected:
	virtual void ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply) override;

	virtual void OnClientConnected(ConnectedClient* client) override;
	virtual void OnClientDisconnected(ConnectedClient* client) override;

	void NotifyAllClientsOfWhoseTurnItIs();

	Thebe::Reference<ChineseCheckersGame> game;
	std::vector<int> freeZoneIDStack;
	int whoseTurnZoneID;
};