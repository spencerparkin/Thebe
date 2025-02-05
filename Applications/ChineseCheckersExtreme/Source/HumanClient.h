#pragma once

#include "GameClient.h"

class HumanClient : public ChineseCheckersGameClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;
};