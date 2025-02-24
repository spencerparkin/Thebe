#include "ChineseCheckers/Generators/CubicDonutGenerator.h"
#include "ChineseCheckers/Factory.h"
#include "ChineseCheckers/Graph.h"

using namespace ChineseCheckers;

CubicDonutGenerator::CubicDonutGenerator(Factory* factory) : GraphGenerator(factory)
{
	this->addDiagonalConnections = true;
}

/*virtual*/ CubicDonutGenerator::~CubicDonutGenerator()
{
}

/*virtual*/ Graph* CubicDonutGenerator::Generate(const std::set<Marble::Color>& participantSet)
{
	const int numRows = 13;
	const int numCols = 13;
	const int marginSize = 4;
	const int zoneSize = 3;

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
			if (i >= marginSize && i < numRows - marginSize &&
				j >= marginSize && j < numCols - marginSize)
			{
				continue;
			}

			Marble::Color color = Marble::Color::NONE;
			if (i < zoneSize && j < zoneSize)
				color = Marble::Color::YELLOW;
			else if (i < zoneSize && j >= numCols - zoneSize)
				color = Marble::Color::BLACK;
			else if (i >= numRows - zoneSize && j < zoneSize)
				color = Marble::Color::GREEN;
			else if (i >= numRows - zoneSize && j >= numCols - zoneSize)
				color = Marble::Color::RED;

			Node* node = this->factory->CreateNode(Vector(0.0, 0.0), color);

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
			if (!node)
				continue;

			if (i > 0 && matrix[i - 1][j])
				node->AddAjacentNode(matrix[i - 1][j]);
			if (i < numRows - 1 && matrix[i + 1][j])
				node->AddAjacentNode(matrix[i + 1][j]);
			if (j > 0 && matrix[i][j - 1])
				node->AddAjacentNode(matrix[i][j - 1]);
			if (j < numCols - 1 && matrix[i][j + 1])
				node->AddAjacentNode(matrix[i][j + 1]);

			if (this->addDiagonalConnections)
			{
				if (i > 0 && j > 0 && matrix[i - 1][j - 1])
					node->AddAjacentNode(matrix[i - 1][j - 1]);
				if (i > 0 && j < numCols - 1 && matrix[i - 1][j + 1])
					node->AddAjacentNode(matrix[i - 1][j + 1]);
				if (i < numRows - 1 && j > 0 && matrix[i + 1][j - 1])
					node->AddAjacentNode(matrix[i + 1][j - 1]);
				if (i < numRows - 1 && j < numCols - 1 && matrix[i + 1][j + 1])
					node->AddAjacentNode(matrix[i + 1][j + 1]);
			}
		}
	}

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			Node* node = matrix[i][j];
			if (!node)
				continue;

			Vector location(double(i - (numRows - 1) / 2), double(j - (numCols - 1) / 2));
			location *= this->scale;
			node->SetLocation(location);
		}
	}

	return graph.release();
}