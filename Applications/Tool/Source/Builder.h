#pragma once

#include <string>

class Builder
{
public:
	Builder();
	virtual ~Builder();

	std::string NoSpaces(const std::string& givenString);
};