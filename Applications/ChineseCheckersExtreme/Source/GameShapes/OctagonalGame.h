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

	virtual bool GenerateGraph(int numPlayers) override;
	virtual bool GetZoneColor(int zoneID, Thebe::Vector3& color) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
	virtual void GenerateFreeZoneIDStack(std::vector<int>& freeZoneIDStack) override;
};