#pragma once

#include "GameClient.h"

/**
 *
 */
class HumanClient : public ChineseCheckersClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;

	void TakeTurn(const std::vector<ChineseCheckersGame::Node*>& nodeArray);

	void SnapCubiesIntoPosition();

private:
	bool animate;
};