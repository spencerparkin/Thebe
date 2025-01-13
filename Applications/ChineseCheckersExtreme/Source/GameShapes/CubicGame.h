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
	virtual bool GetTargetZoneForPlayer(int playerID, int& targetZoneID) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
};