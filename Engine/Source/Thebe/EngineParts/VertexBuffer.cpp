#include "Thebe/EngineParts/VertexBuffer.h"

using namespace Thebe;

VertexBuffer::VertexBuffer()
{
	::memset(&this->vertexBufferView, 0, sizeof(this->vertexBufferView));
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

/*virtual*/ VertexBuffer::~VertexBuffer()
{
}

/*virtual*/ bool VertexBuffer::Setup(void* data)
{
	if (!Buffer::Setup(data))
		return false;

	auto vertexBufferSetupData = static_cast<VertexBufferSetupData*>(data);

	if (this->type == Type::STATIC || this->type == Type::DYNAMIC_BARRIER_METHOD)
		this->vertexBufferView.BufferLocation = this->fastMemBuffer->GetGPUVirtualAddress();
	else
		this->vertexBufferView.BufferLocation = this->slowMemBuffer->GetGPUVirtualAddress();

	this->vertexBufferView.SizeInBytes = this->bufferSize;
	this->vertexBufferView.StrideInBytes = vertexBufferSetupData->strideInBytes;

	this->elementDescArray = vertexBufferSetupData->elementDescArray;

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