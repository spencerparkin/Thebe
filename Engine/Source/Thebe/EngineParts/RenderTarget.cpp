#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
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
	if (!CommandQueue::Setup())
		return false;

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

	CommandQueue::Shutdown();
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

/*virtual*/ bool RenderTarget::GetRenderContext(RenderObject::RenderContext& context)
{
	return false;
}

/*virtual*/ RenderObject* RenderTarget::GetRenderObject()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return nullptr;

	return graphicsEngine->GetRenderObject();
}

/*virtual*/ bool RenderTarget::Render()
{
	// Note that this call should stall if necessary until the returned command allocator can safely be reset.
	// A command allocator cannot be reset while the GPU is still executing commands that were allocated with it.
	ID3D12CommandAllocator* commandAllocator = this->AcquireCommandAllocator(this->commandQueue.Get());
	if (!commandAllocator)
	{
		THEBE_LOG("No command allocator given by the render pass output.");
		return false;
	}

	HRESULT result = commandAllocator->Reset();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to reset command allocator.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!this->commandList.Get())
	{
		result = graphicsEngine->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&this->commandList));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create command list.  Error: 0x%08x", result);
			return false;
		}

		//this->commandList->SetName(L"");
	}
	else
	{
		result = this->commandList->Reset(commandAllocator, nullptr);
		if (FAILED(result))
		{
			THEBE_LOG("Failed to reset command list and put it into the recording state.");
			return false;
		}
	}

	if (!this->PreRender(this->commandList.Get()))
	{
		THEBE_LOG("Pre-render failed.");
		return false;
	}

	RenderObject* renderObject = this->GetRenderObject();
	RenderObject::RenderContext context{};
	if (renderObject && this->GetRenderContext(context))
	{
		ID3D12DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap()->GetDescriptorHeap();
		commandList->SetDescriptorHeaps(1, &csuDescriptorHeap);

		if (!renderObject->Render(this->commandList.Get(), &context))
		{
			THEBE_LOG("Render failed.");
			return false;
		}
	}

	if (!this->PostRender(this->commandList.Get()))
	{
		THEBE_LOG("Post-render failed.");
		return false;
	}

	result = this->commandList->Close();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to close command list.");
		return false;
	}

	// Kick-off the GPU to execute the commands.
	ID3D12CommandList* commandListArray[] = { this->commandList.Get() };
	this->commandQueue->ExecuteCommandLists(_countof(commandListArray), commandListArray);

	// Indicate that we no longer need the command allocator.
	this->ReleaseCommandAllocator(commandAllocator, this->commandQueue.Get());

	return true;
}