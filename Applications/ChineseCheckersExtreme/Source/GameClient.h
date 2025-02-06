#pragma once

#include "Thebe/Network/JsonClient.h"
#include "ChineseCheckers/Graph.h"
#include "ChineseCheckers/Marble.h"
#include "MoveSequence.h"

class ChineseCheckersGameClient : public Thebe::JsonClient
{
public:
	ChineseCheckersGameClient();
	virtual ~ChineseCheckersGameClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Update(double deltaTimeSeconds) override;
	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;
	virtual void HandleConnectionStatus(ConnectionStatus status, int i, bool* abort) override;

	ChineseCheckers::Graph* GetGraph();
	ChineseCheckers::Marble::Color GetColor() const;

	void MakeMove(const MoveSequence& moveSequence);

protected:
	std::unique_ptr<ChineseCheckers::Factory> factory;
	std::unique_ptr<ChineseCheckers::Graph> graph;
	ChineseCheckers::Marble::Color color;
	ChineseCheckers::Marble::Color whoseTurn;
};