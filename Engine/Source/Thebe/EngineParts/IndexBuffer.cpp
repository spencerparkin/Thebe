#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/Log.h"

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

void IndexBuffer::SetFormat(DXGI_FORMAT format)
{
	this->indexBufferView.Format = format;
}

void IndexBuffer::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	this->primitiveTopology = primitiveTopology;
}

/*virtual*/ bool IndexBuffer::Setup()
{
	if (this->primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
	{
		THEBE_LOG("No primitive topology configured.");
		return false;
	}

	if (!Buffer::Setup())
		return false;

	if (this->type == Type::STATIC || this->type == Type::DYNAMIC_BARRIER_METHOD)
		this->indexBufferView.BufferLocation = this->fastMemBuffer->GetGPUVirtualAddress();
	else
		this->indexBufferView.BufferLocation = this->slowMemBuffer->GetGPUVirtualAddress();

	this->indexBufferView.SizeInBytes = this->bufferSize;

	return true;
}

/*virtual*/ void IndexBuffer::Shutdown()
{
	Buffer::Shutdown();

	::memset(&this->indexBufferView, 0, sizeof(this->indexBufferView));
}