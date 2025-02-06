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
	return new ChineseCheckers::Graph();
}

/*virtual*/ ChineseCheckers::Node* Factory::CreateNode(const ChineseCheckers::Vector& location, ChineseCheckers::Marble::Color color)
{
	return new Node(location, color);
}

/*virtual*/ ChineseCheckers::Marble* Factory::CreateMarble(ChineseCheckers::Marble::Color color)
{
	return new Marble(color);
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