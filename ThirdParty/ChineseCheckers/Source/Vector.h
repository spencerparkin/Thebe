#pragma once

namespace ChineseCheckers
{
	/**
	 * These are vectors in the XY plane.  We cannot represent or reason about the
	 * game of Chinese Checkers without some kind of spacial awareness.
	 */
	class Vector
	{
	public:
		Vector();
		Vector(double x, double y);
		Vector(const Vector& vector);

		void operator=(const Vector& vector);
		void operator+=(const Vector& vector);
		void operator-=(const Vector& vector);
		void operator*=(double scalar);
		void operator/=(double scalar);

		double Length() const;
		double Dot(const Vector& vector) const;
		double Cross(const Vector& vector) const;
		Vector Normalized() const;

	public:
		double x, y;
	};

	Vector operator+(const Vector& vectorA, const Vector& vectorB);
	Vector operator-(const Vector& vectorA, const Vector& vectorB);
	Vector operator*(const Vector& vector, double scalar);
	Vector operator*(double scalar, const Vector& vector);
	Vector operator/(const Vector& vector, double scalar);
}