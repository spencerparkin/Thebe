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
	virtual bool GetZoneColor(int zoneID, Thebe::Vector3& color) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
};