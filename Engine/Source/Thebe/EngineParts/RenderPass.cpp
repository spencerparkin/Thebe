#include "Thebe/EngineParts/RenderPass.h"
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

	return true;
}

/*virtual*/ void RenderPass::Shutdown()
{
}

/*virtual*/ void RenderPass::Perform()
{
	// call pre-render on the output.  this may stall us until GPU is ready.

	// get the command-allocator from the output.
	// reset the command-allocator and point the command-list to it.
	// call the input to render with our command-list.

	// call post-render on the output.
}