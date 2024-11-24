#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

VertexBuffer::VertexBuffer()
{
	::memset(&this->vertexBufferView, 0, sizeof(this->vertexBufferView));
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

/*virtual*/ VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::SetStride(UINT32 stride)
{
	this->vertexBufferView.StrideInBytes = stride;
}

/*virtual*/ bool VertexBuffer::Setup()
{
	if (this->elementDescArray.size() == 0)
	{
		THEBE_LOG("Element description array not configured.");
		return false;
	}

	if (this->vertexBufferView.StrideInBytes == 0)
	{
		THEBE_LOG("Stride not configured.");
		return false;
	}

	if (!Buffer::Setup())
		return false;

	if (this->type == Type::STATIC || this->type == Type::DYNAMIC_BARRIER_METHOD)
		this->vertexBufferView.BufferLocation = this->fastMemBuffer->GetGPUVirtualAddress();
	else
		this->vertexBufferView.BufferLocation = this->slowMemBuffer->GetGPUVirtualAddress();

	this->vertexBufferView.SizeInBytes = this->bufferSize;

	return true;
}

/*virtual*/ void VertexBuffer::Shutdown()
{
	Buffer::Shutdown();

	::memset(&this->vertexBufferView, 0, sizeof(this->vertexBufferView));
	this->elementDescArray.clear();
}

std::vector<D3D12_INPUT_ELEMENT_DESC>& VertexBuffer::GetElementDescArray()
{
	return this->elementDescArray;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& VertexBuffer::GetElementDescArray() const
{
	return this->elementDescArray;
}