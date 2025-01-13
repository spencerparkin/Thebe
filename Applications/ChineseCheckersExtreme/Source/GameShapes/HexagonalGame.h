#pragma once

#include "Game.h"

/**
 * Here we represent the traditional 6-pronged hexagonal star configuration of the game.
 */
class HexagonalGame : public ChineseCheckersGame
{
public:
	HexagonalGame();
	virtual ~HexagonalGame();

protected:

	virtual void GenerateGraph() override;
	virtual bool GetZoneColor(int zoneID, Thebe::Vector3& color) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
};