#pragma once

#include "GameClient.h"

/**
 * 
 */
class ComputerClient : public ChineseCheckersClient
{
public:
	ComputerClient();
	virtual ~ComputerClient();

	virtual void Update() override;
};