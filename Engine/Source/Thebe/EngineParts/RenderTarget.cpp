#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

RenderTarget::RenderTarget()
{
}

/*virtual*/ RenderTarget::~RenderTarget()
{
}

/*virtual*/ bool RenderTarget::Setup()
{
	if (this->frameArray.size() > 0)
	{
		THEBE_LOG("Render target already setup.");
		return false;
	}

	this->frameArray.resize(THEBE_NUM_SWAP_FRAMES);

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();

	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		Frame* frame = this->NewFrame();
		this->frameArray[i] = frame;

		frame->fence = new Fence();
		frame->fence->SetGraphicsEngine(graphicsEngine.Get());
		if (!frame->fence->Setup())
			return false;

		HRESULT result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame->commandAllocator));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create command allocator for swap-frame %d.  Error: 0x%08x", i, result);
			return false;
		}
	}

	return true;
}

/*virtual*/ void RenderTarget::Shutdown()
{
	for (Frame* frame : this->frameArray)
	{
		if (frame->fence.Get())
		{
			frame->fence->Shutdown();
			frame->fence = nullptr;
		}

		delete frame;
	}

	this->frameArray.clear();
}

/*virtual*/ ID3D12CommandAllocator* RenderTarget::AcquireCommandAllocator(ID3D12CommandQueue* commandQueue)
{
	UINT i = this->GetCurrentFrame();
	THEBE_ASSERT(0 <= i && i < (UINT)this->frameArray.size());
	Frame* frame = this->frameArray[i];

	// Make sure the GPU is done with the command allocator before we return it to the caller.
	frame->fence->WaitForSignalIfNecessary();

	return frame->commandAllocator.Get();
}

/*virtual*/ void RenderTarget::ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue)
{
	UINT i = this->GetCurrentFrame();
	THEBE_ASSERT(0 <= i && i < (UINT)this->frameArray.size());
	Frame* frame = this->frameArray[i];
	frame->fence->EnqueueSignal(commandQueue);
}

/*virtual*/ bool RenderTarget::PreRender(ID3D12GraphicsCommandList* commandList)
{
	return true;
}

/*virtual*/ bool RenderTarget::PostRender(ID3D12GraphicsCommandList* commandList)
{
	return true;
}