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

/*virtual*/ std::shared_ptr<Graph> Factory::CreateGraph()
{
	return std::make_shared<Graph>();
}

/*virtual*/ std::shared_ptr<Node> Factory::CreateNode(const Vector& location, Marble::Color color)
{
	return std::make_shared<Node>(location, color);
}

/*virtual*/ std::shared_ptr<Marble> Factory::CreateMarble(Marble::Color color)
{
	return std::make_shared<Marble>(color);
}