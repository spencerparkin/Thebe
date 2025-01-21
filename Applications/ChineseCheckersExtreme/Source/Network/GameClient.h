#pragma once

#include "Application.h"
#include "Thebe/Network/JsonClient.h"
#include "Game.h"

/**
 *
 */
class ChineseCheckersClient : public Thebe::JsonClient
{
public:
	ChineseCheckersClient();
	virtual ~ChineseCheckersClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Update(double deltaTimeSeconds) override;

	ChineseCheckersGame* GetGame();

	int GetSourceZoneID();

	void TakeTurn(const std::vector<ChineseCheckersGame::Node*>& nodeArray);

protected:
	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;

	Thebe::Reference<ChineseCheckersGame> game;
	int whoseTurnZoneID;
	int sourceZoneID;
	double pingFrequencySecondsPerPing;
	double timeSecondsToNextPing;
};