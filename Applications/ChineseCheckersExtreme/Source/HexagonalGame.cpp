#include "HexagonalGame.h"

HexagonalGame::HexagonalGame()
{
}

/*virtual*/ HexagonalGame::~HexagonalGame()
{
}

/*virtual*/ void HexagonalGame::GenerateGraph()
{
}

/*virtual*/ bool HexagonalGame::PopulateScene(Thebe::Space* space)
{
	return true;
}

/*virtual*/ bool HexagonalGame::GetInitialZoneForPlayer(int playerID, int& initialZoneID)
{
	return true;
}

/*virtual*/ bool HexagonalGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}