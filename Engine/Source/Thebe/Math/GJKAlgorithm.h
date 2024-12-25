#pragma once

#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Plane.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/AxisAlignedBoundingBox.h"

namespace Thebe
{
	/**
	 * An interesting feature of the GJK algorithm is that it doesn't
	 * care what kind of shape you're working with as long as it supports
	 * the following interface, and as long as it is convex.
	 */
	class THEBE_API GJKShape
	{
	public:
		GJKShape();
		virtual ~GJKShape();

		/**
		 * When the given direction is placed somewhere (anywhere),
		 * any point has a distance along the line containing this
		 * direction by orthogonally projecting that point down onto
		 * the line, and then measuring the distance from the arbitrary
		 * origin (placement of the direction vector) to the projected
		 * point.  For convex shapes, every edge point shows up as a
		 * point among the furtherest of such along some direction in
		 * the mannor told.  The same cannot be said, however, for concave
		 * shapes.  That is, there exists a point in every convex shape
		 * that is never among the furthest of such along any given direction.
		 * Thus, we require a GJK shape to be convex so that we can explore
		 * the entirety of the shape using this support function.
		 * 
		 * @param[in] unitDirection This should be a unit-length vector indicating the desired direction along which to measure distance.
		 * @return Any point on this shape among those furthest in the given direction should be returned, preferrably a vertex, if applicable.
		 */
		virtual Vector3 FurthestPoint(const Vector3& unitDirection) const = 0;

		/**
		 * Calculate and return the smallest possible box that bounds this shape.
		 */
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const = 0;

		/**
		 * Tell the caller if the two given shapes interesect.
		 * 
		 * @return True is returned if and only if the two given shapes share at least one point in common.
		 */
		static bool Intersect(const GJKShape* shapeA, const GJKShape* shapeB);

		// TODO: Can we extend GJK to give us the signed distance between two shapes, and in what direction that distance is measured?

		Transform objectToWorld;
	};

	/**
	 * The GJK algorithm generalizes to N-dimensional euclidean space, and
	 * that's why the term simplex is used.  However, for our purposes, here
	 * a simplex is always going to mean a tetrahedron.
	 */
	class THEBE_API GJKSimplex
	{
	public:
		GJKSimplex();
		virtual ~GJKSimplex();

		struct Face;

		void operator=(const GJKSimplex& simplex);

		/**
		 * A valid tetrahedron will return a non-negative volume here.
		 */
		double CalcVolume() const;

		/**
		 * The given plane array is expected to have four elements.
		 */
		void CalcFacePlanes(Plane* planeArray, const Face** givenFaceArray) const;

		/**
		 * Having populated the vertex array, populate the face array automatically,
		 * making sure that the volume of the tetrahedron is non-negative.
		 */
		void MakeFaces();

		/**
		 * Tell us if this simplex contains the origin.
		 */
		bool ContainsOrigin() const;

		/**
		 * Each face of a tetrahedron is a triangle.
		 */
		struct Face
		{
			/**
			 * These index into the vertex array, and the order does matter.
			 * Front faces are seen with the vertices winding CCW in a plane.
			 */
			int vertexArray[3];

			int OtherVertex() const;
			bool HasVertex(int i) const;
		};

		/**
		 * These are the vertices of the tetrahedron.  The order not matter.
		 */
		Vector3 vertexArray[4];

		/** 
		 * These are the faces (or facets) of the tetrahedron.  Again the order does not matter.
		 */
		Face faceArray[4];
	};

	/**
	 * I suppose that GJK would be less efficient in the case of determining
	 * whether two spheres intersect than just the trivial and most obvious
	 * means of doing so, but we may want to test a sphere against some other
	 * type of shape.
	 */
	class THEBE_API GJKSphere : public GJKShape
	{
	public:
		GJKSphere();
		virtual ~GJKSphere();

		virtual Vector3 FurthestPoint(const Vector3& unitDirection) const override;
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const override;

		Vector3 center;
		double radius;
	};

	/**
	 * 
	 */
	class THEBE_API GJKConvexHull : public GJKShape
	{
	public:
		GJKConvexHull();
		virtual ~GJKConvexHull();

		virtual Vector3 FurthestPoint(const Vector3& unitDirection) const override;
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const override;

		/**
		 * Generate a convex hull from any arbitrarily chosen set of points.
		 */
		bool CalculateFromPointCloud(const std::vector<Vector3>& pointCloud);

		/**
		 * Generate a polygon mesh from this shape's vertex array.
		 */
		bool GeneratePolygonMesh(PolygonMesh& polygonMesh) const;

		/**
		 * The convex hull represented here is the one found as the smallest
		 * of such containing the given set of points.  Further, we assume here
		 * that this is the smallest possible set of points needed to represent
		 * the hull in this manner.
		 */
		std::vector<Vector3> vertexArray;
	};
}