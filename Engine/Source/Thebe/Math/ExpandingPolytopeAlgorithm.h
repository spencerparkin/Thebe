#pragma once

#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Plane.h"
#include "Thebe/Utilities/StackHeap.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API ExpandingPolytopeAlgorithm
	{
	public:
		ExpandingPolytopeAlgorithm();
		virtual ~ExpandingPolytopeAlgorithm();

		class Triangle;
		class PointSupplier;
		class TriangleFactory;

		/**
		 * Note that you must initialize the triangle list and vertex array before you call this.
		 * Typically this is done so that the initial polytope is a tetrahedron.
		 */
		void Expand(PointSupplier* pointSupplier, TriangleFactory* triangleFactory);

		/**
		 * 
		 */
		class THEBE_API PointSupplier
		{
		public:
			virtual bool GetNextPoint(Vector3& point) = 0;
		};

		/**
		 * 
		 */
		class THEBE_API PointListSupplier : public PointSupplier
		{
		public:
			virtual bool GetNextPoint(Vector3& point) override;

			std::list<Vector3> pointList;
		};

		/**
		 *
		 */
		class THEBE_API Triangle
		{
		public:
			Triangle();
			virtual ~Triangle();

			Plane MakePlane(const std::vector<Vector3>& vertexArray) const;
			bool Cancels(const Triangle* triangle, TriangleFactory* triangleFactory) const;
			bool SameAs(const Triangle* triangle) const;
			Triangle* Reversed(TriangleFactory* triangleFactory) const;

		public:
			int vertex[3];
		};

		/**
		 * 
		 */
		class THEBE_API TriangleFactory
		{
		public:
			virtual Triangle* AllocTriangle(int i, int j, int k) = 0;
			virtual void FreeTriangle(Triangle* triangle) = 0;
		};

		/**
		 * 
		 */
		template<typename T>
		class THEBE_API TypedTriangleFactory : public TriangleFactory
		{
		public:
			TypedTriangleFactory(uint64_t maxTriangles)
			{
				uint64_t memoryBufferSize = maxTriangles * sizeof(T);
				this->memoryBuffer = new uint8_t[memoryBufferSize];
				this->objectHeap.Configure(memoryBuffer, memoryBufferSize);
			}

			virtual ~TypedTriangleFactory()
			{
				delete[] this->memoryBuffer;
			}

			virtual Triangle* AllocTriangle(int i, int j, int k) override
			{
				Triangle* triangle = this->objectHeap.Allocate();
				triangle->vertex[0] = i;
				triangle->vertex[1] = j;
				triangle->vertex[2] = k;
				return triangle;
			}

			virtual void FreeTriangle(Triangle* triangle) override
			{
				this->objectHeap.Deallocate((T*)triangle);
			}

		private:
			ObjectHeap<T> objectHeap;
			uint8_t* memoryBuffer;
		};

	public:
		std::list<Triangle*> triangleList;
		std::vector<Vector3> vertexArray;
	};
}