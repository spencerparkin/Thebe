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

	virtual bool GenerateGraph(int numPlayers) override;
	virtual bool GetZoneColor(int zoneID, Thebe::Vector3& color) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
	virtual void GenerateFreePlayerIDStack(std::vector<int>& freePlayerIDStack) override;
};