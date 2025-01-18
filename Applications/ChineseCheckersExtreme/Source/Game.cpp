#include "Game.h"
#include "GameShapes/CubicGame.h"
#include "GameShapes/HexagonalGame.h"
#include "GameShapes/OctagonalGame.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/Log.h"

using namespace Thebe;

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
		occupantValue->SetValue("source_zone_id", new JsonInt(occupant->sourceZoneID));
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

		auto sourceZoneIDValue = dynamic_cast<const JsonInt*>(occupantValue->GetValue("source_zone_id"));
		auto targetZoneIDValue = dynamic_cast<const JsonInt*>(occupantValue->GetValue("target_zone_id"));
		auto healthValue = dynamic_cast<const JsonFloat*>(occupantValue->GetValue("health"));
		auto attackPowerValue = dynamic_cast<const JsonFloat*>(occupantValue->GetValue("attack_power"));

		if (!sourceZoneIDValue || !targetZoneIDValue || !healthValue || !attackPowerValue)
		{
			THEBE_LOG("Not all values could be found for occupant entry %d.", i);
			return false;
		}

		auto occupant = new Occupant();
		occupant->sourceZoneID = (int)sourceZoneIDValue->GetValue();
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

const std::vector<Thebe::Reference<ChineseCheckersGame::Occupant>>& ChineseCheckersGame::GetOccupantArray() const
{
	return this->occupantArray;
}

bool ChineseCheckersGame::FindLegalPath(Node* sourceNode, Node* targetNode, std::vector<Node*>& nodePathArray)
{
	nodePathArray.clear();

	for (Node* adjacentNode : sourceNode->adjacentNodeArray)
	{
		if (adjacentNode == targetNode)
		{
			nodePathArray.push_back(sourceNode);
			nodePathArray.push_back(targetNode);
			return true;
		}
	}

	// Note that we must use a BFS here (instead of a DFS), because
	// we need to return the shortest possible path.  This is so that
	// the user can reasonably string a sequence of shortests paths
	// together to get the desired sequence.

	sourceNode->parentNode = nullptr;
	std::set<Node*> nodeSet;
	nodeSet.insert(sourceNode);
	std::list<Node*> nodeQueue;
	nodeQueue.push_back(sourceNode);
	bool pathFound = false;
	while (nodeQueue.size() > 0)
	{
		Node* node = *nodeQueue.begin();
		nodeQueue.pop_front();
		
		if (node == targetNode)
		{
			pathFound = true;
			break;
		}

		for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
		{
			Vector3 unitDirection;
			Node* adjacentNode = node->GetAdjacencyAndDirection(i, unitDirection);
			if (!adjacentNode->occupant)
				continue;

			Node* hopNode = adjacentNode->GetAdjacencyInDirection(unitDirection);
			if (!hopNode || hopNode->occupant)
				continue;

			if (nodeSet.find(hopNode) != nodeSet.end())
				continue;

			hopNode->parentNode = node;
			nodeQueue.push_back(hopNode);
			nodeSet.insert(hopNode);
		}
	}

	if (!pathFound)
		return false;

	std::list<Node*> nodePathList;
	for (Node* node = targetNode; node != nullptr; node = node->parentNode)
		nodePathList.push_front(node);

	for (Node* node : nodePathList)
		nodePathArray.push_back(node);

	for (Node* node : this->nodeArray)
		node->parentNode = nullptr;

	return true;
}

bool ChineseCheckersGame::IsPathLegal(const std::vector<Node*>& nodePathArray, std::vector<Node*>* hoppedNodesArray /*= nullptr*/)
{
	if (hoppedNodesArray)
		hoppedNodesArray->clear();

	if (nodePathArray.size() < 2)
		return false;

	if (!nodePathArray[0]->occupant)
		return false;

	for (int i = 1; i < (int)nodePathArray.size(); i++)
		if (nodePathArray[i]->occupant)
			return false;

	std::set<Node*> nodeSet;
	for (Node* node : nodePathArray)
	{
		if (nodeSet.find(node) != nodeSet.end())
			return false;

		nodeSet.insert(node);
	}

	if (nodePathArray.size() == 2 && nodePathArray[0]->IsAdjacentTo(nodePathArray[1]))
		return true;
	
	for (int i = 0; i < (int)nodePathArray.size() - 1; i++)
	{
		Node* nodeA = nodePathArray[i];
		Node* nodeB = nodePathArray[i + 1];

		Vector3 unitDirection = (nodeB->location - nodeA->location).Normalized();
		Node* adjacentNode = nodeA->GetAdjacencyInDirection(unitDirection);
		if (!adjacentNode)
			return false;

		if (!adjacentNode->occupant)
			return false;

		if (hoppedNodesArray)
			hoppedNodesArray->push_back(adjacentNode);

		adjacentNode = adjacentNode->GetAdjacencyInDirection(unitDirection);
		if (!adjacentNode)
			return false;

		if (adjacentNode != nodeB)
			return false;
	}

	return true;
}

bool ChineseCheckersGame::ExecutePath(const std::vector<Node*>& nodePathArray)
{
	std::vector<Node*> hoppedNodesArray;
	if (!this->IsPathLegal(nodePathArray, &hoppedNodesArray))
		return false;

	Node* sourceNode = nodePathArray[0];
	Node* targetNode = nodePathArray[nodePathArray.size() - 1];

	targetNode->occupant = sourceNode->occupant;
	sourceNode->occupant = nullptr;

	// This is where the traditional game of Chinese Checkers would end a turn,
	// but this is the extreme version.  What remains is to mutate the attack
	// power of the moved occupant and deal damage to opposing occupants.

	for (Node* hoppedNode : hoppedNodesArray)
	{
		if (hoppedNode->occupant->sourceZoneID == targetNode->occupant->sourceZoneID)
			targetNode->occupant->attackPower += 0.25;
		else
		{
			double attackPower = THEBE_MIN(hoppedNode->occupant->health, targetNode->occupant->attackPower);
			hoppedNode->occupant->health -= attackPower;
			targetNode->occupant->attackPower -= attackPower;

			// TODO: If a hopped node dies here, where do we move it?
		}
	}

	return true;
}

int ChineseCheckersGame::NodeToOffset(Node* node)
{
	for (int i = 0; i < (int)this->nodeArray.size(); i++)
		if (this->nodeArray[i] == node)
			return i;

	return -1;
}

ChineseCheckersGame::Node* ChineseCheckersGame::NodeFromOffset(int offset)
{
	if (offset < 0 || offset >= (int)this->nodeArray.size())
		return nullptr;

	return this->nodeArray[offset];
}

bool ChineseCheckersGame::NodeArrayToOffsetArray(const std::vector<Node*>& nodePathArray, std::vector<int>& nodeOffsetArray)
{
	nodeOffsetArray.clear();

	for (Node* node : nodePathArray)
	{
		int i = this->NodeToOffset(node);
		if (i < 0)
			return false;

		nodeOffsetArray.push_back(i);
	}

	return true;
}

bool ChineseCheckersGame::NodeArrayFromOffsetArray(std::vector<Node*>& nodePathArray, const std::vector<int>& nodeOffsetArray)
{
	nodePathArray.clear();

	for (int i : nodeOffsetArray)
	{
		Node* node = this->NodeFromOffset(i);
		if (!node)
			return false;

		nodePathArray.push_back(node);
	}

	return true;
}

int ChineseCheckersGame::GetNextZone(int zoneID)
{
	int maxPlayers = this->GetMaxPossiblePlayers();

	for (int i = 0; i < maxPlayers; i++)
	{
		if (++zoneID >= maxPlayers)
			zoneID = 1;

		if (this->IsZoneBeingUsed(zoneID))
			return zoneID;
	}

	return 0;
}

bool ChineseCheckersGame::IsZoneBeingUsed(int zoneID)
{
	for (Occupant* occupant : this->occupantArray)
		if (occupant->sourceZoneID == zoneID)
			return true;

	return false;
}

int ChineseCheckersGame::GetNumActivePlayers()
{
	std::set<int> zoneSet;
	for (Occupant* occupant : this->occupantArray)
		if (zoneSet.find(occupant->sourceZoneID) == zoneSet.end())
			zoneSet.insert(occupant->sourceZoneID);

	return (int)zoneSet.size();
}

//---------------------------------- ChineseCheckersGame::Occupant ----------------------------------

ChineseCheckersGame::Occupant::Occupant()
{
	this->sourceZoneID = 0;
	this->targetZoneID = 0;
	this->health = 1.0;
	this->attackPower = 0.0;
	this->collisionObjectHandle = THEBE_INVALID_REF_HANDLE;
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
	this->parentNode = nullptr;
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

ChineseCheckersGame::Node* ChineseCheckersGame::Node::GetAdjacencyAndDirection(int i, Vector3& unitDirection)
{
	if (i < 0 || i >= (int)this->adjacentNodeArray.size())
		return nullptr;

	Node* node = this->adjacentNodeArray[i];
	unitDirection = (node->location - this->location).Normalized();
	return node;
}

ChineseCheckersGame::Node* ChineseCheckersGame::Node::GetAdjacencyInDirection(const Vector3& unitDirection)
{
	for (Node* node : this->adjacentNodeArray)
	{
		Vector3 nodeUnitDirection = (node->location - this->location).Normalized();
		double angle = unitDirection.AngleBetween(nodeUnitDirection);
		if (angle < THEBE_MEDIUM_EPS)
		{
			return node;
		}
	}

	return nullptr;
}

bool ChineseCheckersGame::Node::FindWithHops(Node* targetNode, std::vector<Node*>& nodePathArray)
{
	nodePathArray.push_back(this);

	if (this == targetNode)
		return true;

	for (int i = 0; i < (int)this->adjacentNodeArray.size(); i++)
	{
		Vector3 unitDirection;
		Node* adjacentNode = this->GetAdjacencyAndDirection(i, unitDirection);
		if (!adjacentNode->occupant)
			continue;
		
		Node* hopNode = adjacentNode->GetAdjacencyInDirection(unitDirection);
		if (!hopNode)
			continue;

		bool alreadyTraveled = false;
		for (int j = 0; j < (int)nodePathArray.size() && !alreadyTraveled; j++)
			if (nodePathArray[j] == hopNode)
				alreadyTraveled = true;

		if (alreadyTraveled)
			continue;

		if (hopNode->FindWithHops(targetNode, nodePathArray))
			return true;
	}

	nodePathArray.pop_back();

	return false;
}

bool ChineseCheckersGame::Node::IsAdjacentTo(Node* node)
{
	for (Node* adjacentNode : this->adjacentNodeArray)
		if (adjacentNode == node)
			return true;

	return false;
}