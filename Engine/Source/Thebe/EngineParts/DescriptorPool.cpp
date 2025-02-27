#include "Thebe/EngineParts/DescriptorPool.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

DescriptorPool::DescriptorPool()
{
	::ZeroMemory(&this->descriptorHeapDesc, sizeof(this->descriptorHeapDesc));
	this->incrementSize = 0;
}

/*virtual*/ DescriptorPool::~DescriptorPool()
{
}

/*virtual*/ bool DescriptorPool::Setup()
{
	if (this->descriptorHeap.Get())
	{
		THEBE_LOG("Descriptor pool already setup.");
		return false;
	}

	if (this->descriptorHeapDesc.NumDescriptors == 0)
	{
		THEBE_LOG("Can't create descriptor heap of zero size.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	HRESULT result = graphicsEngine->GetDevice()->CreateDescriptorHeap(&this->descriptorHeapDesc, IID_PPV_ARGS(&this->descriptorHeap));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create descriptor heap.  Error: 0x%08x", result);
		return false;
	}

	this->descriptorOffsetStack.resize(this->descriptorHeapDesc.NumDescriptors);
	for (uint64_t i = 0; i < (uint64_t)this->descriptorHeapDesc.NumDescriptors; i++)
		this->descriptorOffsetStack[i] = uint64_t(this->descriptorHeapDesc.NumDescriptors) - 1 - i;

	this->cpuBaseHandle = this->descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	this->gpuBaseHandle = this->descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	this->incrementSize = graphicsEngine->GetDevice()->GetDescriptorHandleIncrementSize(this->descriptorHeapDesc.Type);

	return true;
}

/*virtual*/ void DescriptorPool::Shutdown()
{
	this->descriptorHeap = nullptr;
}

D3D12_DESCRIPTOR_HEAP_DESC& DescriptorPool::GetDescriptorHeapDesc()
{
	return this->descriptorHeapDesc;
}

ID3D12DescriptorHeap* DescriptorPool::GetDescriptorHeap()
{
	return this->descriptorHeap.Get();
}

bool DescriptorPool::AllocDescriptor(Descriptor& descriptor)
{
	if (this->descriptorOffsetStack.size() == 0)
		return false;

	uint64_t offset = this->descriptorOffsetStack.back();
	this->descriptorOffsetStack.pop_back();

	descriptor.cpuHandle.InitOffsetted(this->cpuBaseHandle, (INT)offset, (UINT)this->incrementSize);
	descriptor.gpuHandle.InitOffsetted(this->gpuBaseHandle, (INT)offset, (UINT)this->incrementSize);

	return true;
}

bool DescriptorPool::FreeDescriptor(const Descriptor& descriptor)
{
	uint64_t cpuOffset = 0, gpuOffset = 0;

	cpuOffset = (uint64_t(descriptor.cpuHandle.ptr) - uint64_t(this->cpuBaseHandle.ptr)) / this->incrementSize;
	gpuOffset = (uint64_t(descriptor.gpuHandle.ptr) - uint64_t(this->gpuBaseHandle.ptr)) / this->incrementSize;

	THEBE_ASSERT(cpuOffset == gpuOffset);
	if (cpuOffset != gpuOffset)
		return false;

	THEBE_ASSERT(cpuOffset < this->descriptorHeapDesc.NumDescriptors);
	if (cpuOffset >= this->descriptorHeapDesc.NumDescriptors)
		return false;

	this->descriptorOffsetStack.push_back(cpuOffset);

	return true;
}