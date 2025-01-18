#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/Polygon.h"
#include "Thebe/Math/Function.h"
#include "Thebe/Log.h"
#include <format>

using namespace Thebe;

//------------------------------------- GJKShape -------------------------------------

GJKShape::GJKShape()
{
}

/*virtual*/ GJKShape::~GJKShape()
{
}

/*static*/ bool GJKShape::Intersect(const GJKShape* shapeA, const GJKShape* shapeB)
{
	if (!shapeA || !shapeB)
		return false;

	// For two spheres that touch in a single point, I can see the GJK algorithm
	// taking a long time to converge on this point.  So just handle two spheres
	// here as a special case.
	auto sphereA = dynamic_cast<const GJKSphere*>(shapeA);
	auto sphereB = dynamic_cast<const GJKSphere*>(shapeB);
	if (sphereA && sphereB)
		return (sphereA->center - sphereB->center).Length() <= sphereA->radius + sphereB->radius;



	return false;
}

void GJKShape::SetObjectToWorld(const Transform& objectToWorld)
{
	this->objectToWorld = objectToWorld;
}

const Transform& GJKShape::GetObjectToWorld() const
{
	return this->objectToWorld;
}

/*virtual*/ bool GJKShape::ContainsObjectPoint(const Vector3& point, void* cache /*= nullptr*/) const
{
	return false;
}

/*virtual*/ bool GJKShape::ContainsWorldPoint(const Vector3& point, void* cache /*= nullptr*/) const
{
	Transform worldToObject;
	worldToObject.Invert(this->objectToWorld);
	return this->ContainsObjectPoint(worldToObject.TransformPoint(point), cache);
}

//------------------------------------- GJKSphere -------------------------------------

GJKSphere::GJKSphere()
{
	this->center.SetComponents(0.0, 0.0, 0.0);
	this->radius = 1.0;
}

/*virtual*/ GJKSphere::~GJKSphere()
{
}

/*virtual*/ Vector3 GJKSphere::FurthestPoint(const Vector3& unitDirection) const
{
	return this->objectToWorld.TransformPoint(this->center) + this->radius * unitDirection;
}

/*virtual*/ AxisAlignedBoundingBox GJKSphere::GetObjectBoundingBox() const
{
	AxisAlignedBoundingBox objectBoundingBox;
	objectBoundingBox.minCorner.SetComponents(-this->radius, -this->radius, -this->radius);
	objectBoundingBox.maxCorner.SetComponents(this->radius, this->radius, this->radius);
	return objectBoundingBox;
}

/*virtual*/ AxisAlignedBoundingBox GJKSphere::GetWorldBoundingBox() const
{
	AxisAlignedBoundingBox worldBoundingBox = this->GetObjectBoundingBox();
	worldBoundingBox.minCorner += this->objectToWorld.translation;
	worldBoundingBox.maxCorner += this->objectToWorld.translation;
	return worldBoundingBox;
}

/*virtual*/ bool GJKSphere::RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const
{
	Vector3 delta = ray.origin - this->center;

	Quadratic quadratic;
	quadratic.A = 1.0;
	quadratic.B = 2.0 * unitSurfaceNormal.Dot(delta);
	quadratic.C = delta.Dot(delta) - this->radius * this->radius;

	std::vector<double> realRoots;
	quadratic.Solve(realRoots);
	if (realRoots.size() == 0)
		return false;

	alpha = std::numeric_limits<double>::max();
	for (double root : realRoots)
		if (root >= 0.0 && root < alpha)
			alpha = root;

	unitSurfaceNormal = (ray.CalculatePoint(alpha) - this->center).Normalized();
	return true;
}

/*virtual*/ Vector3 GJKSphere::CalcGeometricCenter() const
{
	return this->center;
}

/*virtual*/ void GJKSphere::Shift(const Vector3& translation)
{
	this->center += translation;
}

/*virtual*/ bool GJKSphere::ContainsObjectPoint(const Vector3& point, void* cache /*= nullptr*/) const
{
	return point.Length() <= this->radius;
}

/*virtual*/ bool GJKSphere::ContainsWorldPoint(const Vector3& point, void* cache /*= nullptr*/) const
{
	return this->ContainsObjectPoint(point - this->center, cache);
}

//------------------------------------- GJKConvexHull -------------------------------------

GJKConvexHull::GJKConvexHull()
{
}

/*virtual*/ GJKConvexHull::~GJKConvexHull()
{
}

/*virtual*/ Vector3 GJKConvexHull::FurthestPoint(const Vector3& unitDirection) const
{
	double largestDistance = -std::numeric_limits<double>::max();
	Vector3 chosenVertex(0.0, 0.0, 0.0);

	for (const Vector3& objectVertex : this->hull.GetVertexArray())
	{
		Vector3 worldVertex = this->objectToWorld.TransformPoint(objectVertex);
		double distance = worldVertex.Dot(unitDirection);
		if (distance > largestDistance)
		{
			largestDistance = distance;
			chosenVertex = worldVertex;
		}
	}

	return chosenVertex;
}

/*virtual*/ AxisAlignedBoundingBox GJKConvexHull::GetObjectBoundingBox() const
{
	AxisAlignedBoundingBox objectBoundingBox;
	objectBoundingBox.MakeReadyForExpansion();

	for (const Vector3& objectVertex : this->hull.GetVertexArray())
		objectBoundingBox.Expand(objectVertex);

	return objectBoundingBox;
}

/*virtual*/ AxisAlignedBoundingBox GJKConvexHull::GetWorldBoundingBox() const
{
	AxisAlignedBoundingBox worldBoundingBox;
	worldBoundingBox.MakeReadyForExpansion();

	for (const Vector3& objectVertex : this->hull.GetVertexArray())
	{
		Vector3 worldVertex = this->objectToWorld.TransformPoint(objectVertex);
		worldBoundingBox.Expand(worldVertex);
	}

	return worldBoundingBox;
}

/*virtual*/ bool GJKConvexHull::RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const
{
	PolygonMesh worldHull(this->hull);
	for (Vector3& vertex : worldHull.GetVertexArray())
		vertex = this->objectToWorld.TransformPoint(vertex);

	return worldHull.RayCast(ray, alpha, unitSurfaceNormal);
}

/*virtual*/ void GJKConvexHull::Shift(const Vector3& translation)
{
	for (int i = 0; i < (int)this->hull.GetNumVertices(); i++)
		this->hull.SetVertex(i, this->hull.GetVertex(i) + translation);
}

void GJKConvexHull::GenerateEdgeSet(std::set<Graph::UnorderedEdge, Graph::UnorderedEdge>& edgeSet) const
{
	edgeSet.clear();

	for (const PolygonMesh::Polygon& polygon : this->hull.GetPolygonArray())
	{
		for (int i = 0; i < (int)polygon.vertexArray.size(); i++)
		{
			Graph::UnorderedEdge edge;
			edge.i = polygon.vertexArray[i];
			edge.j = polygon.vertexArray[polygon.Mod(i + 1)];
			if (edgeSet.find(edge) == edgeSet.end())
				edgeSet.insert(edge);
		}
	}
}

void GJKConvexHull::GenerateObjectSpacePlaneArray(std::vector<Plane>& objectSpacePlaneArray) const
{
	objectSpacePlaneArray.clear();

	std::vector<Polygon> standalonePolygonArray;
	this->hull.ToStandalonePolygonArray(standalonePolygonArray);

	for (const Polygon& polygon : standalonePolygonArray)
		objectSpacePlaneArray.push_back(polygon.CalcPlane(true));
}

/*virtual*/ Vector3 GJKConvexHull::CalcGeometricCenter() const
{
	Vector3 center(0.0, 0.0, 0.0);
	for (const Vector3& vertex : this->hull.GetVertexArray())
		center += vertex;

	center /= double(this->hull.GetNumVertices());
	return center;
}

Vector3 GJKConvexHull::GetWorldVertex(int i) const
{
	THEBE_ASSERT(0 <= i && i < (int)this->hull.GetNumVertices());
	return this->objectToWorld.TransformPoint(this->hull.GetVertexArray()[i]);
}

/*virtual*/ bool GJKConvexHull::ContainsObjectPoint(const Vector3& point, void* cache /*= nullptr*/) const
{
	THEBE_ASSERT(cache != nullptr);
	if (!cache)
		return false;

	auto pointContainmentCache = static_cast<PointContainmentCache*>(cache);
	if(pointContainmentCache->planeArray.size() == 0)
	{
		for (const PolygonMesh::Polygon& polygon : this->hull.GetPolygonArray())
		{
			Polygon standalonePolygon;
			polygon.ToStandalonePolygon(standalonePolygon, &this->hull);
			Plane plane = standalonePolygon.CalcPlane(true);
			pointContainmentCache->planeArray.push_back(plane);
		}
	}

	for (const Plane& plane : pointContainmentCache->planeArray)
		if (plane.GetSide(point) == Plane::FRONT)
			return false;

	return true;
}

//------------------------------------- GJKSimplex -------------------------------------

