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

/*static*/ bool GJKShape::Intersect(const GJKShape* shapeA, const GJKShape* shapeB, std::unique_ptr<GJKSimplex>* finalSimplex /*= nullptr*/)
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

	// Note that this debug render stuff is NOT designed to run at full speed.
	// Rather, the idea is to be able to visualize what's going on as you STEP through the code.
	// This should, of course, be disabled for any real-time use of the intersection algorithm.
#if defined GJK_RENDER_DEBUG
	static bool debugDraw = false;
	std::unique_ptr<DebugRenderClient> client;
	if (debugDraw)
	{
		client.reset(new DebugRenderClient());
		bool clientSetup = client->Setup();
		THEBE_ASSERT(clientSetup);
	}
	int simplexCount = 0;
#endif //GJK_RENDER_DEBUG

	Vector3 centerA = shapeA->GetObjectToWorld().TransformPoint(shapeA->CalcGeometricCenter());
	Vector3 centerB = shapeB->GetObjectToWorld().TransformPoint(shapeB->CalcGeometricCenter());

	Vector3 unitDirection = (centerB - centerA).Normalized();

	auto pointSimplex = new GJKPointSimplex();
	pointSimplex->point = GJKSimplex::CalcSupportPoint(shapeA, shapeB, unitDirection);

	double epsilon = THEBE_SMALL_EPS;
	std::unique_ptr<GJKSimplex> simplex(pointSimplex);
	while (simplex.get())
	{
#if defined GJK_RENDER_DEBUG
		if (client.get())
			simplex->DebugDraw(client.get(), simplexCount++);
#endif //GJK_RENDER_DEBUG

		if (simplex->ContainsOrigin(epsilon))
		{
			if (finalSimplex)
				finalSimplex->reset(simplex.release());

			return true;
		}

		simplex.reset(simplex->GenerateSimplex(shapeA, shapeB));
	}

#if defined GJK_RENDER_DEBUG
	if (client.get())
		client->Shutdown();
#endif //GKK_RENDER_DEBUG

	return false;
}

/*static*/ bool GJKShape::Penetration(const GJKShape* shapeA, const GJKShape* shapeB, std::unique_ptr<GJKSimplex>& simplex, Vector3& separationDelta)
{
	separationDelta.SetComponents(0.0, 0.0, 0.0);

	if (!simplex.get())
		return false;

	if (!shapeA || !shapeB)
		return false;

	GJKTetrahedronSimplex* tetrahedron = nullptr;
	while (true)
	{
		tetrahedron = dynamic_cast<GJKTetrahedronSimplex*>(simplex.get());
		if (tetrahedron)
			break;

		simplex.reset(simplex->GenerateSimplex(shapeA, shapeB));
		if (!simplex.get())
			return false;
	}

	ExpandingPolytopeAlgorithm epa;
	ExpandingPolytopeAlgorithm::TypedTriangleFactory<GJKTriangleForEPA> triangleFactory(1024);
	GJKPointSupplierForEPA pointSupplier(shapeA, shapeB, &epa);

	epa.vertexArray.push_back(tetrahedron->vertex[0]);
	epa.vertexArray.push_back(tetrahedron->vertex[1]);
	epa.vertexArray.push_back(tetrahedron->vertex[2]);
	epa.vertexArray.push_back(tetrahedron->vertex[3]);

	for (int i = 0; i < 4; i++)
	{
		int vertices[3];
		int k = 0;
		for (int j = 0; j < 4; j++)
			if (j != i)
				vertices[k++] = j;

		auto triangle = (GJKTriangleForEPA*)triangleFactory.AllocTriangle(vertices[0], vertices[1], vertices[2]);
		Plane trianglePlane = triangle->MakePlane(epa.vertexArray);
		if (trianglePlane.GetSide(epa.vertexArray[i], epa.planeThickness) != Plane::Side::BACK)
		{
			auto reverseTriangle = (GJKTriangleForEPA*)triangle->Reversed(&triangleFactory);
			triangleFactory.FreeTriangle(triangle);
			triangle = reverseTriangle;
		}

		epa.triangleList.push_back(triangle);
	}

	if (!epa.Expand(&pointSupplier, &triangleFactory))
		return false;

	double shortestDistance = std::numeric_limits<double>::max();
	for (auto triangle : epa.triangleList)
	{
		Plane trianglePlane = triangle->MakePlane(epa.vertexArray);
		Vector3 planePoint = trianglePlane.ClosestPointTo(Vector3::Zero());
		double distance = planePoint.Length();
		if (distance < shortestDistance)
		{
			shortestDistance = distance;
			separationDelta = planePoint;
		}
	}

	return true;
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

//------------------------------------- GJKPointSupplierForEPA -------------------------------------

GJKPointSupplierForEPA::GJKPointSupplierForEPA(const GJKShape* shapeA, const GJKShape* shapeB, ExpandingPolytopeAlgorithm* epa)
{
	this->shapeA = shapeA;
	this->shapeB = shapeB;
	this->epa = epa;
}

/*virtual*/ GJKPointSupplierForEPA::~GJKPointSupplierForEPA()
{
}

/*virtual*/ bool GJKPointSupplierForEPA::GetNextPoint(Vector3& point)
{
	// New triangles are added at the end of the list and those are most likely to be
	// unchecked, so start our search at the end and work toward the start.
	for (std::list<ExpandingPolytopeAlgorithm::Triangle*>::reverse_iterator iter = this->epa->triangleList.rbegin(); iter != this->epa->triangleList.rend(); ++iter)
	{
		auto triangle = (GJKTriangleForEPA*)*iter;
		if (!triangle->onEdgeOfMinkowskiHull)
		{
			Plane trianglePlane = triangle->MakePlane(this->epa->vertexArray);
			Vector3 supportPoint = GJKSimplex::CalcSupportPoint(this->shapeA, this->shapeB, trianglePlane.unitNormal);
			if (trianglePlane.GetSide(supportPoint, this->epa->planeThickness) == Plane::Side::FRONT)
			{
				point = supportPoint;
				return true;
			}

			triangle->onEdgeOfMinkowskiHull = true;
		}
	}

	return false;
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

GJKSimplex::GJKSimplex()
{
}

/*virtual*/ GJKSimplex::~GJKSimplex()
{
}

/*static*/ Vector3 GJKSimplex::CalcSupportPoint(const GJKShape* shapeA, const GJKShape* shapeB, const Vector3& unitDirection)
{
	return shapeB->FurthestPoint(unitDirection) - shapeA->FurthestPoint(-unitDirection);
}

//------------------------------------- GJKPointSimplex -------------------------------------

GJKPointSimplex::GJKPointSimplex()
{
}

/*virtual*/ GJKPointSimplex::~GJKPointSimplex()
{
}

/*virtual*/ bool GJKPointSimplex::ContainsOrigin(double epsilon) const
{
	double squareLength = this->point.SquareLength();
	return squareLength <= epsilon * epsilon;
}

/*virtual*/ GJKSimplex* GJKPointSimplex::GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const
{
	Vector3 unitDirection = -this->point.Normalized();

	std::unique_ptr<GJKLineSimplex> lineSimplex(new GJKLineSimplex());
	lineSimplex->lineSegment.point[0] = this->point;
	lineSimplex->lineSegment.point[1] = this->CalcSupportPoint(shapeA, shapeB, unitDirection);

	Vector3 lineDelta = lineSimplex->lineSegment.GetDelta();
	double distanceA = lineDelta.Length();
	Vector3 unitLineDirection = lineDelta / distanceA;
	double distanceB = (-this->point).Dot(unitLineDirection);
	if (distanceA < distanceB)
		return nullptr;

	return lineSimplex.release();
}

#if defined GJK_RENDER_DEBUG
/*virtual*/ void GJKPointSimplex::DebugDraw(DebugRenderClient* client, int simplexNumber)
{
	client->AddLine(std::format("simplex{}", simplexNumber), this->point - Vector3::XAxis() * 0.1, this->point + Vector3::XAxis() * 0.1, Vector3(1.0, 0.0, 0.0));
	client->AddLine(std::format("simplex{}", simplexNumber), this->point - Vector3::YAxis() * 0.1, this->point + Vector3::YAxis() * 0.1, Vector3(0.0, 1.0, 0.0));
	client->AddLine(std::format("simplex{}", simplexNumber), this->point - Vector3::ZAxis() * 0.1, this->point + Vector3::ZAxis() * 0.1, Vector3(0.0, 0.0, 1.0));
}
#endif //GJK_RENDER_DEBUG

//------------------------------------- GJKLineSimplex -------------------------------------

GJKLineSimplex::GJKLineSimplex()
{
}

/*virtual*/ GJKLineSimplex::~GJKLineSimplex()
{
}

/*virtual*/ bool GJKLineSimplex::ContainsOrigin(double epsilon) const
{
	return this->lineSegment.ContainsPoint(Vector3::Zero(), nullptr, epsilon);
}

/*virtual*/ GJKSimplex* GJKLineSimplex::GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const
{
	// Note that if the origin was on the infinite line containing this line,
	// then the next simplex we should return would be a GJKSimplexPoint.
	// I don't think we really need to handle this case, though.

	Vector3 linePoint = this->lineSegment.ClosestPointTo(Vector3::Zero(), true);

	double distanceA = linePoint.Length();
	Vector3 unitDirection = -linePoint / distanceA;
	Vector3 supportPoint = this->CalcSupportPoint(shapeA, shapeB, unitDirection);
	double distanceB = (supportPoint - linePoint).Dot(unitDirection);
	if (distanceB < distanceA)
		return nullptr;

	auto triangle = new GJKTriangleSimplex();
	triangle->vertex[0] = this->lineSegment.point[0];
	triangle->vertex[1] = this->lineSegment.point[1];
	triangle->vertex[2] = supportPoint;
	return triangle;
}

#if defined GJK_RENDER_DEBUG
/*virtual*/ void GJKLineSimplex::DebugDraw(DebugRenderClient* client, int simplexNumber)
{
	client->AddLine(std::format("simplex{}", simplexNumber), this->lineSegment.point[0], this->lineSegment.point[1], Vector3(1.0, 0.0, 0.0));
}
#endif //GJK_RENDER_DEBUG

//------------------------------------- GJKTriangleSimplex -------------------------------------

GJKTriangleSimplex::GJKTriangleSimplex()
{
	this->originOnPlane = false;
	this->originDistanceToTrianglePlane = 0.0;
}

/*virtual*/ GJKTriangleSimplex::~GJKTriangleSimplex()
{
}

/*virtual*/ bool GJKTriangleSimplex::ContainsOrigin(double epsilon) const
{
	this->trianglePlane = Plane(this->vertex[0], this->vertex[1], this->vertex[2]);
	this->originDistanceToTrianglePlane = ::fabs(this->trianglePlane.SignedDistanceTo(Vector3::Zero()));
	if (this->originDistanceToTrianglePlane > epsilon)
		return false;
	
	this->originOnPlane = true;

	for (int i = 0; i < 3; i++)
	{
		Vector3 edgeVector = this->vertex[(i + 1) % 3] - this->vertex[i];
		Vector3 unitEdgePlaneNormal = edgeVector.Cross(this->trianglePlane.unitNormal).Normalized();
		this->edgePlane[i] = Plane(this->vertex[i], unitEdgePlaneNormal);
	}

	for (int i = 0; i < 3; i++)
	{
		double distanceToPlane = this->edgePlane[i].SignedDistanceTo(Vector3::Zero());
		if (distanceToPlane > epsilon)
			return false;
	}

	return true;
}

/*virtual*/ GJKSimplex* GJKTriangleSimplex::GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const
{
	if (this->originOnPlane)
	{
		for (int i = 0; i < 3; i++)
		{
			const Plane& planeA = this->edgePlane[i];
			const Plane& planeB = this->edgePlane[(i + 1) % 3];
			if (planeA.GetSide(Vector3::Zero()) == Plane::FRONT && planeB.GetSide(Vector3::Zero()) == Plane::FRONT)
			{
				auto pointSimplex = new GJKPointSimplex();
				pointSimplex->point = this->vertex[(i + 1) % 3];
				return pointSimplex;
			}
		}

		for (int i = 0; i < 3; i++)
		{
			if (this->edgePlane[i].GetSide(Vector3::Zero()) == Plane::FRONT)
			{
				auto lineSimplex = new GJKLineSimplex();
				lineSimplex->lineSegment.point[0] = this->vertex[i];
				lineSimplex->lineSegment.point[1] = this->vertex[(i + 1) % 3];
				return lineSimplex;
			}
		}

		return nullptr;
	}

	if (this->trianglePlane.SignedDistanceTo(Vector3::Zero()) < 0.0)
		this->trianglePlane.unitNormal = -this->trianglePlane.unitNormal;

	Vector3 supportPoint = this->CalcSupportPoint(shapeA, shapeB, this->trianglePlane.unitNormal);
	if (this->trianglePlane.SignedDistanceTo(supportPoint) <= 0.0)
		return nullptr;

	Vector3 projectedPoint = this->trianglePlane.ClosestPointTo(supportPoint);
	double distance = (supportPoint - projectedPoint).Length();
	static double eps = 0.01;
	if (distance < this->originDistanceToTrianglePlane + eps)
		return nullptr;

	auto tetrahedron = new GJKTetrahedronSimplex();
	tetrahedron->vertex[0] = this->vertex[0];
	tetrahedron->vertex[1] = this->vertex[1];
	tetrahedron->vertex[2] = this->vertex[2];
	tetrahedron->vertex[3] = supportPoint;
	return tetrahedron;
}

#if defined GJK_RENDER_DEBUG
/*virtual*/ void GJKTriangleSimplex::DebugDraw(DebugRenderClient* client, int simplexNumber)
{
	for (int i = 0; i < 3; i++)
	{
		int j = (i + 1) % 3;
		client->AddLine(std::format("simplex{}", simplexNumber), this->vertex[i], this->vertex[j], Vector3(0.0, 1.0, 0.0));
	}
}
#endif //GJK_RENDER_DEBUG

//------------------------------------- GJKTetrahedronSimplex -------------------------------------

Random GJKTetrahedronSimplex::random;

GJKTetrahedronSimplex::GJKTetrahedronSimplex()
{
}

/*virtual*/ GJKTetrahedronSimplex::~GJKTetrahedronSimplex()
{
}

/*virtual*/ bool GJKTetrahedronSimplex::ContainsOrigin(double epsilon) const
{
	for (int i = 0; i < 4; i++)
	{
		int k = 0;
		Vector3 faceVertex[3];
		for (int j = 0; j < 4; j++)
			if (j != i)
				faceVertex[k++] = this->vertex[j];

		this->facePlane[i] = Plane(faceVertex[0], faceVertex[1], faceVertex[2]);
		if (this->facePlane[i].SignedDistanceTo(this->vertex[i]) > 0.0)
			this->facePlane[i].unitNormal = -this->facePlane[i].unitNormal;
	}

	for (int i = 0; i < 4; i++)
	{
		double distance = this->facePlane[i].SignedDistanceTo(Vector3::Zero());
		if (distance > epsilon)
			return false;
	}

	return true;
}

/*virtual*/ GJKSimplex* GJKTetrahedronSimplex::GenerateSimplex(const GJKShape* shapeA, const GJKShape* shapeB) const
{
	std::vector<const Plane*> visiblePlaneArray;
	visiblePlaneArray.reserve(4);
	for (int i = 0; i < 4; i++)
	{
		if (this->facePlane[i].GetSide(Vector3::Zero()) == Plane::FRONT)
			visiblePlaneArray.push_back(&this->facePlane[i]);
	}

	THEBE_ASSERT(1 <= visiblePlaneArray.size() && visiblePlaneArray.size() <= 3);

	// Making a random choice here is severely unsatisfying.  However, until I understand
	// the GJK algorithm better, this is the only way I know how to proceed.
	int j = this->random.InRange(0, visiblePlaneArray.size() - 1);
	const Plane* chosenPlane = visiblePlaneArray[j];
	std::vector<Vector3> triangleVertexArray;
	triangleVertexArray.reserve(3);
	for (int i = 0; i < 4; i++)
		if (chosenPlane->GetSide(this->vertex[i], THEBE_SMALL_EPS) == Plane::Side::NEITHER)
			triangleVertexArray.push_back(this->vertex[i]);
	
	THEBE_ASSERT(triangleVertexArray.size() == 3);
	auto triangle = new GJKTriangleSimplex();
	triangle->vertex[0] = triangleVertexArray[0];
	triangle->vertex[1] = triangleVertexArray[1];
	triangle->vertex[2] = triangleVertexArray[2];
	return triangle;
}

#if defined GJK_RENDER_DEBUG
/*virtual*/ void GJKTetrahedronSimplex::DebugDraw(DebugRenderClient* client, int simplexNumber)
{
	for (int i = 0; i < 4; i++)
		for (int j = i + 1; j < 4; j++)
			client->AddLine(std::format("simplex{}", simplexNumber), this->vertex[i], this->vertex[j], Vector3(0.0, 0.0, 1.0));
}
#endif //GJK_RENDER_DEBUG