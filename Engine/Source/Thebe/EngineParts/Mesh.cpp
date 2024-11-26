#include "Thebe/EngineParts/Mesh.h"

using namespace Thebe;

Mesh::Mesh()
{
}

/*virtual*/ Mesh::~Mesh()
{
}

/*virtual*/ bool Mesh::Setup()
{
	return true;
}

/*virtual*/ void Mesh::Shutdown()
{
}

bool Mesh::Instantiate(Reference<MeshInstance>& meshInstance)
{
	return false;
}