#pragma once

#include "Game.h"

/**
 * Here we use triangules instead of hexagons for the platforms upon
 * which the pieces hop.  This shape is a bit tricky, though, because
 * there is no hop that goes directly over another piece.  To compensate,
 * we let you hop over any triangular platform provided it's occupied.
 * This is encorporated into the general rules by allowing a tolerance
 * in the angle made by how you entered or exited a platform.  Under
 * normal circumstances, the tolerance is zero, meaning that to hop
 * over a platform, you must exit in the same direction at which you entered.
 */
class TriangularGame : public ChineseCheckersGame
{
public:
	TriangularGame();
	virtual ~TriangularGame();

protected:

	virtual void GenerateGraph() override;
	virtual bool PopulateScene(Thebe::Space* space) override;
	virtual bool GetInitialZoneForPlayer(int playerID, int& initialZoneID) override;
	virtual bool GetTargetZoneForPlayer(int playerID, int& targetZoneID) override;
};