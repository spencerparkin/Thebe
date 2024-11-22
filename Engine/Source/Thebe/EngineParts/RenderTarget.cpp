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

/*virtual*/ ID3D12CommandAllocator* RenderTarget::AcquireCommandAllocator(ID3D12CommandQueue* commandQueue)
{
	return nullptr;
}

/*virtual*/ void RenderTarget::ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue)
{
}

/*virtual*/ bool RenderTarget::PreRender(ID3D12GraphicsCommandList* commandList)
{
	return true;
}

/*virtual*/ bool RenderTarget::PostRender(ID3D12GraphicsCommandList* commandList)
{
	return true;
}