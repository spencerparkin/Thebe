#include "Thebe/EngineParts/IndexBuffer.h"

using namespace Thebe;

IndexBuffer::IndexBuffer()
{
	::memset(&this->indexBufferView, 0, sizeof(this->indexBufferView));
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	this->primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

/*virtual*/ IndexBuffer::~IndexBuffer()
{
}

/*virtual*/ bool IndexBuffer::Setup(void* data)
{
	if (!Buffer::Setup(data))
		return false;

	auto indexBufferSetupData = static_cast<IndexBufferSetupData*>(data);

	if (this->type == Type::STATIC || this->type == Type::DYNAMIC_BARRIER_METHOD)
		this->indexBufferView.BufferLocation = this->fastMemBuffer->GetGPUVirtualAddress();
	else
		this->indexBufferView.BufferLocation = this->slowMemBuffer->GetGPUVirtualAddress();

	this->indexBufferView.SizeInBytes = this->bufferSize;
	this->indexBufferView.Format = indexBufferSetupData->format;
	this->primitiveTopology = indexBufferSetupData->primitiveTopology;

	return true;
}

/*virtual*/ void IndexBuffer::Shutdown()
{
	Buffer::Shutdown();

	::memset(&this->indexBufferView, 0, sizeof(this->indexBufferView));
}