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
				nodeB->adjacencyArray[vertexB] = nodeB;
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
	triangleStrip.clear();

	for (const Node* node : this->nodeArray)
		if (node->polygon->vertexArray.size() != 3)
			return false;

	for (const Node* node : this->nodeArray)
		node->encorporated = false;

	// This algorithm depends on the triangles of the mesh all being consistently wound CCW.
	while (true)
	{
		std::list<const Node*> triangleList;

		for (const Node* triangle : this->nodeArray)
		{
			if (!triangle->encorporated)
			{
				triangleList.push_back(triangle);
				triangle->encorporated = true;
				break;
			}
		}

		if (triangleList.size() != 1)
			break;

		struct Item
		{
			enum Type
			{
				APPEND,
				PREPEND
			};

			const Node* triangle;
			int i;
			Type type;
		};

		const Node* triangle = *triangleList.begin();
		std::list<Item> queue;
		int j = 0;
		for (int i = 0; i < (int)triangle->adjacencyArray.size() && queue.size() < 2; i++)
		{
			if (triangle->adjacencyArray[i] != nullptr)
			{
				Item item{ triangle, i, Item::Type(j++) };
				queue.push_back(item);
			}
		}

		while (queue.size() > 0)
		{
			Item item = *queue.begin();
			queue.pop_front();

			const Node* nextTriangle = item.triangle->adjacencyArray[item.i];
			switch (item.type)
			{
				case Item::Type::APPEND:
				{
					triangleList.push_back(nextTriangle);
					break;
				}
				case Item::Type::PREPEND:
				{
					triangleList.push_front(nextTriangle);
					break;
				}
			}

			nextTriangle->encorporated = true;

			int i;
			for (i = 0; i < (int)nextTriangle->adjacencyArray.size(); i++)
				if (nextTriangle->adjacencyArray[i] == item.triangle)
					break;

			THEBE_ASSERT(i != (int)nextTriangle->adjacencyArray.size());

			// Make as tight a right-turn as possible.  (We choose right over left arbitrarily here.  All that matters is consistency.)
			while (true)
			{
				i = (i + 1) % nextTriangle->adjacencyArray.size();

				if (nextTriangle->adjacencyArray[i] == item.triangle)
					break;

				if (nextTriangle->adjacencyArray[i] != nullptr && !nextTriangle->adjacencyArray[i]->encorporated)
				{
					Item nextItem{ nextTriangle, i, item.type };
					queue.push_back(item);
					break;
				}
			}
		}

		if (triangleStrip.size() > 0)
			triangleStrip.push_back(-1);

		std::list<const Node*>::iterator iter = triangleList.begin();
		const Node* triangleA = *iter;
		if (triangleList.size() == 1)
		{
			for (int i = 0; i < (int)triangleA->polygon->vertexArray.size(); i++)
				triangleStrip.push_back(triangleA->polygon->vertexArray[i]);
		}
		else if (triangleList.size() > 1)
		{
			const Node* triangleB = *++iter;
			int i = triangleA->FindUncommonVertexWith(triangleB);
			THEBE_ASSERT(i != -1);
			for (int j = 0; j < int(triangleA->polygon->vertexArray.size()); j++)
				triangleStrip.push_back(triangleA->polygon->vertexArray[triangleA->polygon->Mod(i + j)]);

			while (true)
			{
				i = triangleB->FindUncommonVertexWith(triangleA);
				THEBE_ASSERT(i != -1);
				triangleStrip.push_back(triangleB->polygon->vertexArray[i]);
				if (++iter == triangleList.end())
					break;
				triangleA = triangleB;
				triangleB = *iter;
			}
		}
	}

	return true;
}

//------------------------------ FacetGraph::Node ------------------------------

FacetGraph::Node::Node(const PolygonMesh::Polygon* polygon)
{
	this->polygon = polygon;
	this->encorporated = false;

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
				nodeA->polygon->vertexArray[nodeA->polygon->Mod(i + 1)] == nodeA->polygon->vertexArray[j])
			{
				vertexA = i;
				vertexB = j;
				return true;
			}
		}
	}

	return false;
}

int FacetGraph::Node::FindUncommonVertexWith(const Node* node) const
{
	for (int i = 0; i < (int)this->polygon->vertexArray.size(); i++)
		if (!node->polygon->HasVertex(this->polygon->vertexArray[i]))
			return i;

	return -1;
}