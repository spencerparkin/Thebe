#pragma once

#include "GameClient.h"

class ComputerClient : public ChineseCheckersGameClient
{
public:
	ComputerClient();
	virtual ~ComputerClient();

	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;
};