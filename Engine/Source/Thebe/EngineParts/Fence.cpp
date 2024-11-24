#include "Thebe/EngineParts/Fence.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

Fence::Fence()
{
	this->eventHandle = NULL;
	this->count = 0L;
}

/*virtual*/ Fence::~Fence()
{
}

/*virtual*/ bool Fence::Setup()
{
	if (this->eventHandle != NULL || this->fence.Get())
	{
		THEBE_LOG("Fence already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	if (!device)
		return false;

	HRESULT result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->fence));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create fence.  Error: 0x%08x", result);
		return false;
	}

	this->eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (this->eventHandle == NULL)
	{
		THEBE_LOG("Failed to create event.  Error: 0x%08x", GetLastError());
		return false;
	}

	this->count = 0L;

	return true;
}

/*virtual*/ void Fence::Shutdown()
{
	if (this->eventHandle != NULL)
	{
		CloseHandle(this->eventHandle);
		this->eventHandle = NULL;
	}

	this->fence = nullptr;
	this->count = 0L;
}

void Fence::EnqueueSignalAndWaitForIt(ID3D12CommandQueue* commandQueue)
{
	this->EnqueueSignal(commandQueue);
	this->WaitForSignalIfNecessary();
}

void Fence::EnqueueSignal(ID3D12CommandQueue* commandQueue)
{
	this->count = this->fence->GetCompletedValue() + 1;
	commandQueue->Signal(this->fence.Get(), this->count);
}

void Fence::WaitForSignalIfNecessary()
{
	if (this->fence->GetCompletedValue() >= this->count)
		return;

	HRESULT result = this->fence->SetEventOnCompletion(this->count, this->eventHandle);
	if (FAILED(result))
		THEBE_LOG("Failed to set event on fence completion!");
	else
	{
		WaitForSingleObjectEx(this->eventHandle, INFINITE, FALSE);
		THEBE_ASSERT(this->count == this->fence->GetCompletedValue());
	}
}