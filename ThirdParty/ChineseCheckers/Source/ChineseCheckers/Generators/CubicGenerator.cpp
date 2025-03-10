#include "ChineseCheckers/Generators/CubicGenerator.h"
#include "ChineseCheckers/Factory.h"
#include "ChineseCheckers/Graph.h"

using namespace ChineseCheckers;

CubicGenerator::CubicGenerator(Factory* factory) : GraphGenerator(factory)
{
}

/*virtual*/ CubicGenerator::~CubicGenerator()
{
}

/*virtual*/ Graph* CubicGenerator::Generate(const std::set<Marble::Color>& participantSet)
{
	constexpr int numRows = 11;
	constexpr int numCols = 11;
	constexpr int zoneSize = 3;

	std::unique_ptr<Graph> graph(this->factory->CreateGraph());

	graph->SetColorTarget(Marble::Color::YELLOW, Marble::Color::RED);
	graph->SetColorTarget(Marble::Color::RED, Marble::Color::YELLOW);
	graph->SetColorTarget(Marble::Color::BLACK, Marble::Color::GREEN);
	graph->SetColorTarget(Marble::Color::GREEN, Marble::Color::BLACK);

	Node* matrix[numRows][numCols];
	for (int i = 0; i < numRows; i++)
		for (int j = 0; j < numCols; j++)
			matrix[i][j] = nullptr;

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			Marble::Color color = Marble::Color::NONE;
			if (i < zoneSize && j < zoneSize)
				color = Marble::Color::YELLOW;
			else if (i < zoneSize && j > numCols - 1 - zoneSize)
				color = Marble::Color::BLACK;
			else if (i > numRows - 1 - zoneSize && j < zoneSize)
				color = Marble::Color::GREEN;
			else if (i > numRows - 1 - zoneSize && j > numCols - 1 - zoneSize)
				color = Marble::Color::RED;

			Node* node = this->factory->CreateNode(Vector(0.0, 0.0), color);
			if (!node)
				return nullptr;

			graph->AddNode(node);
			matrix[i][j] = node;

			if (participantSet.find(color) != participantSet.end())
			{
				std::shared_ptr<Marble> marble = this->factory->CreateMarble(color);
				if (!marble)
					return nullptr;

				node->SetOccupant(marble);
			}
		}
	}

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			Node* node = matrix[i][j];

			if (i > 0)
				node->AddAjacentNode(matrix[i - 1][j]);
			if (i < numRows - 1)
				node->AddAjacentNode(matrix[i + 1][j]);
			if (j > 0)
				node->AddAjacentNode(matrix[i][j - 1]);
			if (j < numCols - 1)
				node->AddAjacentNode(matrix[i][j + 1]);
		}
	}

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			Node* node = matrix[i][j];
			Vector location(double(i - (numRows - 1) / 2), double(j - (numCols - 1) / 2));
			location *= this->scale;
			node->SetLocation(location);
		}
	}

	return graph.release();
}