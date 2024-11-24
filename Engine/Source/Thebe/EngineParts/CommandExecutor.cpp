#include "Thebe/EngineParts/CommandExecutor.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

CommandExecutor::CommandExecutor()
{
}

/*virtual*/ CommandExecutor::~CommandExecutor()
{
}

/*virtual*/ bool CommandExecutor::Setup(void* data)
{
	if (this->commandAllocator.Get())
	{
		THEBE_LOG("Command executor already setup.");
		return false;
	}

	if (!CommandQueue::Setup(data))
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	if (!device)
		return false;

	HRESULT result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->commandAllocator));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create command allocator.  Error: 0x%08x", result);
		return false;
	}

	return true;
}

/*virtual*/ void CommandExecutor::Shutdown()
{
	CommandQueue::Shutdown();

	this->commandAllocator = nullptr;
	this->commandListArray.clear();
}

bool CommandExecutor::BeginRecording(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	if (!device)
		return false;

	// Note that command-lists are created in the recording state, so we're ready to record now.
	HRESULT result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create command list.  Error: 0x%08x", result);
		return false;
	}

	return true;
}

bool CommandExecutor::EndRecording(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	HRESULT result = commandList->Close();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to close command list.  Error: 0x%08x", result);
		return false;
	}

	this->commandListArray.push_back(commandList);
	return true;
}

void CommandExecutor::Execute()
{
	std::vector<ID3D12CommandList*> commandListPtrArray;
	for (ComPtr<ID3D12GraphicsCommandList>& commandList : this->commandListArray)
		commandListPtrArray.push_back(commandList.Get());

	this->commandQueue->ExecuteCommandLists(commandListPtrArray.size(), commandListPtrArray.data());
	this->WaitForCommandQueueComplete();
	this->commandListArray.clear();
	this->commandAllocator->Reset();
}