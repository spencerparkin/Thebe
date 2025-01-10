#include "Game.h"

//---------------------------------- ChineseCheckersGame ----------------------------------

ChineseCheckersGame::ChineseCheckersGame()
{
}

/*virtual*/ ChineseCheckersGame::~ChineseCheckersGame()
{
	this->Clear();
}

void ChineseCheckersGame::Clear()
{
	this->nodeArray.clear();
	this->occupantArray.clear();
}

bool ChineseCheckersGame::ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue) const
{
	return true;
}

bool ChineseCheckersGame::FromJson(const ParseParty::JsonArray* jsonRootValue)
{
	this->Clear();

	return true;
}

//---------------------------------- ChineseCheckersGame::Occupant ----------------------------------

ChineseCheckersGame::Occupant::Occupant()
{
	this->playerID = 0;
	this->health = 1.0;
	this->attackPower = 0.0;
}

/*virtual*/ ChineseCheckersGame::Occupant::~Occupant()
{
}

//---------------------------------- ChineseCheckersGame::Node ----------------------------------

ChineseCheckersGame::Node::Node()
{
	this->occupant = 0;
	this->zoneID = 0;
}

/*virtual*/ ChineseCheckersGame::Node::~Node()
{
}