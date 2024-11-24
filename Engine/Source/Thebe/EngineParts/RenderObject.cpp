#include "Thebe/EngineParts/RenderObject.h"

using namespace Thebe;

RenderObject::RenderObject()
{
}

/*virtual*/ RenderObject::~RenderObject()
{
}

/*virtual*/ bool RenderObject::Setup()
{
	return true;
}

/*virtual*/ void RenderObject::Shutdown()
{
}

/*virtual*/ bool RenderObject::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	return true;
}