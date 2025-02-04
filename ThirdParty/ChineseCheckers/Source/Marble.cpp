#include "Marble.h"
#include "Graph.h"

using namespace ChineseCheckers;

Marble::Marble(Color color)
{
	this->color = color;
}

/*virtual*/ Marble::~Marble()
{
}

Marble::Color Marble::GetColor() const
{
	return this->color;
}

bool Marble::GetLocation(Vector& location) const
{
	std::shared_ptr<Node> node = this->occupyingNode.lock();
	if (!node)
		return false;

	location = node->GetLocation();
	return true;
}

void Marble::SetOccupyingNode(std::shared_ptr<Node> node)
{
	this->occupyingNode = node;
}