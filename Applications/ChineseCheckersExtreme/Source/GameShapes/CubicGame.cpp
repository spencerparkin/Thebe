#include "CubicGame.h"

CubicGame::CubicGame()
{
}

/*virtual*/ CubicGame::~CubicGame()
{
}

/*virtual*/ void CubicGame::GenerateGraph()
{
}

/*virtual*/ bool CubicGame::PopulateScene(Thebe::Space* space)
{
	return true;
}

/*virtual*/ bool CubicGame::GetInitialZoneForPlayer(int playerID, int& initialZoneID)
{
	return true;
}

/*virtual*/ bool CubicGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}