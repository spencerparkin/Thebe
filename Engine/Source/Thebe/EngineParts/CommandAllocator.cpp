#include "Thebe/EngineParts/CommandAllocator.h"
#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

CommandAllocator::CommandAllocator()
{
}

/*virtual*/ bool CommandAllocator::Setup()
{
	if (this->commandAllocator.Get())
	{
		THEBE_LOG("Command allocator already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	this->fence.Set(new Fence());
	this->fence->SetGraphicsEngine(graphicsEngine.Get());
	if (!this->fence->Setup())
	{
		THEBE_LOG("Failed to create fence for command allocator.");
		return false;
	}

	ID3D12Device* device = graphicsEngine->GetDevice();

	HRESULT result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->commandAllocator));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create command allocator.  Error: 0x%08x", result);
		return false;
	}

	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->commandAllocator.Get(), nullptr, IID_PPV_ARGS(&this->commandList));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create command list.  Error: 0x%08x", result);
		return false;
	}

	// Command lists are created in the recording state.  We're not yet ready to record, so close it now.
	this->commandList->Close();

	return true;
}

/*virtual*/ void CommandAllocator::Shutdown()
{
	this->commandList = nullptr;
	this->commandAllocator = nullptr;
}

/*virtual*/ CommandAllocator::~CommandAllocator()
{
}

bool CommandAllocator::BeginRecordingCommandList()
{
	// Make sure the GPU is finished with our command allocator before we reset it.
	this->fence->WaitForSignalIfNecessary();

	// It should now be safe to reset the allocator.
	HRESULT result = this->commandAllocator->Reset();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to reset command allocator.  Error: 0x%08x", result);
		return false;
	}

	// Have the command-list allocate from our command allocator.  Note that this also opens the command list for recording.
	result = this->commandList->Reset(this->commandAllocator.Get(), nullptr);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to reset command list.  Error: 0x%08x", result);
		return false;
	}

	return true;
}

bool CommandAllocator::EndRecordingCommandList()
{
	HRESULT result = this->commandList->Close();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to close command list.  Error: 0x%08x", result);
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	CommandQueue* commandQueue = graphicsEngine->GetCommandQueue();
	if (!commandQueue)
		return false;

	// Kick-off the GPU to start executing our command-list.
	if (!commandQueue->ExecuteCommandList(this->commandList.Get()))
		return false;

	// Some derivatives need to act before we enqueue our signal.
	this->PreSignal();

	// Enqueue a signal/fence we can use to know that our command allocator memory is no longer in use.
	this->fence->EnqueueSignal(commandQueue->GetCommandQueue());

	return true;
}

/*virtual*/ void CommandAllocator::PreSignal()
{
}

ID3D12GraphicsCommandList* CommandAllocator::GetCommandList()
{
	return this->commandList.Get();
}