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
#include "Thebe/Math/LineSegment.h"
#include "Thebe/Math/ExpandingPolytopeAlgorithm.h"

#define GJK_RENDER_DEBUG

#if defined GJK_RENDER_DEBUG
#include "Thebe/Network/DebugRenderClient.h"
#endif

namespace Thebe
{
	class GJKSimplex;
	class GJKTetrahedronSimplex;

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
		 * Shift the representation of this shape in object-space.
		 * 
		 * @param[in] translation The shape should be shifted by this vector.
		 */
		virtual void Shift(const Vector3& translation) = 0;

		/**
		 * Calculate and return the geometric center of the shape.  This is not necessarily the center of mass.
		 */
		virtual Vector3 CalcGeometricCenter() const = 0;

		/**
		 * Tell the caller if the given point (in object space) is contained within this shape (in object space.)
		 */
		virtual bool ContainsObjectPoint(const Vector3& point, void* cache = nullptr) const;

		/**
		 * Tell the caller if the given point (in world space) is contained within this shape (in world space.);
		 */
		virtual bool ContainsWorldPoint(const Vector3& point, void* cache = nullptr) const;

		/**
		 * Tell the caller if the two given shapes interesect.
		 * 
		 * @param[out] finalSimplex If given, the final simplex of the algorithm is placed here in the case of intersection.
		 * @return True is returned if and only if the two given shapes share at least one point in common.
		 */
		static bool Intersect(const GJKShape* shapeA, const GJKShape* shapeB, std::unique_ptr<GJKSimplex>* finalSimplex = nullptr);

		/**
		 * Use the extended polytop algorithm (EPA) to calculate the penetration depth and direction
		 * of the two given shapes.  It's assumed here that it has already been determined that these
		 * shapes intersect using the @ref GJKShape::Intersect function.
		 * 
		 * @param[in] simplex This just needs to be the simplex returned from @ref GJKShape::Intersect, called with the same shapes.
		 * @param[out] separationDelta This is the vector that, if added to shapeA (or subtracted from shapeB) cause both shapes to share a set of points only on their boundary.
		 * @return True is returned on success; false, otherwise.
		 */
		static bool Penetration(const GJKShape* shapeA, const GJKShape* shapeB, std::unique_ptr<GJKSimplex>& simplex, Vector3& separationDelta);

		void SetObjectToWorld(const Transform& objectToWorld);
		const Transform& GetObjectToWorld() const;

	protected:
		Transform objectToWorld;		///< All shapes should represent themselves in object-space and then require this in order to be realized in world space.
	};

	/**
	 * 
	 */
	class THEBE_API GJKTriangleForEPA : public ExpandingPolytopeAlgorithm::Triangle
	{
	public:
		GJKTriangleForEPA()
		{
			this->onEdgeOfMinkowskiHull = false;
		}

		bool onEdgeOfMinkowskiHull;
	};

	/**
	 * 
	 */
	class THEBE_API GJKPointSupplierForEPA : public ExpandingPolytopeAlgorithm::PointSupplier
	{
	public:
		GJKPointSupplierForEPA(const GJKShape* shapeA, const GJKShape* shapeB, ExpandingPolytopeAlgorithm* epa);
		virtual ~GJKPointSupplierForEPA();

		virtual bool GetNextPoint(Vector3& point) override;

	private:
		const GJKShape* shapeA;
		const GJKShape* shapeB;
		ExpandingPolytopeAlgorithm* epa;
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
		virtual void Shift(const Vector3& translation) override;
		virtual bool ContainsObjectPoint(const Vector3& point, void* cache = nullptr) const override;
		virtual bool ContainsWorldPoint(const Vector3& point, void* cache = nullptr) const override;

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
		virtual Vector3 CalcGeometricCenter() const override;
		virtual void Shift(const Vector3& translation) override;
		virtual bool ContainsObjectPoint(const Vector3& point, void* cache = nullptr) const override;

		struct PointContainmentCache
		{
			std::vector<Plane> planeArray;
		};

		void GenerateEdgeSet(std::set<Graph::UnorderedEdge, Graph::UnorderedEdge>& edgeSet) const;
		void GenerateObjectSpacePlaneArray(std::vector<Plane>& objectSpacePlaneArray) const;
		Vector3 GetWorldVertex(int i) const;

		PolygonMesh hull;
	};

	/**
	 *
	 */
	class THEBE_API GJKSimplex
	{
	public:
		GJKSimplex();
		virtual ~GJKSimplex();

		/**
		 * Tell us if the origin is contained within this simplex.
		 * If it does, then the algorithm can terminate, and we can
		 * conclude that there is an intersection.
		 */
		virtual bool ContainsOrigin(double epsilon) const = 0;

		/**
		 * Return the dimensionality of this simplex.
		 */
		virtual uint32_t GetDimension() const = 0;

		/**
		 * This method should always succeed in the cases we're interested in.
		 * If it does not succeed, then our math has gone wrong.
		 */
		virtual GJKSimplex* FindFacetWithVoronoiRegionContainingOrigin() const;

		/**
		 * This method should always succeed in the cases we're interested in.
		 * When more than one normal can be orthogonal to this simplex (thought of
		 * in this case as a facet of a higher-dimensional simplex), the returned
		 * normal should be chosen such that it points toward the origin as much
		 * as possible.
		 */
		virtual Vector3 CalcFacetNormWithBiasTowardsOrigin() const;

		/**
		 * Again, this method should always succeed for the cases in which we're interested.
		 * In all other cases, it should always fail.
		 */
		virtual GJKSimplex* ExtendSimplexWithPoint(const Vector3& supportPoint) const;

		/**
		 * Provide this as a consistent way to calculate points in the Minkowski difference of the two given shapes.
		 * The idea is to find such a point as far in the given direction as possible.
		 */
		static Vector3 CalcSupportPoint(const GJKShape* shapeA, const GJKShape* shapeB, const Vector3& unitDirection);

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) = 0;
#endif //GJK_RENDER_DEBUG
	};

	/**
	 * This is the 0-dimensional simplex.
	 */
	class THEBE_API GJKPointSimplex : public GJKSimplex
	{
	public:
		GJKPointSimplex();
		virtual ~GJKPointSimplex();

		virtual bool ContainsOrigin(double epsilon) const override;
		virtual uint32_t GetDimension() const override;
		virtual Vector3 CalcFacetNormWithBiasTowardsOrigin() const override;
		virtual GJKSimplex* ExtendSimplexWithPoint(const Vector3& supportPoint) const override;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) override;
#endif //GJK_RENDER_DEBUG

		Vector3 point;
	};

	/**
	 * This is the 1-dimensional simplex.
	 */
	class THEBE_API GJKLineSimplex : public GJKSimplex
	{
	public:
		GJKLineSimplex();
		virtual ~GJKLineSimplex();

		virtual bool ContainsOrigin(double epsilon) const override;
		virtual uint32_t GetDimension() const override;
		virtual Vector3 CalcFacetNormWithBiasTowardsOrigin() const override;
		virtual GJKSimplex* ExtendSimplexWithPoint(const Vector3& supportPoint) const override;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) override;
#endif //GJK_RENDER_DEBUG

		LineSegment lineSegment;
	};

	/**
	 * This is the 2-dimensional simplex.
	 */
	class THEBE_API GJKTriangleSimplex : public GJKSimplex
	{
	public:
		GJKTriangleSimplex();
		virtual ~GJKTriangleSimplex();

		virtual bool ContainsOrigin(double epsilon) const override;
		virtual uint32_t GetDimension() const override;
		virtual Vector3 CalcFacetNormWithBiasTowardsOrigin() const override;
		virtual GJKSimplex* ExtendSimplexWithPoint(const Vector3& supportPoint) const override;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) override;
#endif //GJK_RENDER_DEBUG

		Vector3 vertex[3];
	};

	/**
	 * This is the 3-dimensional simplex.
	 */
	class THEBE_API GJKTetrahedronSimplex : public GJKSimplex
	{
	public:
		GJKTetrahedronSimplex();
		virtual ~GJKTetrahedronSimplex();

		virtual bool ContainsOrigin(double epsilon) const override;
		virtual uint32_t GetDimension() const override;
		virtual GJKSimplex* FindFacetWithVoronoiRegionContainingOrigin() const override;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) override;
#endif //GJK_RENDER_DEBUG

		struct FacetData
		{
			Plane plane;
			int vertex[3];
		};

		Vector3 vertex[4];
		mutable FacetData facetDataArray[4];
	};
}