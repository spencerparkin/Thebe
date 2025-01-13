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
	virtual bool GetZoneColor(int zoneID, Thebe::Vector3& color) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
};