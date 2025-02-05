#include "Factory.h"
#include "Graph.h"
#include "Marble.h"

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