#include "Factory.h"
#include "ChineseCheckers/MoveSequence.h"

//------------------------------------ Factory ------------------------------------

Factory::Factory()
{
}

/*virtual*/ Factory::~Factory()
{
}

/*virtual*/ ChineseCheckers::Graph* Factory::CreateGraph()
{
	return new Graph();
}

/*virtual*/ ChineseCheckers::Node* Factory::CreateNode(const ChineseCheckers::Vector& location, ChineseCheckers::Marble::Color color)
{
	return new Node(location, color);
}

/*virtual*/ ChineseCheckers::Marble* Factory::CreateMarble(ChineseCheckers::Marble::Color color)
{
	return new Marble(color);
}

//------------------------------------ Graph ------------------------------------

Graph::Graph()
{
}

/*virtual*/ Graph::~Graph()
{
	this->Clear();
}

/*virtual*/ void Graph::Clear()
{
	ChineseCheckers::Graph::Clear();

	for (Marble* marble : this->deadMarbleArray)
		delete marble;

	this->deadMarbleArray.clear();
}

/*virtual*/ bool Graph::MoveMarbleConditionally(const ChineseCheckers::MoveSequence& moveSequence)
{
	if (!this->IsValidMoveSequence(moveSequence))
		return false;

	auto marble = (Marble*)this->nodeArray[moveSequence.nodeIndexArray[0]]->GetOccupant();
	THEBE_ASSERT_FATAL(marble != nullptr);

	if (!ChineseCheckers::Graph::MoveMarbleConditionally(moveSequence))
		return false;

	for (int i = 0; i < (int)moveSequence.nodeIndexArray.size() - 1; i++)
	{
		auto nodeA = (Node*)this->nodeArray[moveSequence.nodeIndexArray[i]];
		auto nodeB = (Node*)this->nodeArray[moveSequence.nodeIndexArray[i + 1]];
		auto hoppedNode = (Node*)Node::FindMutualAdjacency(nodeA, nodeB);
		if (hoppedNode)
		{
			auto hoppedMarble = (Marble*)hoppedNode->GetOccupant();
			THEBE_ASSERT_FATAL(hoppedMarble != nullptr);
			if (hoppedMarble->GetColor() != marble->GetColor() && --hoppedMarble->numLives == 0)
			{
				hoppedNode->SetOccupant(nullptr);
				this->deadMarbleArray.push_back(hoppedMarble);
			}
		}
	}

	return true;
}

//------------------------------------ Node ------------------------------------

Node::Node(const ChineseCheckers::Vector& location, ChineseCheckers::Marble::Color color) : ChineseCheckers::Node(location, color)
{
}

/*virtual*/ Node::~Node()
{
}

Thebe::Vector3 Node::GetLocation3D() const
{
	return Thebe::Vector3(this->location.x, 0.0, -this->location.y);
}

//------------------------------------ Marble ------------------------------------

Marble::Marble(Color color) : ChineseCheckers::Marble(color)
{
	this->numLives = 3;
	this->collisionObjectHandle = THEBE_INVALID_REF_HANDLE;
}

/*virtual*/ Marble::~Marble()
{
}