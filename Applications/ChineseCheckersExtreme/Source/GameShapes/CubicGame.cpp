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

/*virtual*/ bool CubicGame::GetZoneColor(int zoneID, Thebe::Vector3& color)
{
	return false;
}

/*virtual*/ int CubicGame::GetMaxPossiblePlayers() const
{
	return 4;
}

/*virtual*/ const char* CubicGame::GetGameType() const
{
	return "cubic";
}