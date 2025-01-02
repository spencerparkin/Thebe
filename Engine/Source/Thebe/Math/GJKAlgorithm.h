#pragma once

#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Plane.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/AxisAlignedBoundingBox.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/Ray.h"
#include "Thebe/Math/Matrix3x3.h"
#include "Thebe/Math/Graph.h"
#include <functional>

namespace Thebe
{
	/**
	 * Such shapes lend themselves to the GJK algorithm, as well as
	 * algorithms in other areas.
	 */
	class THEBE_API GJKShape
	{
	public:
		GJKShape();
		virtual ~GJKShape();

		/**
		 * An interesting feature of the GJK algorithm is that it doesn't
		 * care what kind of shape you're working with as long as it supports
		 * this method.
		 * 
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
		 * Calculate and return the smallest possible axis-aligned box that bounds this shape in object space.
		 */
		virtual AxisAlignedBoundingBox GetObjectBoundingBox() const = 0;

		/**
		 * Calculate and return the smallest possible axis-aligned box that bounds this shape in world space.
		 */
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const = 0;

		/**
		 * Cast a ray against this shape.
		 * 
		 * @param[in] ray This is the ray to cast.
		 * @param[out] alpha This gets set to the distance along the given ray to where it hits the shape, if indeed it does.
		 * @param[out] unitSurfaceNormal This gets set to a vector normal to the surface at the hit point.
		 * @return True should be returned if and only if the ray hits this shape.
		 */
		virtual bool RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const = 0;

		/**
		 * Calculate and return the mass and inertia tensor of this shape.
		 * 
		 * If my understanding is correct, the inertia tensor, when taken in a
		 * quadratic form with an unit-axis vector, gives you the moment of inertia
		 * of the shape about that axis.  This is a scalar value indicating how
		 * "hard" or "easy" it is to rotate the body about this axis.  TODO: Likely not true.  Need to research more.
		 * 
		 * @param[out] objectSpaceInertiaTensor The inertia tensor of the shape in object space is returned in this matrix.
		 * @param[out] totalMass The total mass of the shape is returned in this scalar.
		 * @param[in] densityFunction This function is used to convert from volume to mass while performing calculations.
		 * @return True is returned if successful; false, otherwise.
		 */
		virtual bool CalculateRigidBodyCharacteristics(Matrix3x3& objectSpaceInertiaTensor, double& totalMass, std::function<double(const Vector3&)> densityFunction) const;

		/**
		 * Calculate and return the geometric center of the shape.  This is not necessarily a center of mass.
		 */
		virtual Vector3 CalcGeometricCenter() const = 0;

		/**
		 * Tell the caller if the two given shapes interesect.
		 * 
		 * @return True is returned if and only if the two given shapes share at least one point in common.
		 */
		static bool Intersect(const GJKShape* shapeA, const GJKShape* shapeB);

		void SetObjectToWorld(const Transform& objectToWorld);
		const Transform& GetObjectToWorld() const;

	protected:
		Transform objectToWorld;		///< All shapes should represent themselves in object-space and then require this in order to be realized in world space.
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
		 * 
		 * @param[out] inverted If given, set to true if and only if the tetrahedron had to be inverted.
		 */
		void MakeFaces(bool* inverted = nullptr);

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
	 * 
	 */
	class THEBE_API GJKSphere : public GJKShape
	{
	public:
		GJKSphere();
		virtual ~GJKSphere();

		virtual Vector3 FurthestPoint(const Vector3& unitDirection) const override;
		virtual AxisAlignedBoundingBox GetObjectBoundingBox() const override;
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const override;
		virtual bool RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const override;
		virtual Vector3 CalcGeometricCenter() const override;

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
		virtual AxisAlignedBoundingBox GetObjectBoundingBox() const override;
		virtual AxisAlignedBoundingBox GetWorldBoundingBox() const override;
		virtual bool RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const override;
		virtual bool CalculateRigidBodyCharacteristics(Matrix3x3& objectSpaceInertiaTensor, double& totalMass, std::function<double(const Vector3&)> densityFunction) const override;
		virtual Vector3 CalcGeometricCenter() const override;

		void GenerateEdgeSet(std::set<Graph::UnorderedEdge, Graph::UnorderedEdge>& edgeSet) const;
		void GenerateObjectSpacePlaneArray(std::vector<Plane>& objectSpacePlaneArray) const;
		Vector3 GetWorldVertex(int i) const;

		PolygonMesh hull;
	};
}