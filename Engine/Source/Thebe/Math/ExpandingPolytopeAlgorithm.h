#pragma once

#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Plane.h"

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

		/**
		 * Note that you must initialize this triangle list and vertex array before you call this.
		 * Typically this is done so that the initial polytope is a tetrahedron.
		 */
		void Expand(PointSupplier* pointSupplier);

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
			Triangle(int i, int j, int k);
			Triangle(const Triangle& triangle);
			virtual ~Triangle();

			void operator=(const Triangle& triangle);

			Plane MakePlane(const std::vector<Vector3>& vertexArray) const;
			bool Cancels(const Triangle& triangle) const;
			bool SameAs(const Triangle& triangle) const;
			void Reverse();
			Triangle Reversed() const;

		public:
			int vertex[3];
		};

		std::list<Triangle> triangleList;
		std::vector<Vector3> vertexArray;
	};
}