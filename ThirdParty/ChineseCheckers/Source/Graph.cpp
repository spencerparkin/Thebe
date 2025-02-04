#include "Graph.h"
#include <stdlib.h>

using namespace ChineseCheckers;

//--------------------------------------- Graph ---------------------------------------

Graph::Graph()
{
}

/*virtual*/ Graph::~Graph()
{
}

void Graph::Clear()
{
	this->nodeArray.clear();
}

void Graph::AddNode(std::shared_ptr<Node> node)
{
	this->nodeArray.push_back(node);
}

std::shared_ptr<Graph> Graph::Clone() const
{
	std::shared_ptr<Graph> graph;
	//...
	return graph;
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

bool Graph::AllMarblesAtTarget(Marble::Color marbleColor) const
{
	auto pair = this->colorMap.find(marbleColor);
	if (pair == this->colorMap.end())
		return false;

	Marble::Color targetColor = pair->second;

	for (std::shared_ptr<Node> node : this->nodeArray)
	{
		std::shared_ptr<Marble> occupant = node->GetOccupant();
		if (occupant->GetColor() != marbleColor)
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

	std::shared_ptr<Marble> marble = move.sourceNode->GetOccupant();
	if (!marble)
		return false;

	if (move.targetNode->GetOccupant())
		return false;

	move.targetNode->SetOccupant(marble);
	return true;
}

bool Graph::FindBestMoves(Marble::Color marbleColor, BestMovesCollection& bestMovesCollection, const Vector& generalDirection) const
{
	for (std::shared_ptr<Node> node : this->nodeArray)
	{
		std::shared_ptr<Marble> occupant = node->GetOccupant();
		if (occupant->GetColor() != marbleColor)
			continue;
		
		for (int i = 0; i < (int)node->GetNumAdjacentNodes(); i++)
		{
			std::shared_ptr<Node> adjacentNode;
			if (!node->GetAdjacentNode(i, adjacentNode))
				continue;

			if (!adjacentNode->GetOccupant())
				continue;

			bestMovesCollection.AddMove({ node, adjacentNode }, generalDirection);
		}

		std::vector<const Node*> nodeStack;
		node->ForAllJumps(nodeStack, [&node, &bestMovesCollection, &generalDirection](std::shared_ptr<Node> targetNode) {
			bestMovesCollection.AddMove({ node, targetNode }, generalDirection);
		});
	}

	return true;
}

const std::vector<std::shared_ptr<Node>>& Graph::GetNodeArray() const
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

	for (const std::shared_ptr<Node>& node : this->nodeArray)
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

	for (const std::shared_ptr<Node>& node : this->nodeArray)
	{
		std::shared_ptr<Marble> occupant = node->GetOccupant();
		if (occupant->GetColor() == color)
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
}

/*virtual*/ Node::~Node()
{
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

std::shared_ptr<Marble> Node::GetOccupant()
{
	return this->occupant;
}

const std::shared_ptr<Marble> Node::GetOccupant() const
{
	return this->occupant;
}

void Node::SetOccupant(std::shared_ptr<Marble> marble)
{
	this->occupant = marble;
}

int Node::GetNumAdjacentNodes() const
{
	return (int)this->adjacentNodeArray.size();
}

void Node::AddAjacentNode(std::shared_ptr<Node> node)
{
	this->adjacentNodeArray.push_back(node);
}

bool Node::GetAdjacentNode(int i, std::shared_ptr<Node>& node) const
{
	if (i < 0 || i >= (int)this->adjacentNodeArray.size())
		return false;

	node = this->adjacentNodeArray[i].lock();
	if (!node)
		return false;

	return true;
}

bool Node::GetAdjacentNode(const Vector& givenDirection, std::shared_ptr<Node>& node) const
{
	double epsilon = 1e-4;

	for (int i = 0; i < (int)this->adjacentNodeArray.size(); i++)
	{
		std::shared_ptr<Node> adjacentNode = this->adjacentNodeArray[i].lock();
		if (!adjacentNode)
			continue;

		Vector direction = (adjacentNode->location - this->location).Normalized();
		double dot = direction.Dot(givenDirection);
		if (::abs(dot - 1.0) >= epsilon)
			continue;

		node = adjacentNode;
		return true;
	}

	return false;
}

bool Node::JumpInDirection(int i, std::shared_ptr<Node>& node) const
{
	if (i < 0 || i >= (int)this->adjacentNodeArray.size())
		return false;

	std::shared_ptr<Node> adjacentNode = this->adjacentNodeArray[i].lock();
	if (!adjacentNode)
		return false;

	if (!adjacentNode->GetOccupant())
		return false;

	Vector direction = (adjacentNode->location - this->location).Normalized();
	if (!adjacentNode->GetAdjacentNode(direction, node))
		return false;

	if (node->GetOccupant())
		return false;

	return true;
}

void Node::ForAllJumps(std::vector<const Node*>& nodeStack, std::function<void(std::shared_ptr<Node>)> callback) const
{
	nodeStack.push_back(this);

	for (int i = 0; i < (int)this->adjacentNodeArray.size(); i++)
	{
		std::shared_ptr<Node> jumpNode;
		if (!this->JumpInDirection(i, jumpNode))
			continue;

		bool alreadyVisited = false;
		for (int j = 0; j < (int)nodeStack.size() && !alreadyVisited; j++)
			if (nodeStack[j] == jumpNode.get())
				alreadyVisited = true;

		if (alreadyVisited)
			continue;

		callback(jumpNode);

		jumpNode->ForAllJumps(nodeStack, callback);
	}

	nodeStack.pop_back();
}

void Node::RemoveNullAdjacencies()
{
	int i = 0;
	while (i < (int)this->adjacentNodeArray.size())
	{
		std::shared_ptr<Node> adjacentNode = this->adjacentNodeArray[i].lock();
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