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

/*virtual*/ bool CubicGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}

/*virtual*/ int CubicGame::GetMaxPossiblePlayers() const
{
	return 4;
}

/*virtual*/ const char* CubicGame::GetGameType() const
{
	return "cubic";
}