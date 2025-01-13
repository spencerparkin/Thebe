#include "OctagonalGame.h"

OctagonalGame::OctagonalGame()
{
}

/*virtual*/ OctagonalGame::~OctagonalGame()
{
}

/*virtual*/ void OctagonalGame::GenerateGraph()
{
}

/*virtual*/ bool OctagonalGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}

/*virtual*/ int OctagonalGame::GetMaxPossiblePlayers() const
{
	return 8;
}

/*virtual*/ const char* OctagonalGame::GetGameType() const
{
	return "octagonal";
}