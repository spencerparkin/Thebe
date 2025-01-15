#pragma once

#include "Thebe/Math/Vector2.h"

namespace Thebe
{
	/**
	 * These are axis-aligned rectangles in the XY-plane.
	 */
	class THEBE_API Rectangle
	{
	public:
		Rectangle();
		Rectangle(const Rectangle& rectangle);
		Rectangle(const Vector2& minCorner, const Vector2& maxCorner);

		void operator=(const Rectangle& rectangle);

		bool IsValid() const;
		double GetWidth() const;
		double GetHeight() const;
		double GetAspectRatio() const;
		bool ContainsPoint(const Vector2& point) const;
		bool Intersect(const Rectangle& rectangleA, const Rectangle& rectangleB);
		void ExpandToIncludePoint(const Vector2& point);
		void ExpandToMatchAspectRatio(double aspectRatio);
		void ContractToMatchAspectRatio(double aspectRatio);
		Vector2 PointToUVs(const Vector2& point) const;
		Vector2 PointFromUVs(const Vector2& uv) const;

	public:
		Vector2 minCorner, maxCorner;
	};
}