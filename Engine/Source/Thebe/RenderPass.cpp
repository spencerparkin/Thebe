#include "RenderPass.h"
#include "GraphicsEngine.h"
#include "Log.h"

using namespace Thebe;

RenderPass::RenderPass()
{
}

/*virtual*/ RenderPass::~RenderPass()
{
}

/*virtual*/ bool RenderPass::Setup(GraphicsEngine* graphicsEngine)
{
	if (this->commandQueue.Get())
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

/*virtual*/ void RenderPass::Render(GraphicsEngine* graphicsEngine)
{
}