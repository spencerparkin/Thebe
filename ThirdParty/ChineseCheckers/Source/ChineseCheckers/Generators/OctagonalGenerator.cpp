#include "ChineseCheckers/Generators/OctagonalGenerator.h"

using namespace ChineseCheckers;

OctagonalGenerator::OctagonalGenerator(Factory* factory) : GraphGenerator(factory)
{
}

/*virtual*/ OctagonalGenerator::~OctagonalGenerator()
{
}

/*virtual*/ Graph* OctagonalGenerator::Generate(const std::set<Marble::Color>& participantSet)
{
	return nullptr;
}