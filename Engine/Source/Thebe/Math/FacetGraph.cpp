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
	triangleStrip.clear();

	// TODO: Note that not every sequence of triangles (where consecutive triangles
	//       share two vertices in common) can be turned into a triangle strip.
	//       This was a false assumption upon which I had earler operated.  Anyhow,
	//       there is an obvious way to create strips (delimited by -1) that will
	//       fill the entire graph, but what's not obvious is how to make it optimal.
	//       In any case, I'm not motivated to revisit this problem, because I can't
	//       take advantage of it yet, because there's no mesh I have that shares
	//       vertices between triangles.  So any triangle-strippification would just
	//       inflate the size of the index buffer anyway.

	return true;
}

//------------------------------ FacetGraph::Node ------------------------------

FacetGraph::Node::Node(const PolygonMesh::Polygon* polygon)
{
	this->polygon = polygon;

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

int FacetGraph::Node::FindUncommonVertexWith(const Node* node) const
{
	for (int i = 0; i < (int)this->polygon->vertexArray.size(); i++)
		if (!node->polygon->HasVertex(this->polygon->vertexArray[i]))
			return i;

	return -1;
}

int FacetGraph::Node::FindAdjacencyIndex(const Node* node) const
{
	for (int i = 0; i < (int)this->adjacencyArray.size(); i++)
		if (this->adjacencyArray[i] == node)
			return i;

	return -1;
}