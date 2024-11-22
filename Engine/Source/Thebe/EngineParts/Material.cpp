#include "Thebe/EngineParts/Material.h"

using namespace Thebe;

Material::Material()
{
}

/*virtual*/ Material::~Material()
{
}

/*virtual*/ bool Material::Setup(void* data)
{
	return true;
}

/*virtual*/ void Material::Shutdown()
{
}