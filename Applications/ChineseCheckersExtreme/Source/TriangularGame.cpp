#include "TriangularGame.h"

TriangularGame::TriangularGame()
{
}

/*virtual*/ TriangularGame::~TriangularGame()
{
}

/*virtual*/ void TriangularGame::GenerateGraph()
{
}

/*virtual*/ bool TriangularGame::PopulateScene(Thebe::Space* space)
{
	return true;
}

/*virtual*/ bool TriangularGame::GetInitialZoneForPlayer(int playerID, int& initialZoneID)
{
	return true;
}

/*virtual*/ bool TriangularGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}