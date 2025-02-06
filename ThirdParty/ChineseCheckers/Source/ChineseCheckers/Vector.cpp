#include "ChineseCheckers/Vector.h"
#include <math.h>

using namespace ChineseCheckers;

Vector::Vector()
{
	this->x = 0.0;
	this->y = 0.0;
}

Vector::Vector(double x, double y)
{
	this->x = x;
	this->y = y;
}

Vector::Vector(const Vector& vector)
{
	this->x = vector.x;
	this->y = vector.y;
}

void Vector::operator=(const Vector& vector)
{
	this->x = vector.x;
	this->y = vector.y;
}

void Vector::operator+=(const Vector& vector)
{
	this->x += vector.x;
	this->y += vector.y;
}

void Vector::operator-=(const Vector& vector)
{
	this->x -= vector.x;
	this->y -= vector.y;
}

void Vector::operator*=(double scalar)
{
	this->x *= scalar;
	this->y *= scalar;
}

void Vector::operator/=(double scalar)
{
	this->x /= scalar;
	this->y /= scalar;
}

double Vector::Length() const
{
	return ::sqrt(this->x * this->x + this->y * this->y);
}

double Vector::Dot(const Vector& vector) const
{
	return this->x * vector.x + this->y * vector.y;
}

double Vector::Cross(const Vector& vector) const
{
	return this->x * vector.y - this->y * vector.x;
}

Vector Vector::Normalized() const
{
	return *this / this->Length();
}

namespace ChineseCheckers
{
	Vector operator+(const Vector& vectorA, const Vector& vectorB)
	{
		return Vector(
			vectorA.x + vectorB.x,
			vectorA.y + vectorB.y
		);
	}

	Vector operator-(const Vector& vectorA, const Vector& vectorB)
	{
		return Vector(
			vectorA.x - vectorB.x,
			vectorA.y - vectorB.y
		);
	}

	Vector operator*(const Vector& vector, double scalar)
	{
		return Vector(
			vector.x * scalar,
			vector.y * scalar
		);
	}

	Vector operator*(double scalar, const Vector& vector)
	{
		return Vector(
			vector.x * scalar,
			vector.y * scalar
		);
	}

	Vector operator/(const Vector& vector, double scalar)
	{
		return Vector(
			vector.x / scalar,
			vector.y / scalar
		);
	}
}