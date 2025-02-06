#include "ChineseCheckers/GraphGenerator.h"

using namespace ChineseCheckers;

GraphGenerator::GraphGenerator(Factory* factory)
{
	this->factory = factory;
	this->scale = 1.0;
}

/*virtual*/ GraphGenerator::~GraphGenerator()
{
}

void GraphGenerator::SetScale(double scale)
{
	this->scale = scale;
}