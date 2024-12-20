#include "Builder.h"

Builder::Builder()
{
}

/*virtual*/ Builder::~Builder()
{
}

std::string Builder::NoSpaces(const std::string& givenString)
{
	std::string resultString = givenString;

	while (true)
	{
		size_t pos = resultString.find(' ');
		if (pos == std::string::npos)
			break;

		resultString.replace(pos, 1, "_");
	}

	return resultString;
}