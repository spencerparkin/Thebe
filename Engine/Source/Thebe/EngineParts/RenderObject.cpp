#include "Thebe/EngineParts/RenderObject.h"

using namespace Thebe;

RenderObject::RenderObject()
{
	this->flags = THEBE_RENDER_OBJECT_FLAG_VISIBLE | THEBE_RENDER_OBJECT_FLAG_CASTS_SHADOW;
	this->renderOrder.primary = 0;
	this->renderOrder.secondary = 0.0;
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

bool RenderObject::IsVisible() const
{
	return (this->flags & THEBE_RENDER_OBJECT_FLAG_VISIBLE) != 0;
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

/*virtual*/ void RenderObject::PrepareRenderOrder(RenderContext* context) const
{
	this->renderOrder.primary = THEBE_RENDER_ORDER_OPAQUE;
	this->renderOrder.secondary = 0.0;
}

const RenderObject::RenderOrder& RenderObject::GetRenderOrder() const
{
	return this->renderOrder;
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