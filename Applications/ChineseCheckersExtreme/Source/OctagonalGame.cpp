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

/*virtual*/ bool OctagonalGame::PopulateScene(Thebe::Space* space)
{
	return true;
}

/*virtual*/ bool OctagonalGame::GetInitialZoneForPlayer(int playerID, int& initialZoneID)
{
	return true;
}

/*virtual*/ bool OctagonalGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}