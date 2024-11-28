#include "Thebe/EngineParts/UploadHeap.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include <d3dx12.h>

using namespace Thebe;

UploadHeap::UploadHeap()
{
	this->uploadBufferMapped = nullptr;
	this->uploadBufferSize = 0L;
}

/*virtual*/ UploadHeap::~UploadHeap()
{
}

void UploadHeap::SetUploadBufferSize(UINT64 uploadBufferSize)
{
	this->uploadBufferSize = uploadBufferSize;
}

UINT8* UploadHeap::GetUploadBufferPtr()
{
	return this->uploadBufferMapped;
}

/*virtual*/ bool UploadHeap::Setup()
{
	if (this->uploadBuffer.Get())
	{
		THEBE_LOG("Upload heap already setup.");
		return false;
	}

	if (this->uploadBufferSize == 0)
	{
		THEBE_LOG("Upload buffer size is zero.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(this->uploadBufferSize);
	HRESULT result = graphicsEngine->GetDevice()->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&this->uploadBuffer));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to created upload buffer resource.  Error: 0x%08x", result);
		return false;
	}

	this->blockManager.Reset(this->uploadBufferSize);

	CD3DX12_RANGE readRange(0, 0);
	result = this->uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&this->uploadBufferMapped));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to mapped upload buffer into CPU memory.  Error: 0x%08x", result);
		return false;
	}

	return true;
}

/*virtual*/ void UploadHeap::Shutdown()
{
	if (this->uploadBuffer.Get() && this->uploadBufferMapped)
		this->uploadBuffer->Unmap(0, nullptr);

	this->uploadBufferMapped = nullptr;
	this->blockMap.clear();
	this->blockManager.Reset(0);
}

bool UploadHeap::Allocate(UINT64 size, UINT64 align, UINT64& offset)
{
	BlockManager::BlockNode* blockNode = this->blockManager.Allocate(size, align);
	if (!blockNode)
	{
		THEBE_LOG("Block manager failed to allocate %llu bytes with %llu alignment.", size, align);
		return false;
	}

	offset = blockNode->GetBlock()->GetOffset();
	auto pair = this->blockMap.find(offset);
	if (pair != this->blockMap.end())
		THEBE_LOG("Block manager allocated from a location that is supposedly already allocated!  Uh, this shouldn't happen.");
	else
		this->blockMap.insert(std::pair(offset, blockNode));

	return true;
}

bool UploadHeap::Deallocate(UINT64 offset)
{
	auto pair = this->blockMap.find(offset);
	if (pair == this->blockMap.end())
	{
		THEBE_LOG("Failed to look-up offset 0x%016x for deallocation.", offset);
		return false;
	}

	BlockManager::BlockNode* blockNode = pair->second;
	if (!this->blockManager.Deallocate(blockNode))
	{
		THEBE_LOG("Block manager rejected block node for deallocation.");
		return false;
	}

	return true;
}