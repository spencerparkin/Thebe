#include "HexagonalGame.h"

using namespace Thebe;

HexagonalGame::HexagonalGame()
{
}

/*virtual*/ HexagonalGame::~HexagonalGame()
{
}

/*virtual*/ void HexagonalGame::GenerateGraph()
{
	this->Clear();

	constexpr int numRows = 17;
	constexpr int numCols = 17;
	static int mask[numRows][numCols] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0, 0, 0},
		{0, 0, 0, 0, 5, 5, 5, 5, 7, 7, 7, 7, 7, 3, 3, 3, 3},
		{0, 0, 0, 0, 5, 5, 5, 7, 7, 7, 7, 7, 7, 3, 3, 3, 0},
		{0, 0, 0, 0, 5, 5, 7, 7, 7, 7, 7, 7, 7, 3, 3, 0, 0},
		{0, 0, 0, 0, 5, 7, 7, 7, 7, 7, 7, 7, 7, 3, 0, 0, 0},
		{0, 0, 0, 0, 7, 7, 7, 7, 8, 7, 7, 7, 7, 0, 0, 0, 0},
		{0, 0, 0, 6, 7, 7, 7, 7, 7, 7, 7, 7, 2, 0, 0, 0, 0},
		{0, 0, 6, 6, 7, 7, 7, 7, 7, 7, 7, 2, 2, 0, 0, 0, 0},
		{0, 6, 6, 6, 7, 7, 7, 7, 7, 7, 2, 2, 2, 0, 0, 0, 0},
		{6, 6, 6, 6, 7, 7, 7, 7, 7, 2, 2, 2, 2, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	// Create the nodes and occupants.
	Node* centerNode = nullptr;
	Node* matrix[numRows][numCols];
	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			matrix[i][j] = nullptr;

			int zoneID = mask[i][j];
			if (zoneID == 0)
				continue;

			auto node = new Node();
			matrix[i][j] = node;
			node->zoneID = zoneID;
			this->nodeArray.push_back(node);

			if (zoneID == 8)
				centerNode = node;

			if (zoneID != 7 && zoneID != 8)
			{
				auto occupant = new Occupant();
				occupant->playerID = zoneID;
				node->occupant = occupant;
				this->occupantArray.push_back(occupant);
			}
		}
	}

	// Wire up the graph.
	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			Node* node = matrix[i][j];
			if (node)
			{
				node->adjacentNodeArray.push_back((j < numCols - 1 && matrix[i][j + 1]) ? matrix[i][j + 1] : nullptr);
				node->adjacentNodeArray.push_back((j < numCols - 1 && i > 0 && matrix[i - 1][j + 1]) ? matrix[i - 1][j + 1] : nullptr);
				node->adjacentNodeArray.push_back((i > 0 && matrix[i - 1][j]) ? matrix[i - 1][j] : nullptr);
				node->adjacentNodeArray.push_back((j > 0 && matrix[i][j - 1]) ? matrix[i][j - 1] : nullptr);
				node->adjacentNodeArray.push_back((j > 0 && i < numRows - 1 && matrix[i + 1][j - 1]) ? matrix[i + 1][j - 1] : nullptr);
				node->adjacentNodeArray.push_back((i < numRows - 1 && matrix[i + 1][j]) ? matrix[i + 1][j] : nullptr);
			}
		}
	}

	// Spacially orient the graph.
	constexpr double radius = 10.0;
	centerNode->location.SetComponents(0.0, 0.0, 0.0);
	std::set<Node*> locationAssignedSet;
	locationAssignedSet.insert(centerNode);
	std::list<Node*> queue;
	queue.push_back(centerNode);
	while (queue.size() > 0)
	{
		Node* node = *queue.begin();
		queue.pop_front();
		for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
		{
			Node* adjacentNode = node->adjacentNodeArray[i];
			if (adjacentNode && locationAssignedSet.find(adjacentNode) == locationAssignedSet.end())
			{
				double angle = 2.0 * THEBE_PI * double(i) / double(node->adjacentNodeArray.size());
				Vector3 delta(radius * cos(angle), 0.0, -radius * sin(angle));
				adjacentNode->location = node->location + delta;
				locationAssignedSet.insert(adjacentNode);
				queue.push_back(adjacentNode);
			}
		}
	}

	// Remove null adjacencies that were just convenient for us during the graph generation process.
	for (auto node : this->nodeArray)
		node->RemoveNullAdjacencies();
}

/*virtual*/ bool HexagonalGame::GetTargetZoneForPlayer(int playerID, int& targetZoneID)
{
	return true;
}

/*virtual*/ int HexagonalGame::GetMaxPossiblePlayers() const
{
	return 6;
}

/*virtual*/ const char* HexagonalGame::GetGameType() const
{
	return "hexagonal";
}