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

	virtual bool HandleResponse(const ParseParty::JsonValue* jsonResponse) override;
};