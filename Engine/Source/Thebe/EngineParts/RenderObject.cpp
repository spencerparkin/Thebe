#include "Thebe/EngineParts/RenderObject.h"

using namespace Thebe;

RenderObject::RenderObject()
{
}

/*virtual*/ RenderObject::~RenderObject()
{
}

/*virtual*/ bool RenderObject::Setup(void* data)
{
	return true;
}

/*virtual*/ void RenderObject::Shutdown()
{
}