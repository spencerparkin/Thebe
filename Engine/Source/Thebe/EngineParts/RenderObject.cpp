#include "Thebe/EngineParts/RenderObject.h"

using namespace Thebe;

RenderObject::RenderObject()
{
	this->flags = THEBE_RENDER_OBJECT_FLAG_VISIBLE | THEBE_RENDER_OBJECT_FLAG_CASTS_SHADOW;
}

/*virtual*/ RenderObject::~RenderObject()
{
}

uint32_t RenderObject::GetFlags() const
{
	return this->flags;
}

void RenderObject::SetFlags(uint32_t flags)
{
	this->flags = flags;
}

/*virtual*/ bool RenderObject::RendersToTarget(RenderTarget* renderTarget) const
{
	return false;
}

/*virtual*/ bool RenderObject::Setup()
{
	return true;
}

/*virtual*/ void RenderObject::Shutdown()
{
}

/*virtual*/ uint32_t RenderObject::GetRenderOrder() const
{
	return THEBE_RENDER_ORDER_OPAQUE;
}

/*virtual*/ bool RenderObject::Render(ID3D12GraphicsCommandList* commandList, RenderContext* context)
{
	return true;
}

/*virtual*/ void RenderObject::PrepareForRender()
{
}

/*virtual*/ void RenderObject::AppendAllChildRenderObjects(std::list<RenderObject*>& renderObjectList)
{
}