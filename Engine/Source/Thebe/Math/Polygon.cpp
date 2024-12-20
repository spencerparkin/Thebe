#include "Thebe/Math/Polygon.h"
#include "Thebe/Math/Ray.h"
#include "Thebe/Math/Interval.h"
#include "Thebe/Math/LineSegment.h"
#include "Thebe/Math/Matrix3x3.h"
#include <algorithm>
#include <unordered_set>

using namespace Thebe;

Polygon::Polygon()
{
}

Polygon::Polygon(const Polygon& polygon)
{
	*this = polygon;
}

/*virtual*/ Polygon::~Polygon()
{
}

void Polygon::operator=(const Polygon& polygon)
{
	this->vertexArray.clear();
	for (const Vector3& vertex : polygon.vertexArray)
		this->vertexArray.push_back(vertex);
}

bool Polygon::IsValid(double tolerance /*= 1e-4*/) const
{
	if (this->vertexArray.size() < 3)
		return false;

	for (const Vector3& vertex : this->vertexArray)
		if (!vertex.IsValid())
			return false;

	Plane plane = this->CalcPlane();
	for (const Vector3& vertex : this->vertexArray)
	{
		double distance = plane.SignedDistanceTo(vertex);
		if (::fabs(distance) >= tolerance)
			return false;
	}

	return true;
}

void Polygon::Clear()
{
	this->vertexArray.clear();
}

bool Polygon::IsConvex(ConvexityInfo* convexityInfo /*= nullptr*/, double tolerance /*= 1e-4*/) const
{
	ConvexityInfo convexityInfoStorage;
	if (!convexityInfo)
		convexityInfo = &convexityInfoStorage;

	convexityInfo->convexVertexArray.clear();
	convexityInfo->concaveVertexArray.clear();

	Plane plane = this->CalcPlane(false);

	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		const Vector3& vertexPrev = this->vertexArray[this->Mod(i - 1)];
		const Vector3& vertex = this->vertexArray[i];
		const Vector3& vertexNext = this->vertexArray[this->Mod(i + 1)];

		Vector3 vectorPrev = (vertexPrev - vertex).Normalized();
		Vector3 vectorNext = (vertexNext - vertex).Normalized();

		THEBE_ASSERT(vectorPrev.IsValid());
		THEBE_ASSERT(vectorNext.IsValid());

		double angle = vectorPrev.AngleBetween(vectorNext, plane.unitNormal);
		if (angle < M_PI)
			convexityInfo->concaveVertexArray.push_back(i);
		else
			convexityInfo->convexVertexArray.push_back(i);
	}

	return convexityInfo->concaveVertexArray.size() == 0;
}

Plane Polygon::CalcPlane(bool assumeConvex /*= false*/) const
{
	THEBE_ASSERT(this->vertexArray.size() > 0);

	// Calculate the most numerically stable normal we can find.
	Vector3 bestNormal;
	double maxLength = 0.0;
	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		const Vector3& vertexPrev = this->vertexArray[this->Mod(i - 1)];
		const Vector3& vertex = this->vertexArray[i];
		const Vector3& vertexNext = this->vertexArray[this->Mod(i + 1)];

		Vector3 vectorPrev = vertexPrev - vertex;
		Vector3 vectorNext = vertexNext - vertex;

		Vector3 normal = vectorNext.Cross(vectorPrev);
		double length = normal.Length();
		if (length > maxLength)
		{
			maxLength = length;
			bestNormal = normal;
		}
	}

	// At this point we have the correct normal, up to sign.
	// What remains is to determine the sign of the normal.
	bestNormal.Normalize();
	if (!assumeConvex)
	{
		double netAngle = 0.0;
		for (int i = 0; i < (signed)this->vertexArray.size(); i++)
		{
			const Vector3& vertexPrev = this->vertexArray[this->Mod(i - 1)];
			const Vector3& vertex = this->vertexArray[i];
			const Vector3& vertexNext = this->vertexArray[this->Mod(i + 1)];

			Vector3 vectorPrev = (vertex - vertexPrev).Normalized();
			Vector3 vectorNext = (vertexNext - vertex).Normalized();

			double turnAngle = vectorPrev.AngleBetween(vectorNext, bestNormal);
			if (turnAngle > M_PI)
				turnAngle -= 2.0 * M_PI;

			netAngle += turnAngle;
		}

		if (netAngle < 0.0)
			bestNormal = -bestNormal;
	}

	return Plane(this->vertexArray[0], bestNormal);
}

Vector3 Polygon::CalcCenter() const
{
	Vector3 center(0.0, 0.0, 0.0);

	if (this->vertexArray.size() > 0)
	{
		for (const Vector3& vertex : this->vertexArray)
			center += vertex;

		center /= double(this->vertexArray.size());
	}

	return center;
}

double Polygon::Area(bool assumeConvex /*= true*/) const
{
	double area = 0.0;

	if (assumeConvex)
	{
		Vector3 center = this->CalcCenter();

		for (int i = 0; i < (signed)this->vertexArray.size(); i++)
		{
			int j = (i + 1) % this->vertexArray.size();

			const Vector3& vertexA = this->vertexArray[i];
			const Vector3& vertexB = this->vertexArray[j];

			area += (vertexA - center).Cross(vertexB - center).Length() / 2.0;
		}
	}
	else
	{
		std::vector<Polygon> polygonArray;
		if (this->TessellateUntilConvex(polygonArray))
		{
			for (const Polygon& convexPolygon : polygonArray)
				area += convexPolygon.Area(true);
		}
	}

	return area;
}

bool Polygon::SplitAgainstPlane(const Plane& plane, Polygon& backPolygon, Polygon& frontPolygon, double planeThickness /*= 1e-6*/) const
{
	backPolygon.vertexArray.clear();
	frontPolygon.vertexArray.clear();

	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		int j = (i + 1) % this->vertexArray.size();

		const Vector3& vertexA = this->vertexArray[i];
		const Vector3& vertexB = this->vertexArray[j];

		Plane::Side side = plane.GetSide(vertexA, planeThickness);

		if (side == Plane::Side::BACK || side == Plane::Side::NEITHER)
			backPolygon.vertexArray.push_back(vertexA);
		if (side == Plane::Side::FRONT || side == Plane::Side::NEITHER)
			frontPolygon.vertexArray.push_back(vertexA);

		double edgeLength = 0.0;
		Ray ray(vertexA, vertexB - vertexA);
		ray.unitDirection.Normalize(&edgeLength);
		double alpha = 0.0;
		Interval interval(0.0, edgeLength);

		if (ray.CastAgainst(plane, alpha) && interval.ContainsInteriorValue(alpha))
		{
			Vector3 intersectionPoint = ray.CalculatePoint(alpha);
			backPolygon.vertexArray.push_back(intersectionPoint);
			frontPolygon.vertexArray.push_back(intersectionPoint);
		}
	}

	if (backPolygon.vertexArray.size() < 3 || frontPolygon.vertexArray.size() < 3)
		return false;

	return true;
}

bool Polygon::ContainsPoint(const Vector3& point, double tolerance /*= 1e-5*/, bool* isInterior /*= nullptr*/) const
{
	if (isInterior)
		*isInterior = false;

	Plane plane = this->CalcPlane(true);
	if (plane.GetSide(point, tolerance) != Plane::Side::NEITHER)
		return false;

	if (this->ContainsPointOnEdge(point, tolerance))
		return true;

	std::vector<LineSegment> edgeArray;
	this->GetEdges(edgeArray);
	for (const LineSegment& edgeSegment : edgeArray)
	{
		const Vector3& vertexA = edgeSegment.point[0];
		const Vector3& vertexB = edgeSegment.point[1];

		double determinant = (vertexA - point).Cross(vertexB - point).Dot(plane.unitNormal);
		if (determinant < 0.0)
			return false;
	}

	if (isInterior)
		*isInterior = true;

	return true;
}

bool Polygon::ContainsPointOnEdge(const Vector3& point, double tolerance /*= 1e-5*/) const
{
	std::vector<LineSegment> edgeArray;
	this->GetEdges(edgeArray);
	for (const LineSegment& edgeSegment : edgeArray)
		if (edgeSegment.ShortestDistanceTo(point) < tolerance)
			return true;

	return false;
}

void Polygon::GetEdges(std::vector<LineSegment>& edgeArray) const
{
	edgeArray.clear();
	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		int j = (i + 1) % this->vertexArray.size();

		const Vector3& pointA = this->vertexArray[i];
		const Vector3& pointB = this->vertexArray[j];

		edgeArray.push_back(LineSegment(pointA, pointB));
	}
}

Vector3 Polygon::ClosestPointTo(const Vector3& point) const
{
	Plane plane = this->CalcPlane();
	Vector3 closestPoint = plane.ClosestPointTo(point);
	if (this->ContainsPoint(closestPoint))
		return closestPoint;

	double smallestDistance = std::numeric_limits<double>::max();
	std::vector<LineSegment> edgeArray;
	this->GetEdges(edgeArray);
	for (const LineSegment& edge : edgeArray)
	{
		Vector3 edgePoint = edge.ClosestPointTo(point);
		double distance = (point - edgePoint).Length();
		if (distance < smallestDistance)
		{
			smallestDistance = distance;
			closestPoint = edgePoint;
		}
	}

	return closestPoint;
}

bool Polygon::IntersectsWith(const LineSegment& lineSegment, Vector3& intersectionPoint) const
{
	Vector3 unitDirection = lineSegment.GetDelta();
	double length = 0.0;
	if (!unitDirection.Normalize(&length))
		return false;

	Ray ray(lineSegment.point[0], unitDirection);
	Plane plane = this->CalcPlane();
	double alpha = 0.0;
	if (!ray.CastAgainst(plane, alpha))
		return false;

	if (alpha < 0.0 || alpha > length)
		return false;

	intersectionPoint = ray.CalculatePoint(alpha);
	return this->ContainsPoint(intersectionPoint);
}

// TODO: I think this may have a bug in it.
bool Polygon::CalculatePlaneOfBestFit(Plane& plane) const
{
	plane.center = Vector3(0.0, 0.0, 0.0);
	plane.unitNormal = Vector3(0.0, 0.0, 1.0);

	double sum_x = 0.0;
	double sum_y = 0.0;
	double sum_z = 0.0;
	double sum_xx = 0.0;
	double sum_xy = 0.0;
	double sum_xz = 0.0;
	double sum_yy = 0.0;
	double sum_yz = 0.0;
	double sum_zz = 0.0;

	for (const Vector3& point : this->vertexArray)
	{
		sum_x += point.x;
		sum_y += point.y;
		sum_z += point.z;
		sum_xx += point.x * point.x;
		sum_xy += point.x * point.y;
		sum_xz += point.x * point.z;
		sum_yy += point.y * point.y;
		sum_yz += point.y * point.z;
		sum_zz += point.z * point.z;
	}

	Matrix3x3 matrixXY, matrixXZ, matrixYZ;

	// z = ax + by + c
	matrixXY.ele[0][0] = sum_xx;
	matrixXY.ele[0][1] = sum_xy;
	matrixXY.ele[0][1] = sum_x;
	matrixXY.ele[1][0] = sum_xy;
	matrixXY.ele[1][1] = sum_yy;
	matrixXY.ele[1][2] = sum_y;
	matrixXY.ele[2][0] = sum_x;
	matrixXY.ele[2][1] = sum_y;
	matrixXY.ele[2][2] = double(this->vertexArray.size());

	// y = ax + bz + c
	matrixXZ.ele[0][0] = sum_xx;
	matrixXZ.ele[0][1] = sum_xz;
	matrixXZ.ele[0][2] = sum_x;
	matrixXZ.ele[1][0] = sum_xz;
	matrixXZ.ele[1][1] = sum_zz;
	matrixXZ.ele[1][2] = sum_z;
	matrixXZ.ele[2][0] = sum_x;
	matrixXZ.ele[2][1] = sum_z;
	matrixXZ.ele[2][2] = double(this->vertexArray.size());

	// x = ay + bz + c
	matrixYZ.ele[0][0] = sum_yy;
	matrixYZ.ele[0][1] = sum_yz;
	matrixYZ.ele[0][2] = sum_y;
	matrixYZ.ele[1][0] = sum_yz;
	matrixYZ.ele[1][1] = sum_zz;
	matrixYZ.ele[1][2] = sum_z;
	matrixYZ.ele[2][0] = sum_y;
	matrixYZ.ele[2][1] = sum_z;
	matrixYZ.ele[2][2] = double(this->vertexArray.size());

	double volXY = ::abs(matrixXY.Determinant());
	double volXZ = ::abs(matrixXZ.Determinant());
	double volYZ = ::abs(matrixYZ.Determinant());

	double a, b, c;
	Vector3 normal;

	if (volXY >= volXZ && volXY >= volYZ)
	{
		Matrix3x3 matrixInv;
		if (!matrixInv.Invert(matrixXY))
			return false;

		Vector3 coeficients = matrixInv * Vector3(sum_xz, sum_yz, sum_z);

		a = coeficients.x;
		b = coeficients.y;
		c = coeficients.z;

		normal = Vector3(a, b, -1.0);
	}
	else if (volXZ >= volXY && volXZ >= volYZ)
	{
		Matrix3x3 matrixInv;
		if (!matrixInv.Invert(matrixXZ))
			return false;

		Vector3 coeficients = matrixInv * Vector3(sum_xy, sum_yz, sum_y);

		a = coeficients.x;
		b = coeficients.y;
		c = coeficients.z;

		normal = Vector3(a, -1.0, b);
	}
	else //if (volYZ > volXY && volYZ > volXZ)
	{
		Matrix3x3 matrixInv;
		if (!matrixInv.Invert(matrixYZ))
			return false;

		Vector3 coeficients = matrixInv * Vector3(sum_xy, sum_xz, sum_x);

		a = coeficients.x;
		b = coeficients.y;
		c = coeficients.z;

		normal = Vector3(-1.0, a, b);
	}

	double length = 0.0;
	if (!normal.Normalize(&length))
		return false;

	plane.unitNormal = normal;
	plane.center = -normal * c / length;
	return true;
}

void Polygon::SnapToPlane(const Plane& plane)
{
	for (Vector3& vertex : this->vertexArray)
	{
		double distance = plane.SignedDistanceTo(vertex);
		vertex -= plane.unitNormal * distance;
	}
}

void Polygon::SetAsConvexHull(const std::vector<Vector3>& pointCloud)
{
	Polygon polygon;
	for (const Vector3& point : pointCloud)
		polygon.vertexArray.push_back(point);

	Plane plane;
	polygon.CalculatePlaneOfBestFit(plane);
	polygon.SnapToPlane(plane);

	// This can be expensive, and there is an algorithm with better time-complexity, but do this for now.
	// We don't want there to be any redundant points in the list.
	std::vector<Vector3> planarPointCloud;
	for (const Vector3& vertex : polygon.vertexArray)
		if (vertex.IsAnyPoint(planarPointCloud, 1e-5))
			planarPointCloud.push_back(vertex);

	this->CalculateConvexHullInternal(planarPointCloud, plane);
}

void Polygon::FixWindingOfTriangle(const Vector3& desiredNormal)
{
	if (this->vertexArray.size() == 3)
	{
		const Vector3& pointA = this->vertexArray[0];
		const Vector3& pointB = this->vertexArray[1];
		const Vector3& pointC = this->vertexArray[2];

		double determinant = (pointB - pointA).Cross(pointC - pointA).Dot(desiredNormal);
		if (determinant < 0.0)
		{
			this->vertexArray[0] = pointA;
			this->vertexArray[1] = pointC;
			this->vertexArray[2] = pointB;
		}
	}
}

// TODO: Test this.  It has not yet been tested, and so it surely doesn't work yet.
void Polygon::CalculateConvexHullInternal(const std::vector<Vector3>& planarPointCloud, const Plane& plane)
{
	this->vertexArray.clear();

	// Handle the trivial cases first.  If they don't apply, then we divide and conquer.
	if (planarPointCloud.size() <= 3)
	{
		for (const Vector3& point : planarPointCloud)
			this->vertexArray.push_back(point);

		this->FixWindingOfTriangle(plane.unitNormal);
		return;
	}

	// Find the center of the planar point-cloud.
	Vector3 center(0.0, 0.0, 0.0);
	for (const Vector3& point : planarPointCloud)
		center += point;
	center /= double(planarPointCloud.size());

	// Find a point furthest from the center.
	double largestDistance = 0.0;
	Vector3 furthestPoint;
	for (const Vector3& point : planarPointCloud)
	{
		double distance = (point - center).Length();
		if (distance > largestDistance)
		{
			largestDistance = distance;
			furthestPoint = point;
		}
	}

	// I'm guessing this might be a good dividing plane.
	Plane dividingPlane;
	dividingPlane.center = center;
	dividingPlane.unitNormal = (furthestPoint - center).Normalized();

	// Break the planar-point-cloud into two smaller such sets.
	std::vector<Vector3> planarPointCloudA, planarPointCloudB;
	for (const Vector3& point : planarPointCloud)
	{
		double signedDistance = dividingPlane.SignedDistanceTo(point);
		if (signedDistance < 0.0)
			planarPointCloudA.push_back(point);
		else
			planarPointCloudB.push_back(point);
	}

	THEBE_ASSERT(planarPointCloudA.size() > 0);
	THEBE_ASSERT(planarPointCloudB.size() > 0);

	// Divided, go conquer.
	Polygon polygonA, polygonB;
	polygonA.CalculateConvexHullInternal(planarPointCloudA, plane);
	polygonB.CalculateConvexHullInternal(planarPointCloudB, plane);

	// The secret sauce is now the ability to simply combine two convex hulls into one.
	for (const Vector3& vertex : polygonA.vertexArray)
		this->vertexArray.push_back(vertex);
	for (const Vector3& vertex : polygonB.vertexArray)
		this->vertexArray.push_back(vertex);

	// Handle some trivial cases first.
	if (polygonA.vertexArray.size() + polygonB.vertexArray.size() <= 3)
	{
		this->FixWindingOfTriangle(plane.unitNormal);
		return;
	}

	// Make a combined list of all edges concerned.  Note that the order of edges here is important.
	// It doesn't matter that we started with polygonA, then did polygonB.  What matters is that
	// generally, the edges go in a counter-clock-wise direction.
	std::list<LineSegment> edgeList;
	for (int i = 0; i < (signed)polygonA.vertexArray.size(); i++)
		edgeList.push_back(LineSegment(polygonA.vertexArray[i], polygonA.vertexArray[i + 1]));
	for (int i = 0; i < (signed)polygonB.vertexArray.size(); i++)
		edgeList.push_back(LineSegment(polygonB.vertexArray[i], polygonB.vertexArray[i + 1]));

	// Cull those that are not on the convex hull.
	std::list<LineSegment>::iterator iter = edgeList.begin();
	while (iter != edgeList.end())
	{
		std::list<LineSegment>::iterator nextIter(iter);
		nextIter++;

		const LineSegment& edge = *iter;

		Plane edgePlane;
		edgePlane.center = edge.point[0];
		edgePlane.unitNormal = (edge.point[1] - edge.point[0]).Cross(plane.unitNormal).Normalized();

		if (edgePlane.AnyPointOnSide(this->vertexArray, Plane::Side::BACK))
			edgeList.erase(iter);

		iter = nextIter;
	}

	// Wipe the polygon; we're about to construct it.
	this->vertexArray.clear();

	// Walk edges, making two new ones in the process.
	iter = edgeList.begin();
	while (iter != edgeList.end())
	{
		std::list<LineSegment>::iterator nextIter(iter);
		nextIter++;

		const LineSegment& edge = *iter;
		this->vertexArray.push_back(edge.point[0]);

		if (nextIter == edgeList.end())
			break;

		const LineSegment& nextEdge = *nextIter;
		if (!edge.point[1].IsPoint(nextEdge.point[0]))
			this->vertexArray.push_back(edge.point[1]);

		iter = nextIter;
	}
}

bool Polygon::RayCast(const Ray& ray, double& alpha, Vector3& unitSurfaceNormal) const
{
	if (this->ContainsPoint(ray.origin))
		return false;

	Plane plane = this->CalcPlane(true);
	if (!ray.CastAgainst(plane, alpha) || alpha < 0.0)
		return false;

	Vector3 hitPoint = ray.CalculatePoint(alpha);
	if (!this->ContainsPoint(hitPoint))
		return false;

	unitSurfaceNormal = plane.unitNormal;
	if (unitSurfaceNormal.Dot(ray.unitDirection) > 0.0)
		unitSurfaceNormal = -unitSurfaceNormal;

	return true;
}

/*static*/ void Polygon::Compress(std::vector<Polygon>& polygonArray, bool mustBeConvex, bool sanityCheck /*= false*/)
{
	std::list<Polygon> polygonQueue;
	for (Polygon& polygon : polygonArray)
		polygonQueue.push_back(polygon);

	polygonArray.clear();

	while (polygonQueue.size() > 0)
	{
		std::list<Polygon>::iterator iter = polygonQueue.begin();
		std::vector<Polygon> coplanarPolygonArray;
		coplanarPolygonArray.push_back(*iter);
		polygonQueue.erase(iter);

		Plane plane = coplanarPolygonArray[0].CalcPlane();

		iter = polygonQueue.begin();
		while (iter != polygonQueue.end())
		{
			std::list<Polygon>::iterator nextIter(iter);
			nextIter++;

			Thebe::Plane otherPlane = (*iter).CalcPlane();
			if (otherPlane.IsPlane(plane))
			{
				coplanarPolygonArray.push_back(*iter);
				polygonQueue.erase(iter);
			}

			iter = nextIter;
		}

		double totalAreaBeforehand = 0.0;
		double totalAreaAfterword = 0.0;

		if (sanityCheck)
			for (const Polygon& polygon : coplanarPolygonArray)
				totalAreaBeforehand += polygon.Area(false);

		MergeCoplanarPolygons(coplanarPolygonArray);

		if (sanityCheck)
		{
			for (const Polygon& polygon : coplanarPolygonArray)
				totalAreaAfterword += polygon.Area(false);
			
			double tolerance = 1e-3;
			double error = totalAreaBeforehand - totalAreaAfterword;
			if (::fabs(error) >= tolerance)
			{
				THEBE_ASSERT(false);
			}
		}

		if (!mustBeConvex)
		{
			for (Polygon& polygon : coplanarPolygonArray)
				polygonArray.push_back(polygon);
		}
		else
		{
			for (Polygon& polygon : coplanarPolygonArray)
				polygon.TessellateUntilConvex(polygonArray);
		}
	}
}

/*static*/ void Polygon::MergeCoplanarPolygons(std::vector<Polygon>& coplanarPolygonArray)
{
	std::list<Polygon> polygonQueue;
	for (Polygon& polygon : coplanarPolygonArray)
		polygonQueue.push_back(polygon);

	coplanarPolygonArray.clear();

	while (polygonQueue.size() > 0)
	{
		std::list<Polygon>::iterator iter = polygonQueue.begin();
		Polygon polygonA = *iter;
		polygonQueue.erase(iter);

		bool merged = false;
		for (iter = polygonQueue.begin(); iter != polygonQueue.end(); iter++)
		{
			Polygon polygonB = *iter;

			Polygon mergedPolygon;
			if (mergedPolygon.MergeCoplanarPolygonPair(polygonA, polygonB))
			{
				polygonQueue.erase(iter);
				polygonQueue.push_back(mergedPolygon);
				merged = true;
				break;
			}
		}

		if (!merged)
			coplanarPolygonArray.push_back(polygonA);
	}
}

void Polygon::AddVerticesFrom(const Polygon& polygon, int i, int j, bool includeI /*= true*/, bool includeJ /*= true*/)
{
	i = polygon.Mod(i);
	j = polygon.Mod(j);

	int k = i;
	while (true)
	{
		if ((k != i || includeI) && (k != j || includeJ))
			this->vertexArray.push_back(polygon.vertexArray[k]);

		if (k == j)
			break;

		k = polygon.Mod(k + 1);
	}
}

void Polygon::Reduce(double tolerance /*= 1e-3*/)
{
	while (true)
	{
		if (this->FindAndRemoveRepeatedPoint(tolerance))
			continue;

		if (this->FindAndRemoveVertexOnEdge(tolerance))
			continue;

		if (this->FindAndRemoveSymmetricSpike(tolerance))
			continue;

		if (this->FindAndRemoveNonSymmetricSpike(tolerance))
			continue;

		break;
	}
}

bool Polygon::FindAndRemoveRepeatedPoint(double epsilon)
{
	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		int j = this->Mod(i + 1);
		const Vector3& pointA = this->vertexArray[i];
		const Vector3& pointB = this->vertexArray[j];
		if (pointA.IsPoint(pointB, epsilon))
		{
			// It doesn't matter which point we remove.  Choose point A arbitrarily.
			this->vertexArray.erase(this->vertexArray.begin() + i);
			return true;
		}
	}

	return false;
}

bool Polygon::FindAndRemoveVertexOnEdge(double epsilon)
{
	for (int j = 0; j < (signed)this->vertexArray.size(); j++)
	{
		int i = this->Mod(j - 1);
		int k = this->Mod(j + 1);

		LineSegment lineSegment;
		lineSegment.point[0] = this->vertexArray[i];
		lineSegment.point[1] = this->vertexArray[k];

		if (lineSegment.ContainsInteriorPoint(this->vertexArray[j], epsilon))
		{
			this->vertexArray.erase(this->vertexArray.begin() + j);
			return true;
		}
	}

	return false;
}

bool Polygon::FindAndRemoveSymmetricSpike(double epsilon)
{
	for (int j = 0; j < (signed)this->vertexArray.size(); j++)
	{
		int i = this->Mod(j - 1);
		int k = this->Mod(j + 1);

		const Vector3& pointA = this->vertexArray[i];
		const Vector3& pointB = this->vertexArray[k];

		if (pointA.IsPoint(pointB, epsilon))
		{
			this->vertexArray.erase(this->vertexArray.begin() + j);
			return true;
		}
	}

	return false;
}

bool Polygon::FindAndRemoveNonSymmetricSpike(double epsilon)
{
	for (int j = 0; j < (signed)this->vertexArray.size(); j++)
	{
		int i = this->Mod(j - 1);
		int k = this->Mod(j + 1);

		LineSegment lineSegA;
		lineSegA.point[0] = this->vertexArray[i];
		lineSegA.point[1] = this->vertexArray[j];

		if (lineSegA.ContainsInteriorPoint(this->vertexArray[k], epsilon))
		{
			this->vertexArray.erase(this->vertexArray.begin() + j);
			return true;
		}

		LineSegment lineSegB;
		lineSegB.point[0] = this->vertexArray[j];
		lineSegB.point[1] = this->vertexArray[k];

		if (lineSegB.ContainsInteriorPoint(this->vertexArray[i], epsilon))
		{
			this->vertexArray.erase(this->vertexArray.begin() + j);
			return true;
		}
	}

	return false;
}

bool Polygon::SelfOverlaps(double epsilon /*= 1e-6*/) const
{
	struct Intersection
	{
		Vector3 vectorAIn, vectorAOut;
		Vector3 vectorBIn, vectorBOut;
	};

	std::vector<Intersection> intersectionArray;

	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		const Vector3& pointA = this->vertexArray[i];

		LineSegment edgeA;
		edgeA.point[0] = this->vertexArray[i];
		edgeA.point[1] = this->vertexArray[this->Mod(i + 1)];

		for (int j = 0; j < (signed)this->vertexArray.size(); j++)
		{
			if (i == j)
				continue;

			if (this->Mod(i + 1) == j || this->Mod(i - 1) == j)
				continue;

			const Vector3& pointB = this->vertexArray[j];

			LineSegment edgeB;
			edgeB.point[0] = this->vertexArray[j];
			edgeB.point[1] = this->vertexArray[this->Mod(j + 1)];

			if (pointA.IsPoint(pointB, epsilon))
			{
				Vector3 center = (pointA + pointB) / 2.0;
				Intersection intersection;
				intersection.vectorAIn = (this->vertexArray[this->Mod(i - 1)] - center).Normalized();
				intersection.vectorAOut = (this->vertexArray[this->Mod(i + 1)] - center).Normalized();
				intersection.vectorBIn = (this->vertexArray[this->Mod(j - 1)] - center).Normalized();
				intersection.vectorBOut = (this->vertexArray[this->Mod(j + 1)] - center).Normalized();
				intersectionArray.push_back(intersection);
			}

			if (edgeA.ContainsInteriorPoint(edgeB.point[0], epsilon))
			{
				Intersection intersection;
				intersection.vectorAIn = (this->vertexArray[this->Mod(i + 0)] - this->vertexArray[j]).Normalized();
				intersection.vectorAOut = (this->vertexArray[this->Mod(i + 1)] - this->vertexArray[j]).Normalized();
				intersection.vectorBIn = (this->vertexArray[this->Mod(j + 1)] - this->vertexArray[j]).Normalized();
				intersection.vectorBOut = (this->vertexArray[this->Mod(j - 1)] - this->vertexArray[j]).Normalized();
				intersectionArray.push_back(intersection);
			}

			if (edgeB.ContainsInteriorPoint(edgeA.point[0], epsilon))
			{
				Intersection intersection;
				intersection.vectorAIn = (this->vertexArray[this->Mod(i - 1)] - this->vertexArray[i]).Normalized();
				intersection.vectorAOut = (this->vertexArray[this->Mod(i + 1)] - this->vertexArray[i]).Normalized();
				intersection.vectorBIn = (this->vertexArray[this->Mod(j + 0)] - this->vertexArray[i]).Normalized();
				intersection.vectorBOut = (this->vertexArray[this->Mod(j + 1)] - this->vertexArray[i]).Normalized();
				intersectionArray.push_back(intersection);
			}

			LineSegment connector;
			if (connector.SetAsShortestConnector(edgeA, edgeB) && connector.Length() < epsilon)
			{
				Vector3 center = connector.Lerp(0.5);
				if (edgeA.ContainsInteriorPoint(center, epsilon) && edgeB.ContainsInteriorPoint(center, epsilon))
				{
					Intersection intersection;
					intersection.vectorAIn = (this->vertexArray[this->Mod(i + 0)] - center).Normalized();
					intersection.vectorAOut = (this->vertexArray[this->Mod(i + 1)] - center).Normalized();
					intersection.vectorBIn = (this->vertexArray[this->Mod(j + 0)] - center).Normalized();
					intersection.vectorBOut = (this->vertexArray[this->Mod(j + 1)] - center).Normalized();
					intersectionArray.push_back(intersection);
				}
			}
		}
	}

	if(intersectionArray.size() > 0)
	{
		Plane plane = this->CalcPlane(false);

		for(const Intersection& intersection : intersectionArray)
		{
			double angleA = intersection.vectorAOut.AngleBetween(intersection.vectorAIn, plane.unitNormal);

			double angleB0 = intersection.vectorAOut.AngleBetween(intersection.vectorBIn, plane.unitNormal);
			if (0.0 < angleB0 && angleB0 < angleA)
				return true;

			double angleB1 = intersection.vectorAOut.AngleBetween(intersection.vectorBOut, plane.unitNormal);
			if (0.0 < angleB1 && angleB1 < angleA)
				return true;
		}
	}

	return false;
}

bool Polygon::MergeCoplanarPolygonPair(const Polygon& polygonA, const Polygon& polygonB)
{
	if (!this->MergeCoplanarPolygonPairInternal(polygonA, polygonB))
		return false;

	// Reduction must follow the merge, because the merger process
	// can leave behind a valid (in some sense) yet undesirable
	// polygon.  Reduction does not change the shape or area of the
	// polygon.  It just removes redundancies.
	this->Reduce();

	// Lastly, at this point, make sure the resulting polygon doesn't
	// touch itself in a way that is invalid.
	if (this->SelfOverlaps())
		return false;

	return true;
}

bool Polygon::MergeCoplanarPolygonPairInternal(const Polygon& polygonA, const Polygon& polygonB)
{
	constexpr double epsilon = 1e-7;

	this->vertexArray.clear();

	// We just merge here at the first touching edges we find, but there
	// could be multiple such edge-pairs, and so we're not necessarily
	// finding the best pair at which to merge the two polygons.
	for (int i = 0; i < (signed)polygonA.vertexArray.size(); i++)
	{
		LineSegment edgeSegA;
		edgeSegA.point[0] = polygonA.vertexArray[i];
		edgeSegA.point[1] = polygonA.vertexArray[polygonA.Mod(i + 1)];

		Vector3 unitEdgeADirection = edgeSegA.GetDelta().Normalized();

		for (int j = 0; j < (signed)polygonB.vertexArray.size(); j++)
		{
			LineSegment edgeSegB;
			edgeSegB.point[0] = polygonB.vertexArray[j];
			edgeSegB.point[1] = polygonB.vertexArray[polygonB.Mod(j + 1)];

			Vector3 unitEdgeBDirection = edgeSegB.GetDelta().Normalized();

			double angle = unitEdgeADirection.AngleBetween(unitEdgeBDirection);
			if (::fabs(angle - M_PI) < epsilon)
			{
				bool edgeSegA0InteriorToB = edgeSegB.ContainsInteriorPoint(edgeSegA.point[0]);
				bool edgeSegA1InteriorToB = edgeSegB.ContainsInteriorPoint(edgeSegA.point[1]);
				bool edgeSegB0InteriorToA = edgeSegA.ContainsInteriorPoint(edgeSegB.point[0]);
				bool edgeSegB1InteriorToA = edgeSegA.ContainsInteriorPoint(edgeSegB.point[1]);
				
				bool edgeSegA0IsEdgeSegB1 = edgeSegA.point[0].IsPoint(edgeSegB.point[1], epsilon);
				bool edgeSegB0IsEdgeSegA1 = edgeSegB.point[0].IsPoint(edgeSegA.point[1], epsilon);

				if ((edgeSegA0InteriorToB && edgeSegB0InteriorToA) ||
					(edgeSegA1InteriorToB && edgeSegB1InteriorToA) ||
					(edgeSegA0InteriorToB && edgeSegA1InteriorToB) ||
					(edgeSegB0InteriorToA && edgeSegB1InteriorToA))
				{
					this->AddVerticesFrom(polygonA, i + 1, i);
					this->AddVerticesFrom(polygonB, j + 1, j);

					return true;
				}
				else if (edgeSegA0IsEdgeSegB1 && (edgeSegA1InteriorToB || edgeSegB0InteriorToA))
				{
					this->AddVerticesFrom(polygonA, i + 1, i, true, false);
					this->AddVerticesFrom(polygonB, j + 1, j);

					return true;
				}
				else if (edgeSegB0IsEdgeSegA1 && (edgeSegA0InteriorToB || edgeSegB1InteriorToA))
				{
					this->AddVerticesFrom(polygonA, i + 1, i, false, true);
					this->AddVerticesFrom(polygonB, j + 1, j);

					return true;
				}
				else if (edgeSegA0IsEdgeSegB1 && edgeSegB0IsEdgeSegA1)
				{
					this->AddVerticesFrom(polygonA, i + 1, i, false, true);
					this->AddVerticesFrom(polygonB, j + 1, j, false, true);

					return true;
				}
			}
		}
	}

	return false;
}

bool Polygon::TessellateUntilConvex(std::vector<Polygon>& polygonArray) const
{
	if (this->vertexArray.size() < 3)
		return false;

	ConvexityInfo convexityInfo;
	if (this->IsConvex(&convexityInfo))
	{
		polygonArray.push_back(*this);
		return true;
	}

	Polygon polygonA, polygonB;

	Plane plane = this->CalcPlane();

	struct Pair
	{
		int i, j;
		double distance;
	};

	std::vector<Pair> vertexPairsArray;

	for (int i = 0; i < (signed)convexityInfo.concaveVertexArray.size(); i++)
	{
		for (int j = i + 1; j < (signed)convexityInfo.concaveVertexArray.size(); j++)
		{
			Pair pair;
			pair.i = convexityInfo.concaveVertexArray[i];
			pair.j = convexityInfo.concaveVertexArray[j];
			pair.distance = (this->vertexArray[pair.i] - this->vertexArray[pair.j]).Length();
			vertexPairsArray.push_back(pair);
		}
	}

	for (int i = 0; i < (signed)convexityInfo.concaveVertexArray.size(); i++)
	{
		for (int j = 0; j < (signed)convexityInfo.convexVertexArray.size(); j++)
		{
			Pair pair;
			pair.i = convexityInfo.concaveVertexArray[i];
			pair.j = convexityInfo.convexVertexArray[j];
			pair.distance = (this->vertexArray[pair.i] - this->vertexArray[pair.j]).Length();
			vertexPairsArray.push_back(pair);
		}
	}

	std::sort(vertexPairsArray.begin(), vertexPairsArray.end(), [](const Pair& pairA, const Pair& pairB) -> bool {
		return pairA.distance < pairB.distance;
	});

	bool splitFound = false;
	for (const Pair& pair : vertexPairsArray)
	{
		if (this->Split(pair.i, pair.j, polygonA, polygonB))
		{
			splitFound = true;
			break;
		}
	}

	if (!splitFound)
		return false;

	if (!polygonA.TessellateUntilConvex(polygonArray))
		return false;

	if (!polygonB.TessellateUntilConvex(polygonArray))
		return false;

	return true;
}

bool Polygon::TessellateUntilTriangular(std::vector<Polygon>& polygonArray) const
{
	if (this->vertexArray.size() < 3)
		return false;

	if (this->vertexArray.size() == 3)
	{
		polygonArray.push_back(*this);
		return true;
	}

	double bestAreaRatio = 0.0;
	int chosen_i = -1;
	int chosen_j = -1;

	for (int i = 0; i < (signed)this->vertexArray.size(); i++)
	{
		for (int j = i + 1; j < (signed)this->vertexArray.size(); j++)
		{
			Polygon polygonA, polygonB;

			if (this->Split(i, j, polygonA, polygonB, true))
			{
				double areaRatio = polygonA.Area() / polygonB.Area();
				if (::abs(areaRatio - 0.5) < ::abs(bestAreaRatio - 0.5))
				{
					chosen_i = i;
					chosen_j = j;
					bestAreaRatio = areaRatio;
				}
			}
		}
	}

	if (chosen_i == -1 || chosen_j == -1)
		return false;

	Polygon polygonA, polygonB;
	if (!this->Split(chosen_i, chosen_j, polygonA, polygonB, true))
		return false;

	if (!polygonA.TessellateUntilTriangular(polygonArray))
		return false;

	if (!polygonB.TessellateUntilTriangular(polygonArray))
		return false;

	return true;
}

bool Polygon::Split(int i, int j, Polygon& polygonA, Polygon& polygonB, bool assumeConvex /*= false*/) const
{
	// You can't split a triangle down any further without producing a degenerate polygon.
	if (this->vertexArray.size() <= 3)
		return false;

	i = this->Mod(i);
	j = this->Mod(j);

	// A split along an edge (or at a vertex) would result in one polygon being the original and the other being degenerate.
	if (i == j || this->Mod(i + 1) == j || this->Mod(i - 1) == j)
		return false;

	// At this point, if we can assume that the polygon is convex, then there is nothing left to vet about the purposed cut.
	if (!assumeConvex)
	{
		LineSegment cuttingSeg;
		cuttingSeg.point[0] = this->vertexArray[i];
		cuttingSeg.point[1] = this->vertexArray[j];

		Plane plane = this->CalcPlane();

		Vector3 cuttingVector = cuttingSeg.GetDelta().Normalized();
		Vector3 edgeVectorA, edgeVectorB;
		double angleA = 0.0, angleB = 0.0;

		// Does the cutting edge at i to j travel into the interior of the polygon?
		edgeVectorA = (this->vertexArray[this->Mod(i + 1)] - this->vertexArray[i]).Normalized();
		edgeVectorB = (this->vertexArray[this->Mod(i - 1)] - this->vertexArray[i]).Normalized();
		angleA = edgeVectorA.AngleBetween(cuttingVector, plane.unitNormal);
		angleB = edgeVectorA.AngleBetween(edgeVectorB, plane.unitNormal);
		if (!(0.0 < angleA && angleA < angleB))
			return false;	// No.  It won't work.

		// Does the cutting edge at j to i travel into the interior of the polygon?
		edgeVectorA = (this->vertexArray[this->Mod(j + 1)] - this->vertexArray[j]).Normalized();
		edgeVectorB = (this->vertexArray[this->Mod(j - 1)] - this->vertexArray[j]).Normalized();
		angleA = edgeVectorA.AngleBetween(-cuttingVector, plane.unitNormal);
		angleB = edgeVectorA.AngleBetween(edgeVectorB, plane.unitNormal);
		if (!(0.0 < angleA && angleA < angleB))
			return false;	// No.  It won't work.

		constexpr double epsilon = 1e-5;

		std::vector<LineSegment> edgeArray;
		this->GetEdges(edgeArray);

		// The last check is to make sure that the purposed cut is unobstructed in its path
		// from one vertex of the polygon, though the interior, to another vertex.
		bool lineSegUnobstructed = true;
		for (const LineSegment& edgeSeg : edgeArray)
		{
			if (edgeSeg.point[0].IsPoint(cuttingSeg.point[0]) ||
				edgeSeg.point[0].IsPoint(cuttingSeg.point[1]) ||
				edgeSeg.point[1].IsPoint(cuttingSeg.point[0]) ||
				edgeSeg.point[1].IsPoint(cuttingSeg.point[1]))
			{
				continue;
			}

			LineSegment connector;
			if (connector.SetAsShortestConnector(edgeSeg, cuttingSeg) && connector.Length() < epsilon)
			{
				lineSegUnobstructed = false;
				break;
			}
		}

		if (!lineSegUnobstructed)
			return false;
	}

	// Perform the cut.

	polygonA.vertexArray.clear();
	polygonB.vertexArray.clear();

	polygonA.AddVerticesFrom(*this, i, j);
	polygonB.AddVerticesFrom(*this, j, i);

	return true;
}

int Polygon::Mod(int i) const
{
	THEBE_ASSERT(this->vertexArray.size() > 0);
	if (i < 0)
	{
		int j = -i / int(this->vertexArray.size()) + 1;
		i += j * this->vertexArray.size();
	}
	else
	{
		i = i % int(this->vertexArray.size());
	}
	return i;
}

bool Polygon::HasVertex(const Vector3& givenVertex, double epsilon /*= 1e-5*/) const
{
	for (const Vector3& vertex : this->vertexArray)
		if (vertex.IsPoint(givenVertex, epsilon))
			return true;

	return false;
}

/*static*/ void Polygon::DumpArray(const std::vector<Polygon>& polygonArray, std::ostream& stream)
{
	uint32_t numPolygons = uint32_t(polygonArray.size());
	stream.write((char*)&numPolygons, sizeof(numPolygons));
	for (const Polygon& polygon : polygonArray)
		polygon.Dump(stream);
}

/*static*/ void Polygon::RestoreArray(std::vector<Polygon>& polygonArray, std::istream& stream)
{
	uint32_t numPolygons = 0;
	stream.read((char*)&numPolygons, sizeof(numPolygons));
	polygonArray.clear();
	for (uint32_t i = 0; i < numPolygons; i++)
	{
		Polygon polygon;
		polygon.Restore(stream);
		polygonArray.push_back(polygon);
	}
}

void Polygon::Dump(std::ostream& stream) const
{
	uint32_t numVertices = uint32_t(this->vertexArray.size());
	stream.write((char*)&numVertices, sizeof(numVertices));
	for (const Vector3& vertex : this->vertexArray)
		vertex.Dump(stream);
}

void Polygon::Restore(std::istream& stream)
{
	uint32_t numVertices = 0;
	stream.read((char*)&numVertices, sizeof(numVertices));
	this->vertexArray.clear();
	for (uint32_t i = 0; i < numVertices; i++)
	{
		Vector3 vertex;
		vertex.Restore(stream);
		this->vertexArray.push_back(vertex);
	}
}