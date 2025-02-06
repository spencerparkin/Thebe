#include "ChineseCheckers/Generators/TraditionalGenerator.h"
#include "ChineseCheckers/Graph.h"
#include "ChineseCheckers/Factory.h"
#include <map>

using namespace ChineseCheckers;

TraditionalGenerator::TraditionalGenerator(Factory* factory) : GraphGenerator(factory)
{
}

/*virtual*/ TraditionalGenerator::~TraditionalGenerator()
{
}

/*virtual*/ Graph* TraditionalGenerator::Generate(const std::set<Marble::Color>& participantSet)
{
	constexpr int numRows = 17;
	constexpr int numCols = 17;
	static int mask[numRows][numCols] = {
		{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, RZ,  0,  0,  0,  0},
		{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, RZ, RZ,  0,  0,  0,  0},
		{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, RZ, RZ, RZ,  0,  0,  0,  0},
		{ 0,  0,  0,  0,  0,  0,  0,  0,  0, RZ, RZ, RZ, RZ,  0,  0,  0,  0},
		{ 0,  0,  0,  0, BZ, BZ, BZ, BZ, NZ, NZ, NZ, NZ, NZ, GZ, GZ, GZ, GZ},
		{ 0,  0,  0,  0, BZ, BZ, BZ, NZ, NZ, NZ, NZ, NZ, NZ, GZ, GZ, GZ,  0},
		{ 0,  0,  0,  0, BZ, BZ, NZ, NZ, NZ, NZ, NZ, NZ, NZ, GZ, GZ,  0,  0},
		{ 0,  0,  0,  0, BZ, NZ, NZ, NZ, NZ, NZ, NZ, NZ, NZ, GZ,  0,  0,  0},
		{ 0,  0,  0,  0, NZ, NZ, NZ, NZ, CZ, NZ, NZ, NZ, NZ,  0,  0,  0,  0},
		{ 0,  0,  0, KZ, NZ, NZ, NZ, NZ, NZ, NZ, NZ, NZ, PZ,  0,  0,  0,  0},
		{ 0,  0, KZ, KZ, NZ, NZ, NZ, NZ, NZ, NZ, NZ, PZ, PZ,  0,  0,  0,  0},
		{ 0, KZ, KZ, KZ, NZ, NZ, NZ, NZ, NZ, NZ, PZ, PZ, PZ,  0,  0,  0,  0},
		{KZ, KZ, KZ, KZ, NZ, NZ, NZ, NZ, NZ, PZ, PZ, PZ, PZ,  0,  0,  0,  0},
		{ 0,  0,  0,  0, YZ, YZ, YZ, YZ,  0,  0,  0,  0,  0,  0,  0,  0,  0},
		{ 0,  0,  0,  0, YZ, YZ, YZ,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
		{ 0,  0,  0,  0, YZ, YZ,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
		{ 0,  0,  0,  0, YZ,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
	};

	std::unique_ptr<Graph> graph(this->factory->CreateGraph());
	if (!graph)
		return nullptr;

	graph->SetColorTarget(Marble::Color::BLACK, Marble::Color::GREEN);
	graph->SetColorTarget(Marble::Color::GREEN, Marble::Color::BLACK);
	graph->SetColorTarget(Marble::Color::YELLOW, Marble::Color::RED);
	graph->SetColorTarget(Marble::Color::RED, Marble::Color::YELLOW);
	graph->SetColorTarget(Marble::Color::BLUE, Marble::Color::MAGENTA);
	graph->SetColorTarget(Marble::Color::MAGENTA, Marble::Color::BLUE);

	Node* matrix[numRows][numCols];
	for (int i = 0; i < numRows; i++)
		for (int j = 0; j < numCols; j++)
			matrix[i][j] = nullptr;

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			if (mask[i][j] == 0)
				continue;

			Marble::Color color = Marble::Color::NONE;
			switch (mask[i][j])
			{
				case KZ: color = Marble::Color::BLACK; break;
				case YZ: color = Marble::Color::YELLOW; break;
				case PZ: color = Marble::Color::MAGENTA; break;
				case GZ: color = Marble::Color::GREEN; break;
				case RZ: color = Marble::Color::RED; break;
				case BZ: color = Marble::Color::BLUE; break;
			}

			Node* node = this->factory->CreateNode(Vector(0.0, 0.0), color);
			if (!node)
				return nullptr;

			graph->AddNode(node);
			matrix[i][j] = node;

			if (participantSet.find(color) != participantSet.end())
			{
				Marble* marble = this->factory->CreateMarble(color);
				if (!marble)
					return nullptr;

				node->SetOccupant(marble);
			}
		}
	}

	Node* centerNode = nullptr;

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			Node* node = matrix[i][j];
			if (!node)
				continue;

			if (mask[i][j] == CZ)
				centerNode = node;

			node->AddAjacentNode((j < numCols - 1 && matrix[i][j + 1]) ? matrix[i][j + 1] : nullptr);
			node->AddAjacentNode((j < numCols - 1 && i > 0 && matrix[i - 1][j + 1]) ? matrix[i - 1][j + 1] : nullptr);
			node->AddAjacentNode((i > 0 && matrix[i - 1][j]) ? matrix[i - 1][j] : nullptr);
			node->AddAjacentNode((j > 0 && matrix[i][j - 1]) ? matrix[i][j - 1] : nullptr);
			node->AddAjacentNode((j > 0 && i < numRows - 1 && matrix[i + 1][j - 1]) ? matrix[i + 1][j - 1] : nullptr);
			node->AddAjacentNode((i < numRows - 1 && matrix[i + 1][j]) ? matrix[i + 1][j] : nullptr);
		}
	}

	if (!centerNode)
		return nullptr;

	double radius = this->scale;
	centerNode->SetLocation(Vector(0.0, 0.0));
	std::set<Node*> locationAssignedSet;
	locationAssignedSet.insert(centerNode);
	std::list<Node*> queue;
	queue.push_back(centerNode);
	while (queue.size() > 0)
	{
		Node* node = *queue.begin();
		queue.pop_front();
		
		for (int i = 0; i < node->GetNumAdjacentNodes(); i++)
		{
			Node* adjacentNode = node->GetAdjacentNode(i);

			if (adjacentNode && locationAssignedSet.find(adjacentNode) == locationAssignedSet.end())
			{
				double angle = 2.0 * M_PI * double(i) / double(node->GetNumAdjacentNodes());
				Vector delta(radius * cos(angle), -radius * sin(angle));
				adjacentNode->SetLocation(node->GetLocation() + delta);
				locationAssignedSet.insert(adjacentNode);
				queue.push_back(adjacentNode);
			}
		}
	}

	for (Node* node : graph->GetNodeArray())
		node->RemoveNullAdjacencies();

	return graph.release();
}