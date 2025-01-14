#include "Game.h"
#include "GameShapes/CubicGame.h"
#include "GameShapes/HexagonalGame.h"
#include "GameShapes/OctagonalGame.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/Log.h"

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
	using namespace ParseParty;

	auto rootValue = new JsonObject();
	jsonRootValue.reset(rootValue);

	std::map<Node*, int> nodeOffsetMap;
	for (int i = 0; i < (int)this->nodeArray.size(); i++)
		nodeOffsetMap.insert(std::pair(this->nodeArray[i], i));

	std::map<Occupant*, int> occupantOffsetMap;
	for (int i = 0; i < (int)this->occupantArray.size(); i++)
		occupantOffsetMap.insert(std::pair(this->occupantArray[i], i));

	auto nodeArrayValue = new JsonArray();
	rootValue->SetValue("node_array", nodeArrayValue);

	for (auto node : this->nodeArray)
	{
		auto nodeValue = new JsonObject();
		nodeArrayValue->PushValue(nodeValue);

		nodeValue->SetValue("zone_id", new JsonInt(node->zoneID));
		nodeValue->SetValue("location", Thebe::JsonHelper::VectorToJsonValue(node->location));

		auto adjacencyArrayValue = new JsonArray();
		nodeValue->SetValue("adjacency_array", adjacencyArrayValue);
		for (auto adjacentNode : node->adjacentNodeArray)
		{
			int i = nodeOffsetMap.find(adjacentNode)->second;
			adjacencyArrayValue->PushValue(new JsonInt(i));
		}

		if (node->occupant)
		{
			int i = occupantOffsetMap.find(node->occupant)->second;
			nodeValue->SetValue("occupant", new JsonInt(i));
		}
	}

	auto occupantArrayValue = new JsonArray();
	rootValue->SetValue("occupant_array", occupantArrayValue);

	for (auto occupant : this->occupantArray)
	{
		auto occupantValue = new JsonObject();
		occupantArrayValue->PushValue(occupantValue);
		occupantValue->SetValue("player_id", new JsonInt(occupant->playerID));
		occupantValue->SetValue("target_zone_id", new JsonInt(occupant->targetZoneID));
		occupantValue->SetValue("health", new JsonFloat(occupant->health));
		occupantValue->SetValue("attack_power", new JsonFloat(occupant->attackPower));
	}

	return true;
}

bool ChineseCheckersGame::FromJson(const ParseParty::JsonValue* jsonRootValue)
{
	using namespace ParseParty;

	this->Clear();

	auto rootValue = dynamic_cast<const JsonObject*>(jsonRootValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root JSON value to be an object.");
		return false;
	}

	auto nodeArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("node_array"));
	if (!nodeArrayValue)
	{
		THEBE_LOG("No node array value found.");
		return false;
	}

	auto occupantArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("occupant_array"));
	if (!occupantArrayValue)
	{
		THEBE_LOG("No occupant array value found.");
		return false;
	}

	for (int i = 0; i < (int)occupantArrayValue->GetSize(); i++)
	{
		auto occupantValue = dynamic_cast<const JsonObject*>(occupantArrayValue->GetValue(i));
		if (!occupantValue)
		{
			THEBE_LOG("Expected entry %d of occupant array to be an object.", i);
			return false;
		}

		auto playerIDValue = dynamic_cast<const JsonInt*>(occupantValue->GetValue("player_id"));
		auto targetZoneIDValue = dynamic_cast<const JsonInt*>(occupantValue->GetValue("target_zone_id"));
		auto healthValue = dynamic_cast<const JsonFloat*>(occupantValue->GetValue("health"));
		auto attackPowerValue = dynamic_cast<const JsonFloat*>(occupantValue->GetValue("attack_power"));

		if (!playerIDValue || !targetZoneIDValue || !healthValue || !attackPowerValue)
		{
			THEBE_LOG("Not all values could be found for occupant entry %d.", i);
			return false;
		}

		auto occupant = new Occupant();
		occupant->playerID = (int)playerIDValue->GetValue();
		occupant->targetZoneID = (int)targetZoneIDValue->GetValue();
		occupant->health = healthValue->GetValue();
		occupant->attackPower = attackPowerValue->GetValue();
		this->occupantArray.push_back(occupant);
	}

	for (int i = 0; i < (int)nodeArrayValue->GetSize(); i++)
	{
		auto nodeValue = dynamic_cast<const JsonObject*>(nodeArrayValue->GetValue(i));
		if (!nodeValue)
		{
			THEBE_LOG("Expected entry %d of node array to be an object.", i);
			return false;
		}

		auto node = new Node();
		this->nodeArray.push_back(node);

		auto zoneIDValue = dynamic_cast<const JsonInt*>(nodeValue->GetValue("zone_id"));
		if (!zoneIDValue)
		{
			THEBE_LOG("No zone ID found.");
			return false;
		}

		node->zoneID = (int)zoneIDValue->GetValue();

		if (!Thebe::JsonHelper::VectorFromJsonValue(nodeValue->GetValue("location"), node->location))
		{
			THEBE_LOG("Failed to get node location.");
			return false;
		}

		auto occupantValue = dynamic_cast<const JsonInt*>(nodeValue->GetValue("occupant"));
		if (occupantValue)
		{
			int j = (int)occupantValue->GetValue();
			if (j < 0 || j >= (int)this->occupantArray.size())
			{
				THEBE_LOG("Node %d has occupant %d which is out of range 0 to %d.", i, j, this->occupantArray.size() - 1);
				return false;
			}

			node->occupant = this->occupantArray[j].Get();
		}
	}

	// Wire up the graph on the second pass now that all the nodes have been created.
	for (int i = 0; i < (int)nodeArrayValue->GetSize(); i++)
	{
		auto nodeValue = dynamic_cast<const JsonObject*>(nodeArrayValue->GetValue(i));
		THEBE_ASSERT_FATAL(nodeValue != nullptr);

		auto adjacencyArrayValue = dynamic_cast<const JsonArray*>(nodeValue->GetValue("adjacency_array"));
		if (!adjacencyArrayValue)
		{
			THEBE_LOG("No adjacency array value found.");
			return false;
		}

		auto node = this->nodeArray[i].Get();

		for (int j = 0; j < (int)adjacencyArrayValue->GetSize(); j++)
		{
			auto offsetValue = dynamic_cast<const JsonInt*>(adjacencyArrayValue->GetValue(j));
			if (!offsetValue)
			{
				THEBE_LOG("Expected entry %d of the adjacency array to be an integer.", j);
				return false;
			}

			int offset = (int)offsetValue->GetValue();
			if (offset < 0 || offset >= (int)this->nodeArray.size())
			{
				THEBE_LOG("Node %d points to adjacent node at offset %d, but that offset is out of range (%d)", i, offset, (int)this->nodeArray.size());
				return false;
			}

			node->adjacentNodeArray.push_back(this->nodeArray[offset].Get());
		}
	}

	return true;
}

/*static*/ ChineseCheckersGame* ChineseCheckersGame::Factory(const char* gameType)
{
	if (0 == ::strcmp(gameType, "cubic"))
		return new CubicGame();
	else if (0 == ::strcmp(gameType, "hexagonal"))
		return new HexagonalGame();
	else if (0 == ::strcmp(gameType, "octagonal"))
		return new OctagonalGame();

	return nullptr;
}

const std::vector<Thebe::Reference<ChineseCheckersGame::Node>>& ChineseCheckersGame::GetNodeArray() const
{
	return this->nodeArray;
}

//---------------------------------- ChineseCheckersGame::Occupant ----------------------------------

ChineseCheckersGame::Occupant::Occupant()
{
	this->playerID = 0;
	this->targetZoneID = 0;
	this->health = 1.0;
	this->attackPower = 0.0;
}

/*virtual*/ ChineseCheckersGame::Occupant::~Occupant()
{
}

//---------------------------------- ChineseCheckersGame::Node ----------------------------------

ChineseCheckersGame::Node::Node()
{
	this->occupant = nullptr;
	this->zoneID = 0;
	this->occupant = nullptr;
}

/*virtual*/ ChineseCheckersGame::Node::~Node()
{
}

void ChineseCheckersGame::Node::RemoveNullAdjacencies()
{
	std::vector<Node*> nodeArray;
	for (auto node : this->adjacentNodeArray)
		if (node)
			nodeArray.push_back(node);

	this->adjacentNodeArray = nodeArray;
}