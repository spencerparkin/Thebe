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

/*virtual*/ std::shared_ptr<ChineseCheckers::Marble> Factory::CreateMarble(ChineseCheckers::Marble::Color color)
{
	return std::make_shared<Marble>(color);
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

	this->deadMarbleArray.clear();
}

/*virtual*/ bool Graph::MoveMarbleConditionally(const ChineseCheckers::MoveSequence& moveSequence)
{
	if (!this->IsValidMoveSequence(moveSequence))
		return false;

	std::shared_ptr<ChineseCheckers::Marble> nativeMarble = this->nodeArray[moveSequence.nodeIndexArray[0]]->GetOccupant();
	Marble* marble = dynamic_cast<Marble*>(nativeMarble.get());
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
			std::shared_ptr<ChineseCheckers::Marble> nativeHoppedMarble = hoppedNode->GetOccupant();
			auto hoppedMarble = dynamic_cast<Marble*>(nativeHoppedMarble.get());
			THEBE_ASSERT_FATAL(hoppedMarble != nullptr);	// TODO: I've seen the program crash here due to there being no occupant in the hopped node.  How did this happen?
			if (hoppedMarble->GetColor() != marble->GetColor() && --hoppedMarble->numLives == 0)
			{
				hoppedNode->SetOccupant(nullptr);
				this->deadMarbleArray.push_back(nativeHoppedMarble);
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