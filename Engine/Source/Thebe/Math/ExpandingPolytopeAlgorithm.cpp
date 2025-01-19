#include "Thebe/Math/ExpandingPolytopeAlgorithm.h"

using namespace Thebe;

//--------------------------- ExpandingPolytopeAlgorithm ---------------------------

ExpandingPolytopeAlgorithm::ExpandingPolytopeAlgorithm()
{
}

/*virtual*/ ExpandingPolytopeAlgorithm::~ExpandingPolytopeAlgorithm()
{
}

void ExpandingPolytopeAlgorithm::Expand(PointSupplier* pointSupplier)
{
	const double planeThickness = 1e-6;

	while (true)
	{
		Vector3 point;
		if (!pointSupplier->GetNextPoint(point))
			break;

		std::list<Triangle> newTriangleList;
		for (const Triangle& triangle : triangleList)
		{
			Plane plane = triangle.MakePlane(this->vertexArray);
			if (plane.GetSide(point, planeThickness) == Plane::Side::FRONT)
			{
				int i = (signed)this->vertexArray.size();

				Triangle triangleA = triangle.Reversed();
				Triangle triangleB(i, triangle.vertex[0], triangle.vertex[1]);
				Triangle triangleC(i, triangle.vertex[1], triangle.vertex[2]);
				Triangle triangleD(i, triangle.vertex[2], triangle.vertex[0]);

				newTriangleList.push_back(triangleA);
				newTriangleList.push_back(triangleB);
				newTriangleList.push_back(triangleC);
				newTriangleList.push_back(triangleD);
			}
		}

		if (newTriangleList.size() == 0)
			continue;

		this->vertexArray.push_back(point);

		for (const Triangle& newTriangle : newTriangleList)
		{
			bool addTriangle = true;
			for (std::list<Triangle>::iterator iter = triangleList.begin(); iter != triangleList.end(); iter++)
			{
				const Triangle& triangle = *iter;
				if (newTriangle.Cancels(triangle))
				{
					triangleList.erase(iter);
					addTriangle = false;
					break;
				}
			}

			if (addTriangle)
				triangleList.push_back(newTriangle);
		}
	}
}

//--------------------------- ExpandingPolytopeAlgorithm::Triangle ---------------------------

/*virtual*/ bool ExpandingPolytopeAlgorithm::PointListSupplier::GetNextPoint(Vector3& point)
{
	if (this->pointList.size() == 0)
		return false;

	point = *this->pointList.begin();
	this->pointList.pop_front();
	return true;
}

//--------------------------- ExpandingPolytopeAlgorithm::Triangle ---------------------------

ExpandingPolytopeAlgorithm::Triangle::Triangle()
{
	for (int i = 0; i < 3; i++)
		this->vertex[i] = 0;
}

ExpandingPolytopeAlgorithm::Triangle::Triangle(int i, int j, int k)
{
	this->vertex[0] = i;
	this->vertex[1] = j;
	this->vertex[2] = k;
}

ExpandingPolytopeAlgorithm::Triangle::Triangle(const Triangle& triangle)
{
	*this = triangle;
}

/*virtual*/ ExpandingPolytopeAlgorithm::Triangle::~Triangle()
{
}

void ExpandingPolytopeAlgorithm::Triangle::operator=(const Triangle& triangle)
{
	for (int i = 0; i < 3; i++)
		this->vertex[i] = triangle.vertex[i];
}

Plane ExpandingPolytopeAlgorithm::Triangle::MakePlane(const std::vector<Vector3>& vertexArray) const
{
	const Vector3& vertexA = vertexArray[this->vertex[0]];
	const Vector3& vertexB = vertexArray[this->vertex[1]];
	const Vector3& vertexC = vertexArray[this->vertex[2]];

	Vector3 normal = (vertexB - vertexA).Cross(vertexC - vertexA).Normalized();
	return Plane(vertexA, normal);
}

bool ExpandingPolytopeAlgorithm::Triangle::Cancels(const Triangle& triangle) const
{
	Triangle reverse = triangle;
	reverse.Reverse();
	return reverse.SameAs(*this);
}

bool ExpandingPolytopeAlgorithm::Triangle::SameAs(const Triangle& triangle) const
{
	for (int i = 0; i < 3; i++)
	{
		if (this->vertex[i] == triangle.vertex[0])
		{
			for (int j = 0; j < 3; j++)
				if (this->vertex[(i + j) % 3] != triangle.vertex[j])
					return false;

			return true;
		}
	}

	return false;
}

void ExpandingPolytopeAlgorithm::Triangle::Reverse()
{
	int i = this->vertex[0];
	this->vertex[0] = this->vertex[2];
	this->vertex[2] = i;
}

ExpandingPolytopeAlgorithm::Triangle ExpandingPolytopeAlgorithm::Triangle::Reversed() const
{
	Triangle triangle(*this);
	triangle.Reverse();
	return triangle;
}