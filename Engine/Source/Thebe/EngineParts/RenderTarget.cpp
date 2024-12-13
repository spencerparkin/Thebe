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

/*virtual*/ bool RenderTarget::PreRender(RenderObject::RenderContext& context)
{
	return false;
}

/*virtual*/ bool RenderTarget::PostRender()
{
	return false;
}

/*virtual*/ bool RenderTarget::Render()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	THEBE_ASSERT(0 <= frameIndex && frameIndex < (UINT)this->frameArray.size());
	Frame* frame = this->frameArray[frameIndex];

	// Make sure the GPU is done with the command allocator before we reset it.
	frame->fence->WaitForSignalIfNecessary();
	ID3D12CommandAllocator* commandAllocator = frame->commandAllocator.Get();
	HRESULT result = commandAllocator->Reset();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to reset command allocator.");
		return false;
	}

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

	RenderObject::RenderContext context{};
	context.renderTarget = this;
	if (!this->PreRender(context))
	{
		THEBE_LOG("Pre-render failed.");
		return false;
	}
	
	RenderObject* renderObject = graphicsEngine->GetRenderObject();
	if (renderObject)
	{
		ID3D12DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap()->GetDescriptorHeap();
		this->commandList->SetDescriptorHeaps(1, &csuDescriptorHeap);

		if (!renderObject->Render(this->commandList.Get(), &context))
		{
			THEBE_LOG("Render failed.");
			return false;
		}
	}

	if (!this->PostRender())
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

	// Let a derived class enqueue anything they wish before we enqueue a
	// fence signal to mark the true end of our present use of the queue.
	this->PreSignal();

	frame->fence->EnqueueSignal(this->commandQueue.Get());

	return true;
}

/*virtual*/ void RenderTarget::PreSignal()
{
}