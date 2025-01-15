#include "Thebe/Math/Rectangle.h"

using namespace Thebe;

Rectangle::Rectangle()
{
}

Rectangle::Rectangle(const Rectangle& rectangle)
{
	this->minCorner = rectangle.minCorner;
	this->maxCorner = rectangle.maxCorner;
}

Rectangle::Rectangle(const Vector2& minCorner, const Vector2& maxCorner)
{
	this->minCorner = minCorner;
	this->maxCorner = maxCorner;
}

bool Rectangle::IsValid() const
{
	if (this->minCorner.x > this->maxCorner.x)
		return false;

	if (this->minCorner.y > this->maxCorner.y)
		return false;

	return true;
}

void Rectangle::operator=(const Rectangle& rectangle)
{
	this->minCorner = rectangle.minCorner;
	this->maxCorner = rectangle.maxCorner;
}

double Rectangle::GetWidth() const
{
	return this->maxCorner.x - this->minCorner.x;
}

double Rectangle::GetHeight() const
{
	return this->maxCorner.y - this->minCorner.y;
}

double Rectangle::GetAspectRatio() const
{
	return this->GetWidth() / this->GetHeight();
}

bool Rectangle::ContainsPoint(const Vector2& point) const
{
	return
		this->minCorner.x <= point.x && point.x <= this->maxCorner.x &&
		this->minCorner.y <= point.y && point.y <= this->maxCorner.y;
}

bool Rectangle::Intersect(const Rectangle& rectangleA, const Rectangle& rectangleB)
{
	// TODO: Write this.
	return false;
}

void Rectangle::ExpandToIncludePoint(const Vector2& point)
{
	this->minCorner.x = THEBE_MIN(this->minCorner.x, point.x);
	this->minCorner.y = THEBE_MIN(this->minCorner.y, point.y);
	this->maxCorner.x = THEBE_MAX(this->maxCorner.x, point.x);
	this->maxCorner.y = THEBE_MAX(this->maxCorner.y, point.y);
}

void Rectangle::ExpandToMatchAspectRatio(double aspectRatio)
{
	// TODO: Write this.
}

void Rectangle::ContractToMatchAspectRatio(double aspectRatio)
{
	// TODO: Write this.
}

Vector2 Rectangle::PointToUVs(const Vector2& point) const
{
	Vector2 uv;
	uv.x = (point.x - this->minCorner.x) / this->GetWidth();
	uv.y = (point.y - this->minCorner.y) / this->GetHeight();
	return uv;
}

Vector2 Rectangle::PointFromUVs(const Vector2& uv) const
{
	Vector2 point;
	point.x = this->minCorner.x + uv.x * this->GetWidth();
	point.y = this->minCorner.y + uv.y + this->GetHeight();
	return point;
}