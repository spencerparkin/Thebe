#pragma once

#if 0
#include "GameClient.h"

/**
 *
 */
class HumanClient : public ChineseCheckersClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual bool HandleResponse(const ParseParty::JsonValue* jsonResponse) override;

	void TakeTurn(const std::vector<ChineseCheckersGame::Node*>& nodeArray);

	void SnapCubiesIntoPosition();

private:
	bool animate;
};
#endif