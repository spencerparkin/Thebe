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

	// GJK is based off the observation that two shapes intersect if and only if their Minkowski difference contains the origin.
	// We start by building an initial simplex (tetrahedron) inside the Minkowski difference between the two shapes.
	// (Leading up to this initial simplex, there may be some early-outs, but I'm just going to ignore those for now.)
	// We then work as follows...
	//   1) If the current simplex contains the origin, the shapes intersect and we're done.
	//   2) Consider making a new simplex by using a face of the current simplex and a new point in the Minkowski difference.
	//      a) If it is determined that no new simplex can be made in the direction of the origin, then there is no intersection and we're done.
	//      b) Make the new simplex and continue with step 1.

	std::vector<Vector3> unitDirectionArray;
	unitDirectionArray.resize(6);
	unitDirectionArray[0].SetComponents(1.0, 0.0, 0.0);
	unitDirectionArray[1].SetComponents(-1.0, 0.0, 0.0);
	unitDirectionArray[2].SetComponents(0.0, 1.0, 0.0);
	unitDirectionArray[3].SetComponents(0.0, -1.0, 0.0);
	unitDirectionArray[4].SetComponents(0.0, 0.0, 1.0);
	unitDirectionArray[5].SetComponents(0.0, 0.0, -1.0);

	GJKSimplex simplex;
	Vector3* unitDirection = unitDirectionArray.data();
	Vector3 pointA, pointB;
	double epsilon = 1e-5;

	// Find the initial simplex.
	int i = 0;
	while (i < 4)
	{
		pointA = shapeA->FurthestPoint(-*unitDirection);
		pointB = shapeB->FurthestPoint(*unitDirection);
		simplex.vertexArray[i] = pointB - pointA;

		double shortestDistance = std::numeric_limits<double>::max();
		for (int j = 0; j < i; j++)
		{
			double distance = (simplex.vertexArray[j] - simplex.vertexArray[i]).Length();
			if (distance < shortestDistance)
				shortestDistance = distance;
		}

		unitDirection++;

		if (shortestDistance > epsilon)
			i++;
		else if (unitDirection - unitDirectionArray.data() == unitDirectionArray.size())
		{
			THEBE_LOG("GJK failed to find initial tetrahedron.");
			return false;
		}
	}

	simplex.MakeFaces();

	struct Candidate
	{
		GJKSimplex newSimplex;
		double dot;
	};

	// Try to walk the simplex to the origin.
	std::set<std::string> faceSet;
	Vector3 origin(0.0, 0.0, 0.0);
	std::vector<Candidate> candidateArray;
	int iterationCount = 0;
	int maxIterationCount = 32;
	while (true)
	{
		if (iterationCount++ >= maxIterationCount)
		{
			// I know for a fact that this can happen with the way I've tried to impliment GJK.
			// Until I learn more about how it actually works, safe-guard against an infinite loop.
			//THEBE_ASSERT(false);
			return false;
		}

		if (simplex.ContainsOrigin())
			return true;

		Plane planeArray[4];
		simplex.CalcFacePlanes(planeArray);

		Vector3 centerArray[4];
		simplex.CalcFaceCenters(centerArray);

		candidateArray.clear();

		for (int i = 0; i < 4; i++)
		{
			const Plane* facePlane = &planeArray[i];
			constexpr double planeThickness = 0.001;
			if (facePlane->GetSide(origin, planeThickness) != Plane::FRONT)
				continue;

			const GJKSimplex::Face* face = &simplex.faceArray[i];

			pointA = shapeA->FurthestPoint(-facePlane->unitNormal);
			pointB = shapeB->FurthestPoint(facePlane->unitNormal);

			// Formulate the simplex such that it *should* have positive area.
			Candidate candidate;
			candidate.newSimplex.vertexArray[0] = simplex.vertexArray[face->vertexArray[2]];
			candidate.newSimplex.vertexArray[1] = simplex.vertexArray[face->vertexArray[1]];
			candidate.newSimplex.vertexArray[2] = simplex.vertexArray[face->vertexArray[0]];
			candidate.newSimplex.vertexArray[3] = pointB - pointA;

			// How far is the new Minkowski point in the direction of the face normal?
			double distanceA = (candidate.newSimplex.vertexArray[3] - centerArray[i]).Dot(facePlane->unitNormal);

			// How far is the origin in the direction of the face normal?
			double distanceB = -centerArray[i].Dot(facePlane->unitNormal);

			// If the new Minkowki point is not at least as far as the origin in the face normal direction, then this simplex won't work.
			if (distanceA < distanceB)
				continue;

			candidate.newSimplex.MakeFaces();

			static double epsilon = 1e-5;
			double simplexVolume = candidate.newSimplex.CalcVolume();
			if (simplexVolume <= epsilon)
				continue;

			candidate.dot = (-centerArray[i].Normalized()).Dot(facePlane->unitNormal);
			candidateArray.push_back(candidate);
			break;
		}

		if (candidateArray.size() == 0)
			break;

		// Chose the candidate simplex that faces most toward the origin.
		const Candidate* chosenCandidate = nullptr;
		for (int i = 0; i < (int)candidateArray.size(); i++)
		{
			const Candidate* candidate = &candidateArray[i];
			if (!chosenCandidate || chosenCandidate->dot < candidate->dot)
				chosenCandidate = candidate;
		}

		THEBE_ASSERT_FATAL(chosenCandidate != nullptr);
		simplex = chosenCandidate->newSimplex;
	}

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

//------------------------------------- GJKSimplex -------------------------------------

GJKSimplex::GJKSimplex()
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
			this->faceArray[i].vertexArray[j] = 0;
}

GJKSimplex::GJKSimplex(const GJKSimplex& simplex)
{
	*this = simplex;
}

/*virtual*/ GJKSimplex::~GJKSimplex()
{
}

void GJKSimplex::operator=(const GJKSimplex& simplex)
{
	for (int i = 0; i < 4; i++)
	{
		this->vertexArray[i] = simplex.vertexArray[i];

		for (int j = 0; j < 3; j++)
			this->faceArray[i].vertexArray[j] = simplex.faceArray[i].vertexArray[j];
	}
}

bool GJKSimplex::ContainsOrigin() const
{
	Plane planeArray[4];
	this->CalcFacePlanes(planeArray);

	Vector3 origin(0.0, 0.0, 0.0);
	for (int i = 0; i < 4; i++)
		if (planeArray[i].GetSide(origin) == Plane::FRONT)
			return false;

	return true;
}

int GJKSimplex::Face::OtherVertex() const
{
	for (int i = 0; i < 4; i++)
		if (!this->HasVertex(i))
			return i;

	THEBE_ASSERT(false);
	return -1;
}

bool GJKSimplex::Face::HasVertex(int i) const
{
	for (int j = 0; j < 3; j++)
		if (this->vertexArray[j] == i)
			return true;

	return false;
}

double GJKSimplex::CalcVolume() const
{
	const Face* face = &this->faceArray[0];

	const Vector3& vertexA = this->vertexArray[face->vertexArray[0]];
	const Vector3& vertexB = this->vertexArray[face->vertexArray[1]];
	const Vector3& vertexC = this->vertexArray[face->vertexArray[2]];
	const Vector3& vertexD = this->vertexArray[face->OtherVertex()];

	double volume = (vertexC - vertexA).Cross(vertexB - vertexA).Dot(vertexD - vertexA) / 6.0;
	return volume;
}

void GJKSimplex::CalcFacePlanes(Plane* planeArray) const
{
	for (int i = 0; i < 4; i++)
	{
		const Face* face = &this->faceArray[i];

		const Vector3& vertexA = this->vertexArray[face->vertexArray[0]];
		const Vector3& vertexB = this->vertexArray[face->vertexArray[1]];
		const Vector3& vertexC = this->vertexArray[face->vertexArray[2]];

		planeArray[i] = Plane(vertexA, (vertexB - vertexA).Cross(vertexC - vertexA).Normalized());
	}
}

void GJKSimplex::CalcFaceCenters(Vector3* centerArray) const
{
	for (int i = 0; i < 4; i++)
	{
		const Face* face = &this->faceArray[i];

		const Vector3& vertexA = this->vertexArray[face->vertexArray[0]];
		const Vector3& vertexB = this->vertexArray[face->vertexArray[1]];
		const Vector3& vertexC = this->vertexArray[face->vertexArray[2]];

		centerArray[i] = (vertexA + vertexB + vertexC) / 3.0;
	}
}

void GJKSimplex::MakeFaces(bool* inverted /*= nullptr*/)
{
	if (inverted)
		*inverted = false;

	this->faceArray[0].vertexArray[0] = 0;
	this->faceArray[0].vertexArray[1] = 1;
	this->faceArray[0].vertexArray[2] = 2;

	this->faceArray[1].vertexArray[0] = 0;
	this->faceArray[1].vertexArray[1] = 2;
	this->faceArray[1].vertexArray[2] = 3;

	this->faceArray[2].vertexArray[0] = 2;
	this->faceArray[2].vertexArray[1] = 1;
	this->faceArray[2].vertexArray[2] = 3;

	this->faceArray[3].vertexArray[0] = 1;
	this->faceArray[3].vertexArray[1] = 0;
	this->faceArray[3].vertexArray[2] = 3;

	if (this->CalcVolume() < 0.0)
	{
		if (inverted)
			*inverted = true;

		this->faceArray[0].vertexArray[1] = 2;
		this->faceArray[0].vertexArray[2] = 1;

		this->faceArray[1].vertexArray[1] = 3;
		this->faceArray[1].vertexArray[2] = 2;

		this->faceArray[2].vertexArray[1] = 3;
		this->faceArray[2].vertexArray[2] = 1;

		this->faceArray[3].vertexArray[1] = 3;
		this->faceArray[3].vertexArray[2] = 0;

		THEBE_ASSERT(this->CalcVolume() >= 0.0);
	}
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