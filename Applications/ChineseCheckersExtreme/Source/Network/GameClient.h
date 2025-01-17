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
	virtual void Update() override;

	ChineseCheckersGame* GetGame();

	int GetSourceZoneID();

protected:
	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;

	Thebe::Reference<ChineseCheckersGame> game;
	int whoseTurnZoneID;
	int sourceZoneID;
};