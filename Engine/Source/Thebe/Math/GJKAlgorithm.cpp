#include "Thebe/Math/GJKAlgorithm.h"
#include "Thebe/Math/PolygonMesh.h"

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

	// TODO: Check to see if GJK works even if we don't handle this simple case.
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

	GJKSimplex simplex;
	Vector3 unitDirection;
	Vector3 pointA, pointB;

	unitDirection.SetComponents(1.0, 0.0, 0.0);
	pointA = shapeA->FurthestPoint(unitDirection);
	pointB = shapeB->FurthestPoint(unitDirection);
	simplex.vertexArray[0] = pointB - pointA;

	unitDirection.SetComponents(-1.0, 0.0, 0.0);
	pointA = shapeA->FurthestPoint(unitDirection);
	pointB = shapeB->FurthestPoint(unitDirection);
	simplex.vertexArray[1] = pointB - pointA;

	unitDirection.SetComponents(0.0, 1.0, 0.0);
	pointA = shapeA->FurthestPoint(unitDirection);
	pointB = shapeB->FurthestPoint(unitDirection);
	simplex.vertexArray[2] = pointB - pointA;

	unitDirection.SetComponents(0.0, 0.0, 1.0);
	pointA = shapeA->FurthestPoint(unitDirection);
	pointB = shapeB->FurthestPoint(unitDirection);
	simplex.vertexArray[3] = pointB - pointA;

	simplex.MakeFaces();

	Vector3 origin(0.0, 0.0, 0.0);
	bool newSimplexFound = false;

	// Where I'm really fuzzy here is in the (possibly false) notion that
	// a direction in the Minkowski space corresponds reasonably to the
	// same or similar direction in world space.

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

				pointA = shapeA->FurthestPoint(facePlane->unitNormal);
				pointB = shapeB->FurthestPoint(facePlane->unitNormal);

				GJKSimplex newSimplex;
				newSimplex.vertexArray[0] = simplex.vertexArray[face->vertexArray[0]];
				newSimplex.vertexArray[1] = simplex.vertexArray[face->vertexArray[1]];
				newSimplex.vertexArray[2] = simplex.vertexArray[face->vertexArray[2]];
				newSimplex.vertexArray[3] = pointB - pointA;
				newSimplex.MakeFaces();

				if (newSimplex.CalcVolume() > 0.0)
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

//------------------------------------- GJKSimplex -------------------------------------

GJKSimplex::GJKSimplex()
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
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
		this->faceArray[0].vertexArray[0] = 0;
		this->faceArray[0].vertexArray[1] = 2;
		this->faceArray[0].vertexArray[2] = 1;

		this->faceArray[1].vertexArray[0] = 0;
		this->faceArray[1].vertexArray[1] = 3;
		this->faceArray[1].vertexArray[2] = 2;

		this->faceArray[2].vertexArray[0] = 2;
		this->faceArray[2].vertexArray[1] = 3;
		this->faceArray[2].vertexArray[2] = 1;

		this->faceArray[3].vertexArray[0] = 1;
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
	return this->center + this->radius * unitDirection;
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
	double largestDistance = std::numeric_limits<double>::min();
	const Vector3* chosenVertex = nullptr;

	for (const Vector3& vertex : this->vertexArray)
	{
		double distance = vertex.Dot(unitDirection);
		if (distance > largestDistance)
		{
			largestDistance = distance;
			chosenVertex = &vertex;
		}
	}

	THEBE_ASSERT_FATAL(chosenVertex != nullptr);
	return *chosenVertex;
}

bool GJKConvexHull::CalculateFromPointCloud(const std::vector<Vector3>& pointCloud)
{
	PolygonMesh hull;
	if (!hull.GenerateConvexHull(pointCloud))
		return false;

	this->vertexArray.clear();
	for (const Vector3& vertex : hull.GetVertexArray())
		this->vertexArray.push_back(vertex);

	return true;
}