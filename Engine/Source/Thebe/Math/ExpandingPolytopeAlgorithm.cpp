#include "Thebe/Math/ExpandingPolytopeAlgorithm.h"

using namespace Thebe;

//--------------------------- ExpandingPolytopeAlgorithm ---------------------------

ExpandingPolytopeAlgorithm::ExpandingPolytopeAlgorithm()
{
}

/*virtual*/ ExpandingPolytopeAlgorithm::~ExpandingPolytopeAlgorithm()
{
}

void ExpandingPolytopeAlgorithm::Expand(PointSupplier* pointSupplier, TriangleFactory* triangleFactory)
{
	const double planeThickness = 1e-6;

	while (true)
	{
		Vector3 point;
		if (!pointSupplier->GetNextPoint(point))
			break;

		std::list<Triangle*> newTriangleList;
		for (const Triangle* triangle : this->triangleList)
		{
			Plane plane = triangle->MakePlane(this->vertexArray);
			if (plane.GetSide(point, planeThickness) == Plane::Side::FRONT)
			{
				int i = (signed)this->vertexArray.size();

				Triangle* triangleA = triangle->Reversed(triangleFactory);
				Triangle* triangleB = triangleFactory->AllocTriangle(i, triangle->vertex[0], triangle->vertex[1]);
				Triangle* triangleC = triangleFactory->AllocTriangle(i, triangle->vertex[1], triangle->vertex[2]);
				Triangle* triangleD = triangleFactory->AllocTriangle(i, triangle->vertex[2], triangle->vertex[0]);

				newTriangleList.push_back(triangleA);
				newTriangleList.push_back(triangleB);
				newTriangleList.push_back(triangleC);
				newTriangleList.push_back(triangleD);
			}
		}

		if (newTriangleList.size() == 0)
			continue;

		this->vertexArray.push_back(point);

		for (Triangle* newTriangle : newTriangleList)
		{
			bool addTriangle = true;
			for (std::list<Triangle*>::iterator iter = triangleList.begin(); iter != triangleList.end(); iter++)
			{
				Triangle* triangle = *iter;
				if (newTriangle->Cancels(triangle, triangleFactory))
				{
					triangleFactory->FreeTriangle(triangle);
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
	this->vertex[0] = 0;
	this->vertex[1] = 0;
	this->vertex[2] = 0;
}

/*virtual*/ ExpandingPolytopeAlgorithm::Triangle::~Triangle()
{
}

Plane ExpandingPolytopeAlgorithm::Triangle::MakePlane(const std::vector<Vector3>& vertexArray) const
{
	const Vector3& vertexA = vertexArray[this->vertex[0]];
	const Vector3& vertexB = vertexArray[this->vertex[1]];
	const Vector3& vertexC = vertexArray[this->vertex[2]];

	return Plane(vertexA, vertexB, vertexC);
}

bool ExpandingPolytopeAlgorithm::Triangle::Cancels(const Triangle* triangle, TriangleFactory* triangleFactory) const
{
	Triangle* reverse = triangle->Reversed(triangleFactory);
	bool cancels = reverse->SameAs(this);
	triangleFactory->FreeTriangle(reverse);
	return cancels;
}

bool ExpandingPolytopeAlgorithm::Triangle::SameAs(const Triangle* triangle) const
{
	for (int i = 0; i < 3; i++)
	{
		if (this->vertex[i] == triangle->vertex[0])
		{
			for (int j = 0; j < 3; j++)
				if (this->vertex[(i + j) % 3] != triangle->vertex[j])
					return false;

			return true;
		}
	}

	return false;
}

ExpandingPolytopeAlgorithm::Triangle* ExpandingPolytopeAlgorithm::Triangle::Reversed(TriangleFactory* triangleFactory) const
{
	return triangleFactory->AllocTriangle(this->vertex[2], this->vertex[1], this->vertex[0]);
}