#pragma once

#include "Game.h"

/**
 * 
 */
class OctagonalGame : public ChineseCheckersGame
{
public:
	OctagonalGame();
	virtual ~OctagonalGame();

protected:

	virtual void GenerateGraph() override;
	virtual bool PopulateScene(Thebe::Space* space) override;
	virtual bool GetTargetZoneForPlayer(int playerID, int& targetZoneID) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
};