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
	virtual bool PopulateScene(Thebe::Space* space) override;
	virtual bool GetInitialZoneForPlayer(int playerID, int& initialZoneID) override;
	virtual bool GetTargetZoneForPlayer(int playerID, int& targetZoneID) override;
	virtual int GetMaxPossiblePlayers() const override;
	virtual const char* GetGameType() const override;
};