#include "Factory.h"


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
}

/*virtual*/ bool Graph::MoveMarbleConditionally(const ChineseCheckers::MoveSequence& moveSequence)
{
	if (!ChineseCheckers::Graph::MoveMarbleConditionally(moveSequence))
		return false;

	// TODO: This is where the "extreme" rules come into play.  Here, a marble
	//       can take damage and deal damage.  If a marble dies, then it has
	//       to be set back somehow.

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
	this->health = 1.0;
	this->collisionObjectHandle = THEBE_INVALID_REF_HANDLE;
}

/*virtual*/ Marble::~Marble()
{
}