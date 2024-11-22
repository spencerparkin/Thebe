#include "Thebe/EngineParts/RenderTarget.h"

using namespace Thebe;

RenderTarget::RenderTarget()
{
}

/*virtual*/ RenderTarget::~RenderTarget()
{
}

/*virtual*/ bool RenderTarget::Setup(void* data)
{
	return true;
}

/*virtual*/ void RenderTarget::Shutdown()
{
}