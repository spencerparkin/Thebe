#include "Thebe/Math/FacetGraph.h"

using namespace Thebe;

//------------------------------ FacetGraph ------------------------------

FacetGraph::FacetGraph()
{
}

/*virtual*/ FacetGraph::~FacetGraph()
{
	this->Clear();
}

bool FacetGraph::Regenerate(const PolygonMesh& polygonMesh)
{
	this->Clear();

	for (const PolygonMesh::Polygon& polygon : polygonMesh.GetPolygonArray())
	{
		auto node = new Node(&polygon);
		this->nodeArray.push_back(node);
	}

	for (int i = 0; i < (int)this->nodeArray.size(); i++)
	{
		Node* nodeA = this->nodeArray[i];

		for (int j = i + 1; j < (int)this->nodeArray.size(); j++)
		{
			Node* nodeB = this->nodeArray[j];

			int vertexA = -1;
			int vertexB = -1;

			if (Node::SharedAdjacencyFound(nodeA, nodeB, vertexA, vertexB))
			{
				if (nodeA->adjacencyArray[vertexA] != nullptr || nodeB->adjacencyArray[vertexB] != nullptr)
					return false;

				nodeA->adjacencyArray[vertexA] = nodeB;
				nodeB->adjacencyArray[vertexB] = nodeA;
			}
		}
	}

	return true;
}

void FacetGraph::Clear()
{
	for (Node* node : this->nodeArray)
		delete node;

	this->nodeArray.clear();
}

bool FacetGraph::GenerateTriangleStrip(std::vector<int>& triangleStrip) const
{
	// TODO: This is untested and surely doesn't work.  Test it.

	triangleStrip.clear();

	for (const Node* triangle : this->nodeArray)
		if (triangle->polygon->vertexArray.size() != 3)
			return false;

	for (const Node* triangle : this->nodeArray)
		triangle->visited = false;

	while (true)
	{
		int i;
		for (i = 0; i < (int)this->nodeArray.size(); i++)
			if (!this->nodeArray[i]->visited)
				break;

		if (i == (int)this->nodeArray.size())
			break;

		const Node* triangle = this->nodeArray[i];

		if (triangleStrip.size() > 0)
			triangleStrip.push_back(-1);

		for (i = 0; i < 3; i++)
			triangleStrip.push_back(triangle->polygon->vertexArray[i]);

		int j = 0;
		i = 1;

		while (true)
		{
			triangle->visited = true;

			const Node* nextTriangle = triangle->adjacencyArray[i];
			if (!nextTriangle || nextTriangle->visited)
				break;

			for (i = 0; i < 3; i++)
				if (!triangle->polygon->HasVertex(nextTriangle->polygon->vertexArray[i]))
					break;

			THEBE_ASSERT(i != 3);

			triangleStrip.push_back(nextTriangle->polygon->vertexArray[i]);
			triangle = nextTriangle;

			if (++j % 2 == 0 && --i < 0)
				i = 2;
		}
	}

	return true;
}

//------------------------------ FacetGraph::Node ------------------------------

FacetGraph::Node::Node(const PolygonMesh::Polygon* polygon)
{
	this->polygon = polygon;
	this->visited = false;

	this->adjacencyArray.resize(polygon->vertexArray.size());
	for (int i = 0; i < (int)this->adjacencyArray.size(); i++)
		this->adjacencyArray[i] = nullptr;
}

/*virtual*/ FacetGraph::Node::~Node()
{
}

/*static*/ bool FacetGraph::Node::SharedAdjacencyFound(const Node* nodeA, const Node* nodeB, int& vertexA, int& vertexB)
{
	for (int i = 0; i < (int)nodeA->polygon->vertexArray.size(); i++)
	{
		for (int j = 0; j < (int)nodeB->polygon->vertexArray.size(); j++)
		{
			if (nodeA->polygon->vertexArray[i] == nodeB->polygon->vertexArray[nodeB->polygon->Mod(j + 1)] &&
				nodeA->polygon->vertexArray[nodeA->polygon->Mod(i + 1)] == nodeB->polygon->vertexArray[j])
			{
				vertexA = i;
				vertexB = j;
				return true;
			}
		}
	}

	return false;
}

int FacetGraph::Node::FindAdjacencyIndex(const Node* node) const
{
	for (int i = 0; i < (int)this->adjacencyArray.size(); i++)
		if (this->adjacencyArray[i] == node)
			return i;

	return -1;
}