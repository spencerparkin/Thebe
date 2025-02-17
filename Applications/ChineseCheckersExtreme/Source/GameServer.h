#pragma once

#include "Thebe/Network/JsonServer.h"
#include "ChineseCheckers/Graph.h"
#include "ChineseCheckers/Factory.h"
#include "ChineseCheckers/GraphGenerator.h"

class ChineseCheckersGameServer : public Thebe::JsonServer
{
public:
	ChineseCheckersGameServer();
	virtual ~ChineseCheckersGameServer();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Serve() override;

	virtual void OnClientConnected(ConnectedClient* client) override;
	virtual void OnClientDisconnected(ConnectedClient* client) override;

	virtual void ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply) override;

	void SetNumPlayers(int numPlayers);

protected:
	std::unique_ptr<ChineseCheckers::Factory> factory;
	std::set<ChineseCheckers::Marble::Color> participantSet;
	std::vector<ChineseCheckers::Marble::Color> freeColorStack;
	std::unique_ptr<ChineseCheckers::GraphGenerator> graphGenerator;
	std::unique_ptr<ChineseCheckers::Graph> graph;
	ChineseCheckers::Marble::Color whoseTurn;
	ChineseCheckers::Marble::Color winner;
};