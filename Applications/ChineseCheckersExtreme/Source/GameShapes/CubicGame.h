#pragma once

#include "Game.h"

/**
 * 
 */
class CubicGame : public ChineseCheckersGame
{
public:
	CubicGame();
	virtual ~CubicGame();

protected:

	virtual void GenerateGraph() override;
	virtual bool PopulateScene(Thebe::Space* space) override;
	virtual bool GetInitialZoneForPlayer(int playerID, int& initialZoneID) override;
	virtual bool GetTargetZoneForPlayer(int playerID, int& targetZoneID) override;
};