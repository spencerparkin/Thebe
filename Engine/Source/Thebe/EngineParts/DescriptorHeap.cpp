#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------------- DescriptorHeap -------------------------------------

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

	if (this->descriptorHeapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		this->descriptorHeapDesc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	HRESULT result = graphicsEngine->GetDevice()->CreateDescriptorHeap(&this->descriptorHeapDesc, IID_PPV_ARGS(&this->descriptorHeap));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create descriptor heap.  Error: 0x%08x", result);
		return false;
	}

	this->blockMap.clear();
	this->blockManager.Reset(this->descriptorHeapDesc.NumDescriptors);
	return true;
}

/*virtual*/ void DescriptorHeap::Shutdown()
{
	this->descriptorHeap = nullptr;
	this->blockManager.Reset(0);
	this->blockMap.clear();
}

D3D12_DESCRIPTOR_HEAP_DESC& DescriptorHeap::GetDescriptorHeapDesc()
{
	return this->descriptorHeapDesc;
}

ID3D12DescriptorHeap* DescriptorHeap::GetDescriptorHeap()
{
	return this->descriptorHeap.Get();
}

bool DescriptorHeap::AllocDescriptorSet(UINT numDescriptors, DescriptorSet& descriptorSet)
{
	BlockManager::BlockNode* blockNode = this->blockManager.Allocate(numDescriptors, 1);
	if (!blockNode)
	{
		THEBE_LOG("Failed to allocate %d descriptors.", numDescriptors);
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	descriptorSet.offset = blockNode->GetBlock()->GetOffset();
	descriptorSet.size = numDescriptors;
	descriptorSet.descriptorSize = graphicsEngine->GetDevice()->GetDescriptorHandleIncrementSize(this->descriptorHeapDesc.Type);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(this->descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(descriptorSet.offset, descriptorSet.descriptorSize);
	descriptorSet.cpuHandle = cpuHandle;

	if ((this->descriptorHeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(this->descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(descriptorSet.offset, descriptorSet.descriptorSize);
		descriptorSet.gpuHandle = gpuHandle;
	}
	else
	{
		descriptorSet.gpuHandle.ptr = 0;
	}

	descriptorSet.heapHandle = this->GetHandle();

	this->blockMap.insert(std::pair(descriptorSet.offset, blockNode));

	return true;
}

bool DescriptorHeap::FreeDescriptorSet(DescriptorSet& descriptorSet)
{
	if (descriptorSet.heapHandle == THEBE_INVALID_REF_HANDLE)
	{
		THEBE_LOG("Can't free descriptor set that's already freed.");
		return false;
	}

	if (descriptorSet.heapHandle != this->GetHandle())
	{
		THEBE_LOG("Tried to free descriptor set in the wrong descriptor heap.");
		return false;
	}

	auto iter = this->blockMap.find(descriptorSet.offset);
	if (iter == this->blockMap.end())
	{
		THEBE_LOG("Did not find block for descriptor set offset.");
		return false;
	}

	BlockManager::BlockNode* blockNode = iter->second;
	if (!this->blockManager.Deallocate(blockNode))
	{
		THEBE_LOG("Block manager for descriptor heap failed to deallocate block.");
		return false;
	}

	this->blockMap.erase(iter);

	descriptorSet.offset = 0;
	descriptorSet.size = 0;
	descriptorSet.heapHandle = THEBE_INVALID_REF_HANDLE;

	return true;
}

//------------------------------------- DescriptorHeap::DescriptorSet -------------------------------------

DescriptorHeap::DescriptorSet::DescriptorSet()
{
	this->heapHandle = THEBE_INVALID_REF_HANDLE;
	this->offset = 0;
	this->size = 0;
	this->descriptorSize = 0;
}

bool DescriptorHeap::DescriptorSet::GetCpuHandle(UINT i, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle) const
{
	if (i >= this->size)
		return false;

	handle = this->cpuHandle;
	handle.Offset(i, this->descriptorSize);
	return true;
}

bool DescriptorHeap::DescriptorSet::GetGpuHandle(UINT i, CD3DX12_GPU_DESCRIPTOR_HANDLE& handle) const
{
	if (i >= this->size)
		return false;

	handle = this->gpuHandle;
	handle.Offset(i, this->descriptorSize);
	return true;
}

bool DescriptorHeap::DescriptorSet::IsAllocated() const
{
	return this->heapHandle != THEBE_INVALID_REF_HANDLE;
}