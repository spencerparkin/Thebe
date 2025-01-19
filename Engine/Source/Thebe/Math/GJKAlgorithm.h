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
#include <functional>

#define GJK_RENDER_DEBUG

#if defined GJK_RENDER_DEBUG
#include "Thebe/Network/DebugRenderClient.h"
#endif

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
		 * @return True is returned if and only if the two given shapes share at least one point in common.
		 */
		static bool Intersect(const GJKShape* shapeA, const GJKShape* shapeB);

		void SetObjectToWorld(const Transform& objectToWorld);
		const Transform& GetObjectToWorld() const;

	protected:
		Transform objectToWorld;		///< All shapes should represent themselves in object-space and then require this in order to be realized in world space.
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
		 * Generate the next simplex we need in the course of the GJK algorithm.
		 * If null is returned here, the algorithm can terminate, and we can
		 * conclude that there is no intersection.
		 */
		virtual GJKSimplex* GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const = 0;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) = 0;
#endif //GJK_RENDER_DEBUG

		static Vector3 CalcSupportPoint(const GJKShape* shapeA, const GJKShape* shapeB, const Vector3& unitDirection);
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
		virtual GJKSimplex* GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const override;

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
		virtual GJKSimplex* GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const override;

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
		virtual GJKSimplex* GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const override;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) override;
#endif //GJK_RENDER_DEBUG

		Vector3 vertex[3];
		mutable bool originOnPlane;
		mutable Plane edgePlane[3];
		mutable Plane trianglePlane;
		mutable double originDistanceToTrianglePlane;
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
		virtual GJKSimplex* GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const override;

#if defined GJK_RENDER_DEBUG
		virtual void DebugDraw(DebugRenderClient* client, int simplexNumber) override;
#endif //GJK_RENDER_DEBUG

		Vector3 vertex[4];
		mutable Plane facePlane[4];
	};
}