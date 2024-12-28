#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/Polygon.h"
#include "Thebe/Math/Function.h"
#include "Thebe/Log.h"

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

	// Try to walk the simplex to the origin.
	Vector3 origin(0.0, 0.0, 0.0);
	bool newSimplexFound = false;
	do
	{
		if (simplex.ContainsOrigin())
			return true;

		Plane planeArray[4];
		const GJKSimplex::Face* faceArray[4];
		simplex.CalcFacePlanes(planeArray, faceArray);

		newSimplexFound = false;
		for (int i = 0; i < 4; i++)
		{
			const Plane* facePlane = &planeArray[i];
			if (facePlane->GetSide(origin) == Plane::FRONT)
			{
				const GJKSimplex::Face* face = faceArray[i];

				pointA = shapeA->FurthestPoint(-facePlane->unitNormal);
				pointB = shapeB->FurthestPoint(facePlane->unitNormal);

				GJKSimplex newSimplex;
				newSimplex.vertexArray[0] = simplex.vertexArray[face->vertexArray[0]];
				newSimplex.vertexArray[1] = simplex.vertexArray[face->vertexArray[1]];
				newSimplex.vertexArray[2] = simplex.vertexArray[face->vertexArray[2]];
				newSimplex.vertexArray[3] = pointB - pointA;
				newSimplex.MakeFaces();

				static double epsilon = 1e-5;
				if (newSimplex.CalcVolume() > epsilon)
				{
					simplex = newSimplex;
					newSimplexFound = true;
					break;
				}
			}
		}
	} while (newSimplexFound);

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

/*virtual*/ bool GJKShape::CalculateObjectSpaceInertiaTensor(Matrix3x3& objectSpaceInertiaTensor) const
{
	return false;
}

//------------------------------------- GJKSimplex -------------------------------------

GJKSimplex::GJKSimplex()
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
			this->faceArray[i].vertexArray[j] = 0;
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
	this->CalcFacePlanes(planeArray, nullptr);

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

void GJKSimplex::CalcFacePlanes(Plane* planeArray, const Face** givenFaceArray) const
{
	for (int i = 0; i < 4; i++)
	{
		const Face* face = &this->faceArray[i];

		if (givenFaceArray)
			givenFaceArray[i] = face;

		const Vector3& vertexA = this->vertexArray[face->vertexArray[0]];
		const Vector3& vertexB = this->vertexArray[face->vertexArray[1]];
		const Vector3& vertexC = this->vertexArray[face->vertexArray[2]];

		planeArray[i] = Plane(vertexA, (vertexB - vertexA).Cross(vertexC - vertexA).Normalized());
	}
}

void GJKSimplex::MakeFaces()
{
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

/*virtual*/ bool GJKSphere::CalculateObjectSpaceInertiaTensor(Matrix3x3& objectSpaceInertiaTensor) const
{
	double volume = (4.0 / 3.0) * THEBE_PI * this->radius * this->radius * this->radius;
	double diag = (2.0 / 5.0) * volume * this->radius * this->radius;
	objectSpaceInertiaTensor.SetIdentity();
	objectSpaceInertiaTensor.ele[0][0] = diag;
	objectSpaceInertiaTensor.ele[1][1] = diag;
	objectSpaceInertiaTensor.ele[2][2] = diag;
	return true;
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
	return this->hull.RayCast(ray, alpha, unitSurfaceNormal);
}

/*virtual*/ bool GJKConvexHull::CalculateObjectSpaceInertiaTensor(Matrix3x3& objectSpaceInertiaTensor) const
{
	AxisAlignedBoundingBox objectBoundingBox = this->GetObjectBoundingBox();

	for (uint32_t i = 0; i < 3; i++)
		for (uint32_t j = 0; j < 3; j++)
			objectSpaceInertiaTensor.ele[i][j] = 0.0;

	std::vector<Polygon> standalonePolygonArray;
	this->hull.ToStandalonePolygonArray(standalonePolygonArray);

	std::vector<Plane> planeArray;
	for (const Polygon& polygon : standalonePolygonArray)
		planeArray.push_back(polygon.CalcPlane(true));

	auto pointInConvexHull = [&planeArray](const Vector3& point) -> bool
		{
			for (const Plane& plane : planeArray)
				if (plane.GetSide(point) == Plane::FRONT)
					return false;
			return true;
		};

	// This will be a crude approximate of the integrals involved.
	// It is also very slow!  So it really only should be done during the asset build process.
	const uint32_t numSlices = 100;
	double voxelDimension = 1.0 / double(numSlices);
	double voxelVolume = voxelDimension * voxelDimension * voxelDimension;
	Vector3 voxelCenter;
	for (uint32_t i = 0; i < numSlices; i++)
	{
		voxelCenter.x = objectBoundingBox.minCorner.x + ((double(i) + 0.5) / double(numSlices)) * (objectBoundingBox.maxCorner.x - objectBoundingBox.minCorner.x);
		for (uint32_t j = 0; j < numSlices; j++)
		{
			voxelCenter.y = objectBoundingBox.minCorner.y + ((double(j) + 0.5) / double(numSlices)) * (objectBoundingBox.maxCorner.y - objectBoundingBox.minCorner.y);
			for (uint32_t k = 0; k < numSlices; k++)
			{
				voxelCenter.z = objectBoundingBox.minCorner.z + ((double(k) + 0.5) / double(numSlices)) * (objectBoundingBox.maxCorner.z - objectBoundingBox.minCorner.z);
				if (pointInConvexHull(voxelCenter))
				{
					objectSpaceInertiaTensor.ele[0][0] += voxelVolume * (voxelCenter.y * voxelCenter.y + voxelCenter.z * voxelCenter.z);
					objectSpaceInertiaTensor.ele[0][1] += -voxelVolume * voxelCenter.x * voxelCenter.y;
					objectSpaceInertiaTensor.ele[0][2] += -voxelVolume * voxelCenter.x * voxelCenter.z;
					objectSpaceInertiaTensor.ele[1][0] += -voxelVolume * voxelCenter.y * voxelCenter.x;
					objectSpaceInertiaTensor.ele[1][1] += voxelVolume * (voxelCenter.x * voxelCenter.x + voxelCenter.z * voxelCenter.z);
					objectSpaceInertiaTensor.ele[1][2] += -voxelVolume * voxelCenter.y * voxelCenter.z;
					objectSpaceInertiaTensor.ele[2][0] += -voxelVolume * voxelCenter.z * voxelCenter.x;
					objectSpaceInertiaTensor.ele[2][1] += -voxelVolume * voxelCenter.z * voxelCenter.y;
					objectSpaceInertiaTensor.ele[2][2] += voxelVolume * (voxelCenter.x * voxelCenter.x + voxelCenter.y * voxelCenter.y);
				}
			}
		}
	}

	return true;
}