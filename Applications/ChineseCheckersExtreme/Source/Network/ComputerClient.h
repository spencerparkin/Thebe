#pragma once

#if 0
#include "GameClient.h"

/**
 * 
 */
class ComputerClient : public ChineseCheckersClient
{
public:
	ComputerClient();
	virtual ~ComputerClient();

	virtual bool HandleResponse(const ParseParty::JsonValue* jsonResponse) override;
	virtual void Update(double deltaTimeSeconds) override;
};
#endif