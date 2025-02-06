#include "ChineseCheckers/Factory.h"
#include "ChineseCheckers/Graph.h"
#include "ChineseCheckers/Marble.h"

using namespace ChineseCheckers;

Factory::Factory()
{
}

/*virtual*/ Factory::~Factory()
{
}

/*virtual*/ Graph* Factory::CreateGraph()
{
	return new Graph();
}

/*virtual*/ Node* Factory::CreateNode(const Vector& location, Marble::Color color)
{
	return new Node(location, color);
}

/*virtual*/ Marble* Factory::CreateMarble(Marble::Color color)
{
	return new Marble(color);
}