#include "ChineseCheckers/Graph.h"
#include "ChineseCheckers/Factory.h"
#include <stdlib.h>

using namespace ChineseCheckers;

//--------------------------------------- Graph ---------------------------------------

Graph::Graph()
{
}

/*virtual*/ Graph::~Graph()
{
	this->Clear();
}

void Graph::Clear()
{
	for (Node* node : this->nodeArray)
		delete node;

	this->nodeArray.clear();
	this->colorMap.clear();
}

void Graph::AddNode(Node* node)
{
	this->nodeArray.push_back(node);
}

Graph* Graph::Clone(Factory* factory) const
{
	Graph* graph = factory->CreateGraph();
	if (graph)
	{
		std::unique_ptr<ParseParty::JsonValue> jsonValue;
		if (this->ToJson(jsonValue))
			graph->FromJson(jsonValue.get(), factory);
	}

	return graph;
}

void Graph::MakeOffsetMap(std::map<Node*, int>& offsetMap) const
{
	offsetMap.clear();
	int offset = 0;
	for (Node* node : this->nodeArray)
		offsetMap.insert(std::pair(node, offset++));
}

/*virtual*/ bool Graph::ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const
{
	using namespace ParseParty;

	auto graphValue = new JsonObject();
	jsonValue.reset(graphValue);

	auto nodeArrayValue = new JsonArray();
	graphValue->SetValue("node_array", nodeArrayValue);

	std::map<Node*, int> offsetMap;
	this->MakeOffsetMap(offsetMap);

	for (Node* node : this->nodeArray)
	{
		std::unique_ptr<JsonValue> nodeValue;
		if (!node->ToJson(nodeValue, offsetMap))
			return false;

		nodeArrayValue->PushValue(nodeValue.release());
	}

	auto colorMapArrayValue = new JsonArray();
	graphValue->SetValue("color_map_array", colorMapArrayValue);

	for (auto pair : this->colorMap)
	{
		auto colorPairArrayValue = new JsonArray();
		colorPairArrayValue->PushValue(new JsonInt(pair.first));
		colorPairArrayValue->PushValue(new JsonInt(pair.second));
		colorMapArrayValue->PushValue(colorPairArrayValue);
	}

	return true;
}

/*virtual*/ bool Graph::FromJson(const ParseParty::JsonValue* jsonValue, Factory* factory)
{
	using namespace ParseParty;

	this->Clear();

	auto graphValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!graphValue)
		return false;

	auto nodeArrayValue = dynamic_cast<const JsonArray*>(graphValue->GetValue("node_array"));
	if (!nodeArrayValue)
		return false;

	for (int i = 0; i < (int)nodeArrayValue->GetSize(); i++)
	{
		Node* node = factory->CreateNode(Vector(0.0, 0.0), Marble::Color::NONE);
		if (!node)
			return false;

		this->nodeArray.push_back(node);
	}

	for (int i = 0; i < (int)nodeArrayValue->GetSize(); i++)
	{
		auto nodeValue = dynamic_cast<const JsonObject*>(nodeArrayValue->GetValue(i));
		if (!nodeValue)
			return false;

		if (!this->nodeArray[i]->FromJson(nodeValue, this->nodeArray, factory))
			return false;
	}

	auto colorMapArrayValue = dynamic_cast<const JsonArray*>(graphValue->GetValue("color_map_array"));
	if (!colorMapArrayValue)
		return false;

	for (int i = 0; i < (int)colorMapArrayValue->GetSize(); i++)
	{
		auto colorPairArrayValue = dynamic_cast<const JsonArray*>(colorMapArrayValue->GetValue(i));
		if (!colorPairArrayValue)
			return false;

		auto firstValue = dynamic_cast<const JsonInt*>(colorPairArrayValue->GetValue(0));
		auto secondValue = dynamic_cast<const JsonInt*>(colorPairArrayValue->GetValue(1));
		if (!firstValue || !secondValue)
			return false;

		this->colorMap.insert(std::pair(Marble::Color(firstValue->GetValue()), Marble::Color(secondValue->GetValue())));
	}

	return true;
}

bool Graph::SetColorTarget(Marble::Color sourceColor, Marble::Color targetColor)
{
	if (sourceColor == targetColor)
		return false;

	if (this->colorMap.find(sourceColor) != this->colorMap.end())
		this->colorMap.erase(sourceColor);

	this->colorMap.insert(std::pair(sourceColor, targetColor));
	return true;
}

bool Graph::IsValidMoveSequence(const std::vector<int>& moveSequence) const
{
	if (moveSequence.size() < 2)
		return false;

	for (int i : moveSequence)
		if (i < 0 || i >= (int)this->nodeArray.size())
			return false;

	if (!this->nodeArray[moveSequence[0]]->GetOccupant())
		return false;
	
	for (int i = 1; i < (int)moveSequence.size(); i++)
		if (this->nodeArray[moveSequence[i]]->GetOccupant())
			return false;

	const Node* nodeA = this->nodeArray[moveSequence[0]];
	const Node* nodeB = this->nodeArray[moveSequence[1]];
	if (nodeA->IsAdjacentTo(nodeB))
		return moveSequence.size() == 2;

	std::set<int> offsetSet;
	for (int i : moveSequence)
	{
		if (offsetSet.find(i) != offsetSet.end())
			return false;

		offsetSet.insert(i);
	}

	for (int i = 0; i < (int)moveSequence.size() - 1; i++)
	{
		nodeA = this->nodeArray[moveSequence[i]];
		nodeB = this->nodeArray[moveSequence[i + 1]];
		const Node* mutualAdjacency = Node::FindMutualAdjacency(nodeA, nodeB);
		if (!mutualAdjacency || !mutualAdjacency->GetOccupant())
			return false;
	}

	return true;
}

bool Graph::AllMarblesAtTarget(Marble::Color marbleColor) const
{
	auto pair = this->colorMap.find(marbleColor);
	if (pair == this->colorMap.end())
		return false;

	Marble::Color targetColor = pair->second;

	for (Node* node : this->nodeArray)
	{
		Marble* occupant = node->GetOccupant();
		if (!occupant || occupant->GetColor() != marbleColor)
			continue;
		
		if (node->GetColor() != targetColor)
			return false;
	}

	return true;
}

bool Graph::MoveMarbleUnconditionally(const Move& move)
{
	if (!move.sourceNode || !move.targetNode)
		return false;

	Marble* marble = move.sourceNode->GetOccupant();
	if (!marble)
		return false;

	if (move.targetNode->GetOccupant())
		return false;

	move.targetNode->SetOccupant(marble);
	move.sourceNode->SetOccupant(nullptr);
	return true;
}

bool Graph::FindBestMoves(Marble::Color marbleColor, BestMovesCollection& bestMovesCollection, const Vector& generalDirection) const
{
	for (Node* node : this->nodeArray)
	{
		Marble* occupant = node->GetOccupant();
		if (!occupant || occupant->GetColor() != marbleColor)
			continue;
		
		for (int i = 0; i < (int)node->GetNumAdjacentNodes(); i++)
		{
			Node* adjacentNode = node->GetAdjacentNode(i);
			if (!adjacentNode || adjacentNode->GetOccupant())
				continue;

			bestMovesCollection.AddMove({ node, adjacentNode }, generalDirection);
		}

		std::vector<const Node*> nodeStack;
		node->ForAllJumpsDFS(nodeStack, [&node, &bestMovesCollection, &generalDirection](Node* targetNode) -> bool
			{
				bestMovesCollection.AddMove({ node, targetNode }, generalDirection);
				return true;
			});
	}

	return true;
}

const std::vector<Node*>& Graph::GetNodeArray() const
{
	return this->nodeArray;
}

bool Graph::CalcGeneralDirection(Marble::Color color, Vector& generalDirection) const
{
	auto pair = this->colorMap.find(color);
	if (pair == this->colorMap.end())
		return false;

	Marble::Color targetColor = pair->second;

	generalDirection = this->CalcZoneCentroid(targetColor) - this->CalcMarbleCentroid(color);
	double length = generalDirection.Length();
	if (length == 0.0)
		return false;
	
	generalDirection /= length;
	return true;
}

Vector Graph::CalcZoneCentroid(Marble::Color color) const
{
	Vector center(0.0, 0.0);
	int numNodes = 0;

	for (const Node* node : this->nodeArray)
	{
		if (node->GetColor() == color)
		{
			numNodes++;
			center += node->GetLocation();
		}
	}

	if (numNodes > 0)
		center /= double(numNodes);

	return center;
}

Vector Graph::CalcMarbleCentroid(Marble::Color color) const
{
	Vector center(0.0, 0.0);
	int numMarbles = 0;

	for (const Node* node : this->nodeArray)
	{
		const Marble* occupant = node->GetOccupant();
		if (occupant && occupant->GetColor() == color)
		{
			numMarbles++;
			center += node->GetLocation();
		}
	}

	if (numMarbles > 0)
		center /= double(numMarbles);

	return center;
}

//--------------------------------------- Graph::Move ---------------------------------------

double Graph::Move::CalcDistanceInDirection(const Vector& unitLengthDirection) const
{
	if (!this->targetNode || !this->sourceNode)
		return 0.0;

	Vector delta = this->targetNode->GetLocation() - this->sourceNode->GetLocation();
	return delta.Dot(unitLengthDirection);
}

//--------------------------------------- Graph::BestMovesCollection ---------------------------------------

Graph::BestMovesCollection::BestMovesCollection()
{
	this->longestDistance = 0.0;
}

void Graph::BestMovesCollection::AddMove(const Move& move, const Vector& unitLengthDirection)
{
	double distance = move.CalcDistanceInDirection(unitLengthDirection);
	if (distance >= this->longestDistance)
	{
		if (distance > this->longestDistance)
		{
			this->longestDistance = distance;
			this->moveArray.clear();
		}

		this->moveArray.push_back(move);
	}
}

bool Graph::BestMovesCollection::GetRandom(Move& move) const
{
	if (this->moveArray.size() == 0)
		return false;

	int i = (double(rand()) / double(RAND_MAX)) * double(this->moveArray.size());
	if (i < 0)
		i = 0;
	if (i >= (int)this->moveArray.size())
		i = (int)this->moveArray.size() - 1;

	move = this->moveArray[i];
	return true;
}

//--------------------------------------- Node ---------------------------------------

Node::Node(const Vector& location, Marble::Color color)
{
	this->location = location;
	this->color = color;
	this->occupant = nullptr;
}

/*virtual*/ Node::~Node()
{
	delete this->occupant;
}

/*static*/ const Node* Node::FindMutualAdjacency(const Node* nodeA, const Node* nodeB)
{
	if (nodeA->IsAdjacentTo(nodeB) || nodeB->IsAdjacentTo(nodeA))
		return nullptr;

	for (const Node* adjacentNodeA : nodeA->adjacentNodeArray)
		for (const Node* adjacentNodeB : nodeB->adjacentNodeArray)
			if (adjacentNodeA == adjacentNodeB)
				return adjacentNodeA;

	return nullptr;
}

/*virtual*/ bool Node::ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::map<Node*, int>& offsetMap) const
{
	using namespace ParseParty;

	auto nodeObjectValue = new JsonObject();
	jsonValue.reset(nodeObjectValue);

	nodeObjectValue->SetValue("color", new JsonInt(this->color));
	nodeObjectValue->SetValue("location_x", new JsonFloat(this->location.x));
	nodeObjectValue->SetValue("location_y", new JsonFloat(this->location.y));
	
	if (this->occupant)
	{
		std::unique_ptr<JsonValue> occupantValue;
		if (!this->occupant->ToJson(occupantValue))
			return false;

		nodeObjectValue->SetValue("occupant", occupantValue.release());
	}

	auto adjacencyArrayValue = new JsonArray();
	nodeObjectValue->SetValue("adjacency_array", adjacencyArrayValue);

	for (int i = 0; i < (int)this->adjacentNodeArray.size(); i++)
	{
		Node* adjacentNode(this->adjacentNodeArray[i]);
		if (adjacentNode)
		{
			int j = offsetMap.find(adjacentNode)->second;
			adjacencyArrayValue->PushValue(new JsonInt(j));
		}
	}

	return true;
}

/*virtual*/ bool Node::FromJson(const ParseParty::JsonValue* jsonValue, const std::vector<Node*>& nodeArray, Factory* factory)
{
	using namespace ParseParty;

	auto nodeObject = dynamic_cast<const JsonObject*>(jsonValue);
	if (!nodeObject)
		return false;

	auto colorValue = dynamic_cast<const JsonInt*>(nodeObject->GetValue("color"));
	if (!colorValue)
		return false;

	this->color = (Marble::Color)colorValue->GetValue();

	auto xLocationValue = dynamic_cast<const JsonFloat*>(nodeObject->GetValue("location_x"));
	auto yLocationValue = dynamic_cast<const JsonFloat*>(nodeObject->GetValue("location_y"));
	if (!xLocationValue || !yLocationValue)
		return false;

	this->location.x = xLocationValue->GetValue();
	this->location.y = yLocationValue->GetValue();

	auto occupantValue = dynamic_cast<const JsonObject*>(nodeObject->GetValue("occupant"));
	if (occupantValue)
	{
		this->occupant = factory->CreateMarble(Marble::Color::NONE);
		if (!this->occupant)
			return false;

		if (!this->occupant->FromJson(occupantValue))
			return false;
	}

	auto adjacencyArrayValue = dynamic_cast<const JsonArray*>(nodeObject->GetValue("adjacency_array"));
	if (!adjacencyArrayValue)
		return false;

	this->adjacentNodeArray.clear();
	for (int i = 0; i < (int)adjacencyArrayValue->GetSize(); i++)
	{
		auto offsetValue = dynamic_cast<const JsonInt*>(adjacencyArrayValue->GetValue(i));
		if (!offsetValue)
			return false;

		int j = (int)offsetValue->GetValue();
		if (j < 0 || j >= (int)nodeArray.size())
			return false;

		this->adjacentNodeArray.push_back(nodeArray[j]);
	}

	return true;
}

void Node::SetLocation(const Vector& location)
{
	this->location = location;
}

const Vector& Node::GetLocation() const
{
	return this->location;
}

void Node::SetColor(Marble::Color color)
{
	this->color = color;
}

Marble::Color Node::GetColor() const
{
	return this->color;
}

Marble* Node::GetOccupant()
{
	return this->occupant;
}

const Marble* Node::GetOccupant() const
{
	return this->occupant;
}

void Node::SetOccupant(Marble* marble)
{
	this->occupant = marble;
}

int Node::GetNumAdjacentNodes() const
{
	return (int)this->adjacentNodeArray.size();
}

void Node::AddAjacentNode(Node* node)
{
	this->adjacentNodeArray.push_back(node);
}

Node* Node::GetAdjacentNode(int i) const
{
	if (i < 0 || i >= (int)this->adjacentNodeArray.size())
		return nullptr;

	return this->adjacentNodeArray[i];
}

Node* Node::GetAdjacentNode(const Vector& givenDirection) const
{
	double epsilon = 1e-4;

	for (int i = 0; i < (int)this->adjacentNodeArray.size(); i++)
	{
		Node* adjacentNode = this->adjacentNodeArray[i];
		if (!adjacentNode)
			continue;

		Vector direction = (adjacentNode->location - this->location).Normalized();
		double dot = direction.Dot(givenDirection);
		if (::abs(dot - 1.0) >= epsilon)
			continue;

		return adjacentNode;
	}

	return nullptr;
}

Node* Node::JumpInDirection(int i) const
{
	if (i < 0 || i >= (int)this->adjacentNodeArray.size())
		return nullptr;

	Node* adjacentNodeA = this->adjacentNodeArray[i];
	if (!adjacentNodeA || !adjacentNodeA->GetOccupant())
		return nullptr;

	Vector direction = (adjacentNodeA->location - this->location).Normalized();
	Node* adjacentNodeB = adjacentNodeA->GetAdjacentNode(direction);
	if (!adjacentNodeB || adjacentNodeB->GetOccupant())
		return nullptr;

	return adjacentNodeB;
}

bool Node::IsAdjacentTo(const Node* node) const
{
	for (const Node* adjacentNode : this->adjacentNodeArray)
		if (adjacentNode == node)
			return true;

	return false;
}

bool Node::ForAllJumpsDFS(std::vector<const Node*>& nodeStack, std::function<bool(Node*)> callback) const
{
	bool keepGoing = true;

	nodeStack.push_back(this);

	for (int i = 0; i < (int)this->adjacentNodeArray.size(); i++)
	{
		Node* jumpNode = this->JumpInDirection(i);
		if (!jumpNode)
			continue;

		bool alreadyVisited = false;
		for (int j = 0; j < (int)nodeStack.size() && !alreadyVisited; j++)
			if (nodeStack[j] == jumpNode)
				alreadyVisited = true;

		if (alreadyVisited)
			continue;

		keepGoing = callback(jumpNode);
		if (!keepGoing)
			break;

		keepGoing = jumpNode->ForAllJumpsDFS(nodeStack, callback);
		if (!keepGoing)
			break;
	}

	nodeStack.pop_back();

	return keepGoing;
}

bool Node::ForAllJumpsBFS(std::function<bool(Node*, const std::map<Node*, Node*>&)> callback) const
{
	std::map<Node*, Node*> parentMap;
	parentMap.insert(std::pair(const_cast<Node*>(this), nullptr));

	std::list<Node*> queue;
	queue.push_back(const_cast<Node*>(this));

	while (queue.size() > 0)
	{
		Node* node = queue.front();
		queue.pop_front();

		for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
		{
			Node* jumpNode = node->JumpInDirection(i);
			if (!jumpNode)
				continue;

			if (parentMap.find(jumpNode) != parentMap.end())
				continue;

			parentMap.insert(std::pair(jumpNode, node));

			if (!callback(jumpNode, parentMap))
				return false;

			queue.push_back(jumpNode);
		}
	}

	return true;
}

void Node::RemoveNullAdjacencies()
{
	int i = 0;
	while (i < (int)this->adjacentNodeArray.size())
	{
		Node* adjacentNode = this->adjacentNodeArray[i];
		if (adjacentNode)
			i++;
		else
		{
			int j = (int)this->adjacentNodeArray.size() - 1;
			if (i != j)
				this->adjacentNodeArray[i] = this->adjacentNodeArray[j];

			this->adjacentNodeArray.pop_back();
		}
	}
}