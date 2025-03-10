#pragma once

#include "Thebe/Common.h"
#include "Thebe/Math/PolygonMesh.h"

namespace Thebe
{
	class PolygonMesh;

	/**
	 * This is essentially meta-data about a polygon mesh.
	 * Vertices of the graph are faces of the mesh.  Edges of
	 * the graph represent adjacencies of polygons in the mesh.
	 */
	class THEBE_API FacetGraph
	{
	public:
		FacetGraph();
		virtual ~FacetGraph();

		/**
		 * Regenerate this graph as a function of the given mesh.
		 * Note that the given mesh cannot safely go out of scope
		 * before this graph goes out of scope, because we cache
		 * pointers to the polygon mesh internals here.
		 */
		bool Regenerate(const PolygonMesh& polygonMesh);

		/**
		 * Reset this graph to the empty graph and reclaim all memory.
		 */
		void Clear();

		/**
		 * ...
		 */
		bool GenerateTriangleStrip(std::vector<int>& triangleStrip) const;

		/**
		 * Each of these represents a facet (or polygon) of a polygon mesh.
		 */
		class Node
		{
		public:
			Node(const PolygonMesh::Polygon* polygon);
			virtual ~Node();

			/**
			 * This is used by the graph generation process.
			 */
			static bool SharedAdjacencyFound(const Node* nodeA, const Node* nodeB, int& vertexA, int& vertexB);

			/**
			 * Return the first index we find of this polygon's vertices
			 * that is not shared with the given polygon.
			 */
			int FindUncommonVertexWith(const Node* node) const;

			/**
			 * Find the index of this node's adjacency that points to the given node; or -1, if not found.
			 */
			int FindAdjacencyIndex(const Node* node) const;

		public:
			/**
			 * This is the polygon we represent.
			 */
			const PolygonMesh::Polygon* polygon;

			/**
			 * The order here parallels that of the vertex array
			 * for each polygon of the mesh.  An element can be
			 * null if there is no adjacency for the side of the
			 * polygon in question.
			 */
			std::vector<Node*> adjacencyArray;
		};

	private:

		std::vector<Node*> nodeArray;
	};
}