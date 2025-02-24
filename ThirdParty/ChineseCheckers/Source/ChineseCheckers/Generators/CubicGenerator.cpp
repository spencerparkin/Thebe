#include "ChineseCheckers/Generators/CubicGenerator.h"

using namespace ChineseCheckers;

CubicGenerator::CubicGenerator(Factory* factory) : GraphGenerator(factory)
{
}

/*virtual*/ CubicGenerator::~CubicGenerator()
{
}

/*virtual*/ Graph* CubicGenerator::Generate(const std::set<Marble::Color>& participantSet)
{
	return nullptr;
}