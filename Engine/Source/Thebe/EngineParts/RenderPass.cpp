#include "Thebe/EngineParts/RenderPass.h"
#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

RenderPass::RenderPass()
{
}

/*virtual*/ RenderPass::~RenderPass()
{
}

/*virtual*/ bool RenderPass::Setup(void* data)
{
	if (this->commandQueue.Get())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT result = graphicsEngine->GetDevice()->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&this->commandQueue));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create the command queue.  Error: 0x%08x", result);
		return false;
	}

	this->fence = new Fence();
	this->fence->SetGraphicsEngine(graphicsEngine.Get());
	if (!this->fence->Setup(nullptr))
	{
		THEBE_LOG("Failed to create fence for render pass.");
		return false;
	}

	return true;
}

/*virtual*/ void RenderPass::Shutdown()
{
	if (this->fence.Get())
	{
		this->fence->Shutdown();
		this->fence = nullptr;
	}

	if (this->input)
	{
		this->input->Shutdown();
		this->input = nullptr;
	}

	if (this->output)
	{
		this->output->Shutdown();
		this->output = nullptr;
	}

	if (this->camera)
	{
		this->camera->Shutdown();
		this->camera = nullptr;
	}

	this->commandQueue = nullptr;
}

/*virtual*/ void RenderPass::Perform()
{
	// call pre-render on the output.  this may stall us until GPU is ready.

	// get the command-allocator from the output.
	// reset the command-allocator and point the command-list to it.
	// call the input to render with our command-list.

	// call post-render on the output.
}

void RenderPass::WaitForCommandQueueComplete()
{
	if (this->commandQueue.Get() && this->fence.Get())
		this->fence->EnqueueSignalAndWaitForIt(this->commandQueue.Get());
}

ID3D12CommandQueue* RenderPass::GetCommandQueue()
{
	return this->commandQueue.Get();
}

RenderObject* RenderPass::GetInput()
{
	return this->input.Get();
}

RenderTarget* RenderPass::GetOutput()
{
	return this->output.Get();
}