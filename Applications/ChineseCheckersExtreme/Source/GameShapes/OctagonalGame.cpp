#include "OctagonalGame.h"

OctagonalGame::OctagonalGame()
{
}

/*virtual*/ OctagonalGame::~OctagonalGame()
{
}

/*virtual*/ bool OctagonalGame::GenerateGraph(int numPlayers)
{
	return false;
}

/*virtual*/ bool OctagonalGame::GetZoneColor(int zoneID, Thebe::Vector3& color)
{
	return false;
}

/*virtual*/ int OctagonalGame::GetMaxPossiblePlayers() const
{
	return 8;
}

/*virtual*/ const char* OctagonalGame::GetGameType() const
{
	return "octagonal";
}

/*virtual*/ void OctagonalGame::GenerateFreeZoneIDStack(std::vector<int>& freeZoneIDStack)
{
}