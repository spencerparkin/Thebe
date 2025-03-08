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
		 * One application of this kind of graph is to generate
		 * triangle strips from a mesh, and that's what we try
		 * to do here.  We also try to minimize the number of
		 * strips returned.  All the same, there are cases I can
		 * think of where we don't find the optimzal solution here.
		 * 
		 * Note that some graphics APIs allow you to do multiple
		 * triangle strips in one draw-call by concatinating strips
		 * with -1 as a delimeter between strips.
		 * 
		 * We will fail here if the graph is not generated from a
		 * triangle mesh.
		 */
		bool GenerateTriangleStrips(std::vector<std::vector<int>>& triangleStripArray) const;

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

			/**
			 * This is used for tri-stripping, but it could be used for other things.
			 */
			mutable bool encorporated;
		};

	private:

		std::vector<Node*> nodeArray;
	};
}