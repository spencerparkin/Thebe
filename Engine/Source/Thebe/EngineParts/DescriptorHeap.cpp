#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

DescriptorHeap::Descriptor::Descriptor()
{
	this->heapHandle = THEBE_INVALID_REF_HANDLE;
	this->i = 0;
}

DescriptorHeap::DescriptorHeap()
{
	::ZeroMemory(&this->descriptorHeapDesc, sizeof(this->descriptorHeapDesc));
}

/*virtual*/ DescriptorHeap::~DescriptorHeap()
{
}

/*virtual*/ bool DescriptorHeap::Setup()
{
	if (this->descriptorHeap.Get())
	{
		THEBE_LOG("Descriptor heap already setup.");
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

	this->freeDescriptorsStack.resize(this->descriptorHeapDesc.NumDescriptors);
	for (UINT i = 0; i < this->descriptorHeapDesc.NumDescriptors; i++)
		this->freeDescriptorsStack[i] = this->descriptorHeapDesc.NumDescriptors - 1 - i;

	return true;
}

/*virtual*/ void DescriptorHeap::Shutdown()
{
	this->descriptorHeap = nullptr;
	this->freeDescriptorsStack.clear();
}

D3D12_DESCRIPTOR_HEAP_DESC& DescriptorHeap::GetDescriptorHeapDesc()
{
	return this->descriptorHeapDesc;
}

ID3D12DescriptorHeap* DescriptorHeap::GetDescriptorHeap()
{
	return this->descriptorHeap.Get();
}

bool DescriptorHeap::AllocDescriptor(Descriptor& descriptor)
{
	if (this->freeDescriptorsStack.size() == 0)
	{
		THEBE_LOG("Ran out of descriptors in descriptor heap.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	descriptor.i = this->freeDescriptorsStack.back();
	this->freeDescriptorsStack.pop_back();

	UINT descriptorSize = graphicsEngine->GetDevice()->GetDescriptorHandleIncrementSize(this->descriptorHeapDesc.Type);
	
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(this->descriptorHeap->GetGPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(this->descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	gpuHandle.Offset(descriptor.i, descriptorSize);
	cpuHandle.Offset(descriptor.i, descriptorSize);

	descriptor.gpuHandle = gpuHandle;
	descriptor.cpuHandle = cpuHandle;

	descriptor.heapHandle = this->GetHandle();

	return true;
}

bool DescriptorHeap::FreeDescriptor(Descriptor& descriptor)
{
	if (descriptor.heapHandle == THEBE_INVALID_REF_HANDLE)
	{
		THEBE_LOG("Can't free descriptor that's already freed.");
		return false;
	}

	if (this->freeDescriptorsStack.size() == this->descriptorHeapDesc.NumDescriptors)
	{
		THEBE_LOG("Can't free a descriptor in a heap that's full.");
		return false;
	}

	if (descriptor.heapHandle != this->GetHandle())
	{
		THEBE_LOG("Tried to free descriptor in the wrong descriptor heap.");
		return false;
	}

	if (descriptor.i >= this->descriptorHeapDesc.NumDescriptors)
	{
		THEBE_LOG("Given descriptor for free is corrupt.");
		return false;
	}

	this->freeDescriptorsStack.push_back(descriptor.i);
	descriptor.heapHandle = THEBE_INVALID_REF_HANDLE;
	return true;
}