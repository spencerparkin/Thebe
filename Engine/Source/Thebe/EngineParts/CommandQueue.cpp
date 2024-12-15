#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

CommandQueue::CommandQueue()
{
}

/*virtual*/ CommandQueue::~CommandQueue()
{
}

/*virtual*/ bool CommandQueue::Setup()
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
	if (!this->fence->Setup())
	{
		THEBE_LOG("Failed to create fence for command queue.");
		return false;
	}

	return true;
}

/*virtual*/ void CommandQueue::Shutdown()
{
	if (this->fence.Get())
	{
		this->fence->Shutdown();
		this->fence = nullptr;
	}

	this->commandQueue = nullptr;
}

bool CommandQueue::ExecuteCommandList(ID3D12CommandList* commandList)
{
	if (!this->commandQueue.Get())
		return false;

	this->commandQueue->ExecuteCommandLists(1, &commandList);
	return true;
}

void CommandQueue::WaitForCommandQueueComplete()
{
	if (this->commandQueue.Get() && this->fence.Get())
		this->fence->EnqueueSignalAndWaitForIt(this->commandQueue.Get());
}

ID3D12CommandQueue* CommandQueue::GetCommandQueue()
{
	return this->commandQueue.Get();
}